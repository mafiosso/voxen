#include "VX_lib.h"
#include "SDL/SDL.h"
#include <math.h>

VX_uint32 oct_addr_stackless_fpu( VX_oct_node * root ,
                                  float hw,
                                  float * addr,
                                  float * cube_point,
                                  int lod );

VX_uint32 toGRAY32( VX_format * self , VX_byte * chunk ){
    if( *chunk > 145  ){
        return 0x00000000 | *chunk << 16 | *chunk << 8 | *chunk;
    }

    return self->colorkey;
}

VX_uint32 toARGB32( VX_format * self , VX_byte * chunk ){
    VX_uint32 out = *((VX_uint32*)chunk);
    return out;
}

VX_uint32 toFilteredARGB32( VX_format * self , VX_byte * chunk ){
    VX_uint32 out = *((VX_uint32*)chunk);
    VX_byte gray = chunk[0];
    return ( gray >= 36 ? out : 0 );
}

VX_format FMT_255 = {8 , 1 , 3 , 3 , 2 , 0 , 0x0 , toGRAY32 };
VX_format FMT_ARGB32 = {32,4,8,8,8,8,0, toARGB32 };
VX_format FMT_NMR_FILTER = {32,4,8,8,8,8 ,0 , toFilteredARGB32 };

/* Some basic tests to test 'library' - its not library in fact yet */
void filter_bones( VX_raw * r , VX_byte * px , void * arg )
{
    if( *px < 220 )
        *px = 0;
}

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

    return 1;
}

VX_model * test_octtree_dump( VX_model * skeleton )
{
    VX_model * tree = VX_model_octree_new();
    tree->compile( tree , skeleton );
    tree->inspect( tree );
    printf("dumping: %d\n" , tree->dump( tree , "skeleton2.oct" ) );

    return tree;
}

VX_model * test_octtree_load( const char * path )
{
    VX_model * tree = VX_model_octree_new();
    tree->fmt = &FMT_ARGB32;
    tree->load( tree , path );
    tree->inspect( tree );

    return tree;
}

VX_model * test_octree_simple(){
    VX_model * out = VX_model_octree_new();
    out->fmt = &FMT_ARGB32;
    VX_oct_set_size( out , 1024 );
    VX_oct_fill_region( out , 
                        (VX_ms_block){ 256 , 256, 256 /*, 33 , 33 , 33 } , */, 512 , 512 , 512 },
                        0xff00ff11 );
    out->inspect( out );
    return out;                        
}


VX_model * test_raw_load(){
    VX_model * out = VX_model_raw_new( &FMT_255 , (VX_block){0,0,0, 224 , 256 , 300});
    out->load( out , "skeleton.bin" );
    out->inspect( out );

    return out;
}

VX_model * oct_to_raw(){
    VX_model * oct_myhead = VX_model_octree_new();
    oct_myhead->load( oct_myhead , "myhead.oct" );
    VX_model * raw_myhead = VX_model_raw_new( &FMT_ARGB32 , 
                                              (VX_block){0,0,0,512,512,512} );

    raw_myhead->compile( raw_myhead , oct_myhead );
    int lod = 0xff;

    printf("%u %u\n" , *((VX_uint32*)oct_myhead->get_voxel( oct_myhead , 0,0,0, &lod )),
           *((VX_uint32*)raw_myhead->get_voxel( raw_myhead , 0,0,0,&lod )) );


    verify_compilation( oct_myhead , raw_myhead );


    return raw_myhead;
}

void point_addresing(VX_model * m)
{
    printf("loading\n");

    VX_init( VX_DRV_sdl , 640 , 480 , 0 );
    VX_uint32 lvl = 0;

    int t1 = SDL_GetTicks();
    int lod = 0xf;

    for( int z = 0 ; z < m->dim_size.d ; z++ )
    {
        VX_lib.native_surface->lock_surface( VX_lib.native_surface );

        for( int y = 0 ; y < m->dim_size.h ; y++ )
        {

            for( int x = 0 ; x < m->dim_size.w ; x++ )
            {
                int lod_tmp = lod;
                VX_byte * color_ptr = m->get_voxel( m , x , y , z , &lod_tmp );
                if( !color_ptr ){ continue; }
                VX_uint32 color = m->fmt->ARGB32( m->fmt , color_ptr );
                VX_lib.native_surface->set_pixel( VX_lib.native_surface , x , y  , color );
            }
        }

        VX_lib.native_surface->unlock_surface(VX_lib.native_surface);
        
        VX_lib.redraw( &VX_lib );
    }

    printf("ticks: %d\n" , SDL_GetTicks() - t1 );

    m->free( m );
    VX_lib.quit( &VX_lib );
}

void line_adressing( VX_model * m )
{
    VX_init( VX_DRV_sdl , 640 , 480 , VX_FULLSCREEN );
    printf("CPU cores: %d\n" , VX_lib.cores_count );

    VX_camera * cam = VX_camera_new( VX_lib.native_surface , NULL , -1 );
    if( !cam ){
      printf("err: can not init camera!\n");
      return;
    }

    float angles[3] = {0.0f , 0.0f , 0.0f};
    const float rot_step = 0.1f;
    float tsin[3];
    float tcos[3];

    double rot_m[4][4];
    VX_matrix4_identity( rot_m );

    cam->position.x = 87;
    cam->position.y = 26;
    cam->position.z = -20;

    m->inspect(m);

    #define STEP 8.0f

    int i = 10;

    SDL_Event keyevent;
    int end_p = 0;
    VX_uint32 step = 4;

    VX_ambient_light_set( 0xffffffff );

    
    VX_byte ambient_color[4] = {85,85,85,85};
      VX_light dl = { {1,1,1} , 0xffffffff };
    //654 611 91 voxlap
    //494 489 112 voxelstein
    VX_light d2l = { { 494.5f , 489.5f , 112.5f } , 0xffffff00 };
    int idx = VX_light_dynamic_add( &dl );
    int idx2 = VX_light_dynamic_add( &d2l );
    printf("idxs %d %d\n" , idx , idx2 );
    printf("%u\n" , VX_lights_enumerate()[0].clr);
    printf("lghts count: %d\n" , VX_lights_count() );
    PRINT_RAW_FPOINT3( VX_lights_enumerate()[1].pos );
    
    int lod = 10;
    //310 574 127
    m->get_voxel( m , 310 , 574 , 128 , &lod );
    printf("lod: %d\n" , lod );
    
    double lght_rot_m[4][4];
    float l_vect [4] = { -127 , 0 , 0, 0 };
    float l_vect_out [4];
    double angle = 0;

    while (!end_p/* && step--*/ )
    {
        /*      if( ambient_color[2] ){
            ambient_color[2]--;
        }
        else{
            ambient_color[2] = 255;
            }*/

        VX_matrix4_yrotation( lght_rot_m , angle );
        VX_fvector4_multiply( l_vect_out , lght_rot_m , l_vect );

        angle += 0.1;

        VX_ambient_light_set( ambient_color );
        VX_ambient_light_set( *((VX_uint32*)ambient_color) );
        VX_light * ls = VX_lights_enumerate();
        ls[idx].pos[X] = l_vect_out[X] + 128;
        ls[idx].pos[Y] = l_vect_out[Y] + 128;
        ls[idx].pos[Z] = l_vect_out[Z] + 48;

        float mul = 1.0f;
        SDL_PollEvent( &keyevent );

        int ticks = SDL_GetTicks();
        float direction_vect[4] = {0,0,0,0};
        
        if( keyevent.type == SDL_KEYDOWN && 
            keyevent.key.keysym.sym == SDLK_LSHIFT ){
            mul = -1.0f;
        }
        
        switch(keyevent.type){
        case SDL_KEYDOWN:
            switch(keyevent.key.keysym.sym){
            case SDLK_a:
	      //cam->position.x -= STEP;
	      direction_vect[X] = -STEP;
                break;
            case SDLK_d:
	      direction_vect[X] = STEP;
                break;
            case SDLK_q:
	      //cam->position.y += STEP;
	      direction_vect[Y] = STEP;
                break;
            case SDLK_y:
	      //                cam->position.y -= STEP;
	      direction_vect[Y] = -STEP;
                break;
            case SDLK_ESCAPE:
                end_p = 1;
                break;
            case SDLK_w:
	      // cam->position.z += STEP;
	      direction_vect[Z] = STEP;
                break;
            case SDLK_s:
	      //cam->position.z -= STEP;
	      direction_vect[Z] = -STEP;
                break;
             case SDLK_RIGHT:
	       angles[Y] -= rot_step;
	       break;
            case SDLK_UP:
	      angles[X] -= rot_step;
	      break;
	     case SDLK_LEFT:
	       angles[Y] += rot_step;
	       break;
            case SDLK_DOWN:
	      angles[X] += rot_step;
	      break;
            case SDLK_o:
                //ls[1].pos[Z] += 2;
                break;
            case SDLK_l:
                //ls[1].pos[Z] -= 2;
                break;

            default:
                break;
            }
        }
        /* recompute cos and sin */
        for(int j = 0 ; j < 3 ; j++ ){
            tsin[j] = sin( angles[j] );
            tcos[j] = cos( angles[j] );
        }

        /* recompute rotation matrix */
        double rmx[4][4];
        double rmy[4][4];
        double rmz[4][4];
        double tmp[4][4];
        
        VX_matrix4_xrotation( rmx , angles[X] );
        VX_matrix4_yrotation( rmy , angles[Y] );
        VX_matrix4_zrotation( rmz , 0 );

/*        
        float u_axis[4] = { 0,0,1,0};
        float r_axis[4];
        float rot_axis[4];
        double rot_m[4][4];
        float yax[3] = {0,1.0f,0};
        
        VX_fvector4_multiply( r_axis , rmy , u_axis );
        
        CROSS_PRODUCT( rot_axis , yax , r_axis );  
        PRINT_RAW_FPOINT3( rot_axis );
        
        fvector3_normalize( rot_axis , 1.0f );
        
        VX_matrix4_rotate( cam->rotation_matrix , rot_axis , angles[X] );
*/      
        
        VX_matrix4_multiply( tmp , rmy , rmz );
        VX_matrix4_multiply( cam->rotation_matrix , rmx , rmy );

        float dvect[3];
        VX_fvector4_multiply( dvect , cam->rotation_matrix , direction_vect );

        float at[3] = {0,0,1};
        float up[3] = {0,1,0};
        //VX_matrix4_lookat( cam->rotation_matrix , at , up );
        
        cam->position.x += dvect[X];
        cam->position.y += dvect[Y];
        cam->position.z += dvect[Z];
        
        printf("%d %d %d\n" , cam->position.x ,
               cam->position.y , cam->position.z );
        
        cam->draw( cam , m );
        printf("took %d ms\n" , SDL_GetTicks() - ticks );
    }
    
    m->free( m );
    VX_lib.quit( &VX_lib );
}

VX_iline3 inverse_line3( VX_iline3 in ){
    return (VX_iline3){ in.p2 , in.p1 };
}

void matrix_test()
{
    double tmp[4][4];
    double rot[4][4];
    double id[4][4];
    float  vct[4] = {0.0f , 0.0f , 1.0f , 0.0f};
    float  vout[4];

    VX_matrix4_identity( tmp );
    VX_matrix4_identity( id );

    VX_matrix4_print( tmp );

    printf(">>>> \n");

    VX_matrix4_xrotation( rot , deg_to_radians( 90 ) );
    VX_matrix4_multiply( tmp , id , id );

    VX_fvector4_multiply( vout , rot , vct );

    VX_vector4_print( vout );

    VX_matrix4_print( tmp );

    double m [4][4];
    float rv[3] = { 1 , 0 , 0 };

    printf("\n\n");

    VX_matrix4_rotate( m , rv , deg_to_radians( 90 ) );

    VX_matrix4_print(m);
}

int verify_compilation( VX_model * omodel , VX_model * rmodel){
    int lod = 0xff;
    printf("test: octree compilation: ");

    for( int z = 0; z < 224  ; z++ ){
        for( int y = 0; y < 256 ; y++ ){
            for( int x = 0 ; x < 300 ; x++ ){
                lod = 0xff;
                VX_byte* pix1 = omodel->get_voxel( omodel , x,y,z , &lod);
                lod = 0xff;
                VX_byte* pix2 = rmodel->get_voxel( rmodel , x,y,z , &lod);

                if( !pix1 || !pix2 )
                    continue;

                float addr[3] = {x,y,z};

                if( omodel->fmt->ARGB32( omodel->fmt , pix1 ) !=
                    rmodel->fmt->ARGB32( rmodel->fmt , pix2 ))
                {
                    printf("colors differ: %u != %u\n" , 
                           omodel->fmt->ARGB32( omodel->fmt , pix1 ) ,
                           rmodel->fmt->ARGB32( rmodel->fmt  , pix2 ) );
                    printf("verification failed at point: p(%d %d %d)\n" , x , y, z );

                    return 0;
                }
            }
        }
    }

    printf("success\n");
    return 1;
}

unsigned int test_line_addressing( ){
    VX_model * m = test_octree_simple();

    VX_fpoint3 position = {35.0f,32.5f,32.0f};
    VX_fpoint3 ray = {-position.x + 32.5f, -position.y + 32.5f ,
		      -position.z + 33.0f};

    PRINT_POINT3( position );

    PRINT_POINT3( ray );
    

    VX_uint32 pix = m->ray_voxel( m , (float*)&position , (float*)&ray ,
                                  NULL,  0xA );

    return pix;
}

unsigned int test_backtrack( VX_model * m ){

    return 1;
}

VX_model * test_vxl_load( const char * path ){
    VX_model * out = VX_model_raw_new( &FMT_ARGB32 , 
                                       (VX_block){0,0,0, 1024 , 1024 , 256});

/*    VX_model * out = VX_model_octree_new();
      VX_oct_set_size( out , 1024 );*/

    /* hm...this is huge:/ -> 1GB! model */
    if( VX_vxlfmt_load( out , path ) < 0 ){
        out->free( out );
        return NULL;
    }

    printf("LOADED\n");

    VX_model * tree = VX_model_octree_new();
    tree->compile( tree , out );
    tree->inspect( tree );
    
    out->free( out );

    printf("COMPILED\n");

    char outp[ strlen(path) + 1 + strlen( ".vxl" ) ];
    strcpy( outp , path );
    strcat( outp , ".vxl" );

    tree->dump(tree , outp);

    return tree;
}

void manual_stepping( VX_model * m ){
    
}

/* One time used script */
VX_model * myhead_convert(  const char * fmt ,
                            int slices_count ,
                            const char * out_path ){
    char path [strlen(fmt) + strlen("000") + strlen( ".bmp" ) + 1];
    VX_model * r = VX_model_raw_new( &FMT_NMR_FILTER , 
                                     (VX_block){ 0,0,0, 
                                             512 , 512 ,
                                             512 } );

    int model_z = 0;
    int xtra_offset = 128;
    


    for( VX_uint32 z = 0 ; z < slices_count ; z++ ){
        char number_tmp[4];
        char number_out[] = "000";
        sprintf(number_tmp , "%u" , z );
        int len = strlen( number_tmp );
        memcpy( number_out + (3-len) , number_tmp , len );
        sprintf(path , fmt  , number_out );
        printf("using file: %s\n" , path );

        SDL_Surface * s = SDL_LoadBMP( path );
        if( !s ){
            printf("err: no such file\n");
            r->free(r);
            return NULL;
        }
        printf("surface info: %d %d bpp %d\n" , s->w , s->h , s->format->BytesPerPixel );
        VX_byte bytespp = s->format->BytesPerPixel;
        int zcorrection = (z%2) + 1;

        for( int y = 0 ; y < s->h ; y++ ){
            for( int x = 0 ; x < s->w ; x++ ){
                VX_uint32 color = 0;
                VX_byte * a = s->pixels;
                a += (y * s->pitch) + (x * bytespp);

                memcpy( &color , 
                        a,
                        bytespp );

                if( z <= 13 || z >=142 ){
                    #define THR 100
                    /* apply more agressive noise filter */
                    VX_byte cb = (a[0] <= THR ? 0 : a[0] - THR);
                    color = 0;
                    color = cb << 16 | cb << 8 | cb ;
                    
                }

                for( int mz = 0 ; mz < zcorrection  ; mz++ ){
                    r->set_voxel( r , x+xtra_offset , y+xtra_offset ,
                                  mz + model_z,
                                  color );
                }


            }
        }

        model_z += zcorrection;
        SDL_FreeSurface( s );
    }

    VX_model * oct = VX_model_octree_new();
    oct->compile( oct , r );
    oct->dump( oct , out_path );

    return oct;
}

char * read_txt( const char * path ){
    FILE * f;
    if( !( f = fopen( path , "rb" ) ) ){
        return NULL;
    }

    fseek( f , 0 , SEEK_END );
    size_t len = ftell( f );
    rewind( f );
    
    char * out = malloc( len + 1 );
    out[ len ] = '\0';

    if( !fread( out , sizeof( VX_byte ) , len , f ) )
    {
        free( out );
        out = NULL;
    }

    fclose( f );
    return out;
}

int test_ocl(){
    VX_CL_device * cldev = VX_CL_device_new();

    if( cldev ){
        printf("great we have opencl support now\n");
    }

    char * program = read_txt( "./CL_code/vector_add.cl" );
    char * srcs [2] = { program , NULL };

    printf("src: %s\n" , program );

    VX_CL_program * clprog = VX_CL_program_new( cldev , srcs , "vector_add" );
    VX_CL_program * p = clprog;

    if( clprog ){ printf("hurray we have program compiled well!\n"); }

    size_t len = 1024*1024*128;

    float *A = malloc(len*sizeof(float));
    float *B = malloc(len*sizeof(float));
    float *C = malloc(len*sizeof(float));

    C[0] = 3.14f;
    C[len/2] = 1988.0f;
    C[len-1] = 13.0f;

    for( VX_uint32 i = 0 ; i < len ; i++ ){
        A[i] = (float)i;
        B[i] = A[i];
    }

    cl_int ret;

    cl_mem a_mem_obj = clCreateBuffer(cldev->context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, len*sizeof(float), A , &ret);
    printf("ret: %d\n" , ret == CL_SUCCESS );

    cl_mem b_mem_obj = clCreateBuffer(cldev->context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, len*sizeof(float), B , &ret);

    printf("ret: %d\n" , ret == CL_SUCCESS );
    cl_mem c_mem_obj = clCreateBuffer(cldev->context, CL_MEM_WRITE_ONLY , len*sizeof(float), NULL , &ret);
    printf("ret: %d\n" , ret == CL_SUCCESS );

    VX_uint32 tck = SDL_GetTicks();

    /* copy buffers to gpu */
/*    ret = clEnqueueWriteBuffer(cldev->queue, a_mem_obj, CL_TRUE, 0,
            len * sizeof(float), A, 0, NULL, NULL);
    ret = clEnqueueWriteBuffer(cldev->queue, b_mem_obj, CL_TRUE, 0, 
            len * sizeof(float), B, 0, NULL, NULL);
*/

    printf("uploading taken %d ms\n" , SDL_GetTicks() - tck );
    p->push_arg( p , &a_mem_obj , sizeof( cl_mem ) );
    p->push_arg( p , &b_mem_obj , sizeof( cl_mem ) );
    p->push_arg( p , &c_mem_obj , sizeof( cl_mem ) );

    for(int i = 0; i < 24 ; i++){
        tck = SDL_GetTicks();

        p->enter( p , len );

        printf("(%u) calling taken %d ms\n", i , SDL_GetTicks() -tck );
        
    }

    ret = clEnqueueReadBuffer(cldev->queue, c_mem_obj, CL_TRUE, 0, 
                              len * sizeof(float), C, 0, NULL, NULL);


    printf("%d %f %f %f\n" , ret , C[0] , C [len/2] , C[len-1] );

    for(int i = 0 ; i < 24 ; i++ ){
        tck = SDL_GetTicks();
        for( int j = 0 ; j < len ; j++ ){
            C[j] = A[j]  + B[j];
        }
        printf("cpu taken: %d\n" , SDL_GetTicks() - tck );

    }

    return 0;

}

int edge_case_p( VX_int32 * box_min , VX_int32 * box_max , VX_int32 * point ){
    int out = 0;
    out += box_min[X] == point[X];
    out += box_min[Y] == point[Y];
    out += box_min[Z] == point[Z];

    out += box_max[X] == point[X];
    out += box_max[Y] == point[Y];
    out += box_max[Z] == point[Z];

    return out > 1;
}

void test_weird(){
    /*
      inspect b
      $1 = {x = 0, y = 256, z = 0, w = 256}
      (gdb) inspect tmp
      $2 = {156.104706, 256, 256}
      (gdb) inspect dcam
      $3 = {x = 156.104706, y = 256, z = 256}
      (gdb) inspect n_ray 
      $4 = {x = -0.124427781, y = -0.694489181, z = -0.70866245}
      (gdb) inspect bperm 
      $5 = (uint8_t *) 0xbfffed1c ""
      (gdb) inspect *bperm
      $6 = 0 '\000'
      (gdb) inspect tp
      $7 = {x = 256, y = 512, z = 256}
      (gdb) inspect iw
      $8 = 1024
      (gdb) inspect ip
      $9 = {156, 255, 256}
      (gdb) inspect ret
      $10 = 1 '\001'
     */

    VX_ipoint3 bmin = {0, 0, 256};
    VX_ipoint3 bmax = {256,256,512};
    VX_fpoint3 dcam = {156.104706,  256, 256};
    VX_fpoint3 n_ray = {-0.124427781, -0.694489181, -0.70866245};
    VX_uint32 line_perm = VX_line_sort( n_ray );
    VX_byte * bperm = (VX_byte*)(&line_perm);
    float tmp[3];


    int ret = VX_cube_out_point( (int*)(&bmin) , (int*)&bmax ,
                                 &dcam , &n_ray , tmp ,
                                 bperm );

    printf("weird case\n");
    VX_permutation_inspect( line_perm );
    PRINT_POINT3( (*((VX_fpoint3*)tmp)) );
    printf("ret: %d\n" , ret);

    printf("traversal:...\n");
    VX_model * m = test_octree_simple();
    VX_cube b;
    int lod_tmp = 0xff;
    VX_ipoint3 p = {156, 256, 256};
    printf("edge point?: %d\n" , edge_case_p( (int*)&bmin ,
                                              (int*)&bmax ,
                                              (int*)&p ) );

    VX_uint32 pixel = oct_get_vox_stackless( m->root , 1024,
                                             156,
                                             255,
                                             256,
                                             &b,
                                             &lod_tmp );

    

    PRINT_CUBE( b );
}

int test_slab_fpu(){
    float lcube [4] = {0,0,0, 512.0f};
    float tp[3] = {512 , 512 , 512};
    float cam[3] = {128, 128 , -25};
    float ray[3] = {1.0f,0,0.0f};
    float out[3];
    VX_uint32 perm =  VX_line_sort( *((VX_fpoint3*)ray) );

    int ret = VX_cube_out_point_fpu( lcube , tp ,
                                     cam ,
                                     ray ,
                                     out , &perm );
    
    printf( "ret: %d with out <- <%f , %f , %f>\n" , 
            ret , out[X] , out[Y] , out[Z] );
    
}

int main( int argc , char * argv[] )
{
    VX_model * m = NULL;
    /*m = test_octtree_dump( test_raw_load() );	*/

    /*
      matrix_test();
    */
    int box_min[] = {31,31,31};
    int box_max[] = {64,64,64};
    VX_fpoint3 cam_pos = { 48 , 168 , 31};
    VX_fpoint3 dir = {10.0f, 0.0f ,10.0f};
    int d = 0;
    float out[3];
    VX_uint32 perm = VX_line_sort( dir );
    char * path = (argc > 1 ? argv[1] : "skeleton.oct" );

//    printf("fundamentals test: %d\n" , test_fundamentals());

//    VX_permutation_inspect( perm );
//    printf("ret %d\n" ,
//           VX_cube_out_point( box_min , box_max , &cam_pos , &dir , out , (VX_byte*)&perm ));
//    PRINT_POINT3( (*((VX_fpoint3*)out)) );

//    printf("verification ret: %d\n" , verify_compilation());

    //point_addresing( test_raw_load() /*test_octree_simple()*//*test_octtree_load( path )*/);

    //m = test_vxl_load( (argv > 1 ? argv[1] : "test.vxl" ) );
//    m = test_octtree_load( path );
    m = oct_to_raw();
    //m = myhead_convert( "./nmr_downscaled/image%s.bmp" , 160 , "myhead.oct" );
    //m = test_octree_simple();

    //m = test_raw_load();
    // test_ocl();

    matrix_test();

    //test_weird();
    
//test_line_addressing();

//    test_slab_fpu();
    
    line_adressing( m );
    //point_addresing( m );
    //raw_vs_oct();

//    test_backtrack( m );


//    printf("test: addressing: %u\n" , test_line_addressing() );

    return 0;
}
