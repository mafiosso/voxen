/* Basic test set for the VoXen library */
#include "VX_lib.h"

/* Tested successfully on 08-04-2013 on i386 Ubuntu
   - no Segmentation fault detected
   - no valgrind errors present (except 2 suppressed from SDL)
   - no memory leaks (except those in SDL)
*/

#define TEST_FAIL 0
#define TEST_SUCC 1
#define TEST_SKIP 2

int test_fundamentals(){
    VX_ms_block b1 = {0 , 0 , 0 , 256 , 256 , 256 };
    VX_ms_block b2 = {64 , 64 , 64 , 128 , 128 , 128};

    VX_ms_block true_intersection = {64,64,64,128,128,128};
    VX_ms_block computed_intersection = VX_cubes_intersection( b1 , b2 );

    if( memcmp( &computed_intersection ,
                &true_intersection ,
                sizeof(VX_ms_block) ) )
    {
        printf("test: intersection failed\n");
        PRINT_MS_BLOCK( computed_intersection );
        return 0;
    }

    VX_block b = MS_BLOCK_TO_BOX( b1 );
    VX_block true_b = {0 , 0 , 0 , 256 , 256 , 256};

    if( memcmp( &b , &true_b , sizeof( VX_block ) ) ){
        printf("test: casting VX_ms_block to VX_block failed\n");
        PRINT_BLOCK( b );
        return 0;
    }

    VX_ms_block mb = BOX_TO_MS_BLOCK( b );
    
    if( memcmp( &mb , &b1 , sizeof( VX_ms_block ) ) ){
        printf("test: casting VX_block to VX_ms_block failed\n");
        PRINT_MS_BLOCK( mb );
        return 0;
    }

    VX_ipoint3 p = {54, 64 , 38};
    if( !VX_point_in_cube_p( b1 , p ) ){
        printf("test: point lying inside cube failed\n");
        return 0;
    }

    PRINT_MS_BLOCK( b1 );
    PRINT_MS_BLOCK( b2 );

    if( !VX_cubes_intersects_p( b1 , b2 ) ){
        printf("test: testing of intersection of cubes failed\n");
        return 0;
    }

    return TEST_SUCC;
}

int matrix4_cmp( double m1 [4][4] , double m2 [4][4] ){
    for( int y = 0 ; y < 4 ; y++ ){
        for( int x = 0 ; x < 4 ; x++ ){
            if( m1[y][x] != m2[y][x] )
                return 0;
        }
    }

    return 1;
}

void matrix4_fill( double m[4][4] , double item ){
    for( int y = 0 ; y < 4 ; y++ ){
        for( int x = 0 ; x < 4 ; x++ ){
            m[y][x] = item;
        }
    }
}

int test_matrix(){
    float vector[4] = {5,5,5,5};
    double m1[4][4] = {{1,0,0,1},
                       {0,1,0,0},
                       {0,0,1,0},
                       {0,0,0,1}};

    double m2[4][4] = {{1,0,0,0},
                       {0,1,0,0},
                       {0,0,1,0},
                       {1,0,0,1}};

    double e[4][4];
    VX_matrix4_identity( e );

    double c[4][4];
    matrix4_fill( c , 2.0f );

    double chk1[4][4] = {{4,4,4,4},{2,2,2,2},{2,2,2,2},{2,2,2,2}};
    double chk2[4][4] = {{2,2,2,2},{2,2,2,2},{2,2,2,2},{4,4,4,4}};

    float vchk1[4] = {10,5,5,5};
    float vchk2[4] = {5,5,5,10};
    
    double tmp[4][4];
    VX_matrix4_identity( tmp );
    VX_matrix4_multiply( tmp , m1 , c );

    if( !matrix4_cmp( tmp , chk1 )) {
        printf("err: matrix multiplication error\n");
        return TEST_FAIL;
    }

    VX_matrix4_multiply( tmp , m2 , c );

    if( !matrix4_cmp( tmp , chk2 ) )
    {
        printf("err: matrix multiplication error\n");
        return TEST_FAIL;
    }
    
    float vtmp[4];

    VX_fvector4_multiply( vtmp , m1 , vector  );
    if( memcmp( vtmp , vchk1 , sizeof(float)*4 ) ){
        printf("err: vector multiplication by matrix failed\n");
        return TEST_FAIL;
    }

    VX_fvector4_multiply( vtmp , m2 , vector );
    if( memcmp( vtmp , vchk2 , sizeof(float)*4 ) ){
        printf("err: vector multiplication by matrix failed\n");
        return TEST_FAIL;
    }            

    return TEST_SUCC;
}

int test_null_drv(){
    int ret = VX_init( VX_DRV_null , 640 , 480 , 0 );

    if( ret != 0 ){
        printf("err: can not create NULL driver!\n");
        return TEST_FAIL;
    }

    VX_surface * s = VX_lib.make_surface( 20 , 20 );
    if( !s ){
        printf("err: can not create NULL surface!\n");
        return TEST_FAIL;
    }

    if( !s->lock_surface( s ) ){
        printf("err: can not lock NULL surface!\n");
        return TEST_FAIL;
    }

    s->set_pixel( s , 2 , 5 , 0xffffffff );
    if( s->get_pixel( s , 2 , 5 ) != 0xffffffff ){
        printf("err: setting and reading back pixel!\n");
        return TEST_FAIL;
    }

    VX_lib.redraw( &VX_lib );

    if( VX_lib.quit( &VX_lib ) != 0 ){
        printf("err: while quitting null driver\n");
        return TEST_FAIL;
    }

    s->free( s );
    
    return TEST_SUCC;
}

int test_sdl_drv(){
    int ret = VX_init( VX_DRV_sdl , 640 , 480 , 0 );

    if( ret != 0 ){
        printf("err: can not create SDL driver!\n");
        return TEST_FAIL;
    }

    VX_surface * s = VX_lib.make_surface( 20 , 20 );
    if( !s ){
        printf("err: can not create SDL surface!\n");
        return TEST_FAIL;
    }

    if( !s->lock_surface( s ) ){
        printf("err: can not lock SDL surface!\n");
        return TEST_FAIL;
    }

    s->set_pixel( s , 2 , 5 , 0x00ffffff );
    if( s->get_pixel( s , 2 , 5 ) != 0x00ffffff ){
        printf("err: setting and reading back pixel!\n");
        return TEST_FAIL;
    }

    s->unlock_surface( s );

    VX_lib.redraw( &VX_lib );

    if( VX_lib.quit( &VX_lib ) != 0 ){
        printf("err: while quitting SDL driver\n");
        return TEST_FAIL;
    }

    s->free( s );

    return TEST_SUCC;
}


VX_uint32 toARGB32( VX_format * self , VX_byte * chunk ){
    VX_uint32 out = *((VX_uint32*)chunk);
    return out;
}

VX_format FMT_ARGB32 = {32,4,8,8,8,8,0, toARGB32 };

int verify_compilation( VX_model * omodel , VX_model * rmodel){
    int lod = 0xff;

    for( int z = 0; z < omodel->dim_size.d ; z++ ){
        for( int y = 0; y < omodel->dim_size.h ; y++ ){
            for( int x = 0 ; x < omodel->dim_size.w ; x++ ){
                lod = 0xff;
                VX_byte* pix1 = omodel->get_voxel( omodel , x,y,z , &lod);
                lod = 0xff;
                VX_byte* pix2 = rmodel->get_voxel( rmodel , x,y,z , &lod);

                if( !pix1 || !pix2 )
                    continue;

                if( omodel->fmt->ARGB32( omodel->fmt , pix1 ) !=
                    rmodel->fmt->ARGB32( rmodel->fmt , pix2 ))
                {
                    printf("colors differ: %u != %u\n" , 
                           omodel->fmt->ARGB32( omodel->fmt , pix1 ) ,
                           rmodel->fmt->ARGB32( rmodel->fmt , pix2 ) );
                    printf("verification failed at point: p(%d %d %d)\n" , x , y, z );

                    return TEST_FAIL;
                }
            }
        }
    }

    return TEST_SUCC;
}

void fill_rand( VX_model * m ){
    for( int y = 0; y < 32 ; y++ ){
        for( int x = 0 ; x < 32 ; x++ ){
            for( int z = 0 ; z < 32 ; z++ ){
                VX_uint32 c = (rand()%256)|(rand()%256)<<8|(rand()%256)<<16;
                m->set_voxel( m , x , y , z , c);
            }
        }
    }
}

int test_raw(){
    VX_model * rm = VX_model_raw_new( &FMT_ARGB32 , 
                                      (VX_block){0 , 0 , 0 , 32 , 32 , 32});

    if( !rm ){
        printf("err: while creating raw model\n");
        return TEST_FAIL;
    }

    fill_rand( rm );

    int ret = rm->dump( rm , "/tmp/test.rmod" );
    if( ret != 0 ){
        printf("skip: while dumping raw model can't write into /tmp/\n");
        return TEST_SKIP;
    }

    VX_model * rmt = VX_model_raw_new( &FMT_ARGB32 , 
                                       (VX_block){0 , 0 , 0 , 32 , 32 , 32});
    if( !rmt ){
        printf("err: can not create test raw model\n");
        return TEST_SKIP;
    }

    ret = rmt->load( rmt , "/tmp/test.rmod" );
    if( ret != 0 ){
        printf("err: while loading raw model\n");
        return TEST_FAIL;
    }

    if( verify_compilation( rmt , rm ) != TEST_SUCC ){
        printf("err: verification of equalness data failed\n");
        return TEST_FAIL;
    }

    rm->inspect( rm );
    rm->free( rm );
    rmt->free( rmt );

    return TEST_SUCC;
}

int test_octree(){
    VX_model * oct = VX_model_octree_new();

    if( !oct ){
        printf("err: while creating octree model\n");
        return TEST_FAIL;
    }

    fill_rand( oct );

    int ret = oct->dump( oct , "/tmp/test.oct" );
    if( ret != 0 ){
        printf("skip: while dumping octree model can't write into /tmp/\n");
        return TEST_SKIP;
    }

    VX_model * rmt = VX_model_octree_new();
    if( !rmt ){
        printf("err: can not create test octree model\n");
        return TEST_SKIP;
    }

    ret = rmt->load( rmt , "/tmp/test.oct" );
    if( ret != 0 ){
        printf("err: while loading octree model\n");
        return TEST_FAIL;
    }

    if( verify_compilation( rmt , oct ) != TEST_SUCC ){
        printf("err: verification of equalness data failed\n");
        return TEST_FAIL;
    }

    oct->inspect( oct );
    oct->free( oct );
    rmt->free( rmt );

    return TEST_SUCC;
}

int test_adaptive_octree(){
    VX_model * myhead = VX_model_octree_new();
    VX_model * admyhead = VX_adaptive_octree_new();

    myhead->load( myhead , "../data/myheadhole.oct" );
    admyhead->compile( admyhead , myhead );
    myhead->inspect( myhead );
    admyhead->inspect( admyhead );

    return verify_compilation( admyhead , myhead );

    return TEST_SUCC;
}

int test_compilation(){
    VX_model * rm = VX_model_raw_new( &FMT_ARGB32 , 
                                      (VX_block){0 , 0 , 0 , 32 , 32 , 32});

    fill_rand( rm );
    VX_model * om = VX_model_octree_new();
    
    if( !rm || !om ){
        printf("skip: can not create on ore more models\n");
        return TEST_SKIP;
    }

    int ret = om->compile( om , rm );
    if( ret != 0 ){
        printf("err: cant compile raw model to octree!\n");
        return TEST_FAIL;
    }

    if( verify_compilation( om , rm ) != TEST_SUCC ){
        printf("err: output octtree model is not same as original raw\n");
        return TEST_FAIL;
    }

    ret = rm->compile( rm , om );
    if( ret != 0 ){
        printf("err: cant compile octree model into raw!\n");
        return TEST_FAIL;
    }

    if( verify_compilation( om , rm ) != TEST_SUCC ){
        printf("err: output raw model is not same as original octree\n");
        return TEST_FAIL;
    }

    om->free( om );
    rm->free( rm );

    return TEST_SUCC;
}

int test_camera(){
    int ret = VX_init( VX_DRV_null , 640 , 480 , 0 );
    if( ret != 0 ){
        printf("skip: due to null driver could not been established\n");
        return TEST_SKIP;
    }

    VX_camera * cam = VX_camera_new( VX_lib.native_surface , NULL , 1 );
    if( !cam ){
      printf("err: camera creation failed\n");
      return TEST_FAIL;
    }

    VX_model * om = VX_model_octree_new();
    fill_rand( om );
    
    cam->draw( cam , om );

    cam->destroy(cam);
    VX_lib.quit( &VX_lib );

    om->free( om );
    
    return TEST_SUCC;
}

const char * program = "__kernel void sum(__global const float *A,"
    "__global const float *B,"
    "__global float *C) {"
    "int i = get_global_id(0);"
    "C[i] = A[i] + B[i];"
    "}";

int test_opencl(){
    #ifndef NO_OPENCL

    VX_CL_device * cldev = VX_CL_device_new();

    if( !cldev ){
        printf("skip: opencl support not detected\n");
        return TEST_SKIP;
    }

    char * srcs [2] = { (char*)program , NULL };

    VX_CL_program * clprog = VX_CL_program_new( cldev , srcs , "sum" );
    VX_CL_program * p = clprog;

    size_t len = 1024*1024*32;
    float *pA = malloc(len*sizeof(float));
    float *pB = malloc(len*sizeof(float));
    float *pC = malloc(len*sizeof(float));

    if( !p ){
        printf("err: compilation failed!\n");
        goto fail;
    }

    pC[0] = pB[0] = pA[0] = 0.0f;
    pC[len/2] = pB[len/2] = pA[len/2] = 12.0f;
    pC[len-1] = pB[len-1] = pA[len-1] = 24.0f;

    cl_int ret;

    cl_mem a_mem_obj = clCreateBuffer(cldev->context,
                                      CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                      len*sizeof(float), pA , &ret);
    if( ret != CL_SUCCESS ){
        printf("err: can not create HOST memory read only\n");
        goto fail;
    }

    cl_mem b_mem_obj = clCreateBuffer(cldev->context,
                                      CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                      len*sizeof(float), pB , &ret);
    if( ret != CL_SUCCESS ){
        printf("err: can not create HOST memory read only\n");
        goto fail;
    }

    cl_mem c_mem_obj = clCreateBuffer(cldev->context, 
                                      CL_MEM_USE_HOST_PTR,
                                      len*sizeof(float), pC , &ret);

    if( ret != CL_SUCCESS ){
        printf("err: can not create HOST memory write only\n");
        goto fail;
    }

    p->push_arg( p , &a_mem_obj , sizeof( cl_mem ) );
    p->push_arg( p , &b_mem_obj , sizeof( cl_mem ) );
    p->push_arg( p , &c_mem_obj , sizeof( cl_mem ) );

    ret = p->enter( p , len );
    if( ret != 0 )
    {
        printf("err: cant execute kernel!\n");
        return TEST_FAIL;
    }

    ret = clEnqueueReadBuffer(cldev->queue, c_mem_obj, CL_TRUE, 0, 
                              len * sizeof(float), pC, 0, NULL, NULL);


    if( (ret != CL_SUCCESS) ||
        (pC[0] != 0.0f ) || (pC[len/2] != 24.0f) || pC[len-1] != 48.0f ){
        printf("err: output buffer is inconsistent\n");
        goto fail;        
    }

    ret = 0;
    ret += clReleaseMemObject(a_mem_obj);
    ret += clReleaseMemObject(b_mem_obj);
    ret += clReleaseMemObject(c_mem_obj);

    if( ret != CL_SUCCESS ){
        printf("err: releasing memory objects\n");
        return TEST_FAIL;
    }

    p->release( p );
    cldev->release( cldev );
    free( pA );
    free( pB );
    free( pC );
    
    return TEST_SUCC;

fail:
    p->release( p );
    cldev->release( cldev );
    free( pA );
    free( pB );
    free( pC );

    return TEST_FAIL;
#else
    printf("skip: OpenCL not compiled in\n");
    return TEST_SKIP;
#endif
}

int (*tests[])() = { test_adaptive_octree,
                     test_fundamentals , test_matrix,
                     test_null_drv,
                     test_sdl_drv , test_raw , test_octree , 
                     test_compilation, test_camera,
                     test_opencl,
                     NULL };

void do_tests(){
    int results[3] = {0,0,0};
    int exec = 0;

    while( tests[exec] ){
        int ret = (tests[exec++])();
        results[ret]++;
    }

    printf("tests executed: %d\npassed: %d\nfailed: %d\nskipped: %d\n",
           exec , results[TEST_SUCC] , results[TEST_FAIL] , 
           results[TEST_SKIP]);
}

int main( int argc , char ** argv ){
    printf("VoXen tests...\n");
    do_tests();

    return 0;
}
