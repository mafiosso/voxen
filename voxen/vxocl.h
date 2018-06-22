#ifndef VX_OCL_H
#define VX_OCL_H

#ifndef NO_OPENCL

typedef struct VX_CL_device{
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
    const char * entry_point;
    
    int (*push_arg)( struct VX_CL_program * , void * , size_t );
    int (*enter)( struct VX_CL_program * , VX_uint32 wi_count );
    int (*release)( struct VX_CL_program *);

}VX_CL_program;

VX_CL_device * VX_CL_device_new();
VX_CL_program * VX_CL_program_new( VX_CL_device * dev , 
                                   char ** sources,
                                   const char * entry_call );

#endif
#endif
