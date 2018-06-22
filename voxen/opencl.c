#include "VX_lib.h"

#ifndef NO_OPENCL

/*typedef struct VX_CL_device{
    VX_uint32 exec_cores;

    cl_platform_id platform;
    cl_context context;
    cl_command_queue queue;
    cl_device_id device;

    void (*release)( struct VX_CL_device * );
}VX_CL_device;

typedef struct VX_CL_program{
    VX_CL_device * dev;
    cl_program p;
    cl_kernel  ker;
    int arg_top;
    int cpu_blocking_p;
    sem_t block;
    
    int (*push_arg)( struct VX_CL_program * , void * , size_t );
    int (*enter)( struct VX_CL_program * , VX_uint32 wi_count );
    int (*release)( struct VX_CL_program *);

}VX_CL_program;
*/

int VX_CL_program_release( VX_CL_program * p ){
    clReleaseKernel( p->ker );
    clReleaseProgram( p->p );
    free( p );
    return 0;
}

int VX_CL_program_push_arg( VX_CL_program * p , void * arg_val ,
                             size_t asz ){
    int ret = clSetKernelArg( p->ker , p->arg_top++ , asz , arg_val);
    if( ret != CL_SUCCESS ){
        printf("warn: can't set argument properly - releasing device!\n");
        p->release( p );
        return -1;
    }

    return 0;
}

int VX_CL_program_enter( VX_CL_program * p , VX_uint32 wi_count ){
    cl_event event;

    clFinish( p->dev->queue );

    cl_uint ret = clEnqueueNDRangeKernel( p->dev->queue, p->ker,
                                          1, NULL, &wi_count , NULL,
                                          0, NULL, &event);

    p->arg_top = 0;

    if( ret != CL_SUCCESS ){
        printf("err: (ocl) can't execute kernel OpenCL returned %u\n" , ret );
        return -1;
    }

    clFinish( p->dev->queue );
    return 0;    
}

/* sources is NULL terminated array! */
VX_CL_program * VX_CL_program_new( VX_CL_device * dev , 
                                   char ** sources,
                                   const char * entry_call )
{
    cl_uint sources_count = 0;

    while( sources[sources_count] ){
        sources_count++;
    }

    size_t lens[sources_count];
    for( int i = 0 ; i < sources_count; i++ ){
        lens[i] = strlen( sources[i] );
    }

    cl_int ret;

    cl_program p = clCreateProgramWithSource ( dev->context, sources_count ,
                                               (const char**)sources, lens,
                                               &ret);

    if( ret != CL_SUCCESS ){
        printf("err: (ocl) can't create program from sources!\n");
        return NULL;
    }

    ret = clBuildProgram(p, 1, &(dev->device), NULL /* "-cl-single-precision-constant -cl-denorms-are-zero -cl-opt-disable -cl-strict-aliasing -cl-mad-enable -cl-no-signed-zeros -cl-unsafe-math-optimizations -cl-finite-math-only -cl-fast-relaxed-math"*/ , NULL, NULL);

    /* compilation log echo */
    
    size_t log_size;

    clGetProgramBuildInfo(p, dev->device, CL_PROGRAM_BUILD_LOG,
                          0, NULL, &log_size);
    char log[log_size+1];

    clGetProgramBuildInfo(p, dev->device, CL_PROGRAM_BUILD_LOG,
                          log_size, log, NULL);
    log[log_size] = '\0';
    printf("build: (ocl) OpenCL compilation process returned >>>>\n%s" , log );
    
    if( ret != CL_SUCCESS ){
        printf("err: (ocl) can't build program from sources!\n");
        return NULL;
    }

    cl_kernel ker = clCreateKernel(p, entry_call , &ret);

    if( ret != CL_SUCCESS ){
        printf("err: (ocl) no such kernel %s\n" , entry_call );
        return NULL;
    }

    VX_CL_program * out = calloc( 1 , sizeof(VX_CL_program) );
    out->p = p;
    out->ker = ker;
    sem_init( &(out->block) , 0 , 1 );

    out->enter = VX_CL_program_enter;
    out->release = VX_CL_program_release;
    out->push_arg = VX_CL_program_push_arg;
    out->dev = dev;
    out->entry_point = entry_call;

    return out;
}

void VX_CL_device_release( VX_CL_device * dev ){
    clReleaseCommandQueue( dev->queue );
    clReleaseContext( dev->context );
    //clReleaseDevice( dev->device );
    free( dev );    
}

VX_CL_device * VX_CL_device_new(){
    VX_CL_device * out = calloc( 1 , sizeof( VX_CL_device ) );
    cl_int ret;
    cl_uint p_count = 0;

    /* enumerate platforms first and then query for GPU */
    ret = clGetPlatformIDs( 1 , &(out->platform) , &p_count );
    if (ret != CL_SUCCESS) {
        printf("err: (ocl) can't get platform ID!\n");
        goto err;
    }

    printf("platform count: %d\n" , p_count );

    ret = clGetDeviceIDs( out->platform, CL_DEVICE_TYPE_GPU,
                          1, &(out->device), NULL);
    if (ret != CL_SUCCESS) {
        printf("err: (ocl) can't get device ID!\n");
        goto err;
    }

    out->context = clCreateContext(0, 1, &(out->device), NULL, NULL, &ret);
    if (ret != CL_SUCCESS) {
        printf("err: (ocl) can't create context!\n");
        goto err;
    }

    out->queue = clCreateCommandQueue(out->context, out->device, 0, &ret);
    if (ret != CL_SUCCESS) {
        printf("err: (ocl) can't create command queue!\n");
        goto err;
    }

    /* TODO: ask for more device caps */
    out->release = VX_CL_device_release;

    return out;

err:
    free( out );
    return NULL;
}

#endif
