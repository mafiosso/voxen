#include "VX_lib.h"
#include "assert.h"


/* FPU - based stackless traversal */
/* TODO: make static */
/* TEST: successful */
static VX_uint32 oct_addr_stackless_fpu( VX_oct_node * root ,
                                  float hw,
                                  float * addr,
                                  float * cube_point,
                                  int lod )
{
    cube_point[X] = 0.0f; cube_point[Y] = 0.0f; cube_point[Z] = 0.0f;
    cube_point[3] = hw;
    VX_uint32 order;
    register VX_byte * pord = (VX_byte*)&order;

    while( (root->flags != 0) && (lod > 0) )
    {
        hw = hw * 0.5f;
        pord[Z] = (addr[Z] >= (cube_point[Z] + hw));
        pord[Y] = (addr[Y] >= (cube_point[Y] + hw));
        pord[X] = (addr[X] >= (cube_point[X] + hw));

        cube_point[X] += ( pord[X] ? hw : 0.0f );
        cube_point[Y] += ( pord[Y] ? hw : 0.0f );
        cube_point[Z] += ( pord[Z] ? hw : 0.0f );

        root = root->childs[ (pord[Z]<<2) | (pord[Y]<<1) | pord[X] ];
        lod--;

        if( !root ){
            cube_point[3] = hw;
            return 0;
        }
    }

    cube_point[3] = hw;
    return root->color;
}

static inline VX_uint32 oct_step_fpu( VX_oct_node * root ,
                                      float * point_cam,
                                      float * n_ray,
                                      float * point_out ,
                                      float * cube_point,
                                      VX_byte * lperm,
                                      float hw,
                                      int lod )
{
    
    VX_uint32 color =  oct_addr_stackless_fpu( root , 
                                               hw,
                                               point_cam,
                                               cube_point , lod );

    float tp[3] = {cube_point[X] + cube_point[3] ,
                   cube_point[Y] + cube_point[3],
                   cube_point[Z] + cube_point[3] };

    int ret = VX_cube_out_point_fpu( cube_point , tp ,
                                     (VX_fpoint3*)point_cam ,
                                     (VX_fpoint3*)n_ray ,
                                     point_out , lperm );
    
    if( ret < 0 ){ 
        return 0x00ff00ff;
    }

    if( n_ray[ret] < 0.0f ){
        point_out[ret] -= 0.0001f;
    }

    return color;
}

#define POINT_IN_CUBE_FPU_P( C , P )                                    \
    ((P[X] >= C[X] &&( P[X] < C[X] + C[3])) &&                          \
     (P[Y] >= C[Y] && (P[Y] < C[Y] + C[3])) &&                          \
     (P[Z] >= C[Z] && (P[Z] < C[Z] + C[3])))                            \

#define LIGHT_TEST( RAY , DRAY )                                \
    (FLOAT_SIGNUM( (RAY[X]) ) == FLOAT_SIGNUM( (DRAY[X]) )) &&  \
    (FLOAT_SIGNUM( (RAY[Y]) ) == FLOAT_SIGNUM( (DRAY[Y]) )) &&  \
    (FLOAT_SIGNUM( (RAY[Z]) ) == FLOAT_SIGNUM( (DRAY[Z]) ))     \

static VX_uint32 VX_oct_ray_light_rst_fpu( VX_oct_node * root,
                                           float origin[3],
                                           VX_uint32  lcolor,
                                           float      li_pos[3],
                                           float      hw,
                                           VX_byte *  bperm,
                                           float      old_ray[3] )
{
    float ray[3] = { -old_ray[X] , -old_ray[Y] , -old_ray[Z] };
    float dray[3];

    float out[3] = {0,0,0};
    float cube[4] = {0,0,0,hw};
    int stp = 0;

    oct_step_fpu( root ,
                  origin ,
                  ray, out ,
                  cube, bperm,
                  hw, 0xff );

    origin[X] = out[X]; origin[Y] =  out[Y]; origin[Z] = out[Z];

    ray[X] = li_pos[X] - origin[X];
    ray[Y] = li_pos[Y] - origin[Y];
    ray[Z] = li_pos[Z] - origin[Z];

    VX_uint32 line_perm = VX_line_sort( *((VX_fpoint3*)ray) );
    bperm = (VX_byte*)(&line_perm);

    do{
        /* TODO: look for get lod smaller */
        VX_uint32 c = oct_step_fpu( root ,
                                    origin ,
                                    ray, out ,
                                    cube, bperm,
                                    hw, 0xff );
        
        if( c ){ break;}
        if( !(out[X] < hw && out[Y] < hw && out[Z] < hw &&
              out[X] >= 0.0f && out[Y] >= 0.0f && out[Z] >= 0.0f )){
            return lcolor;
        }

        /*if( POINT_IN_CUBE_FPU_P( cube  , ((float*)(&out)) ) ){
            return lcolor;
            }*/
        dray[X] = li_pos[X] - out[X];
        dray[Y] = li_pos[Y] - out[Y];
        dray[Z] = li_pos[Z] - out[Z];

        if( !(LIGHT_TEST( ray , dray ) ) )
            return lcolor;

        stp++;

        origin[X] = out[X]; origin[Y] = out[Y]; origin[Z] = out[Z];
        
    } while( 1 );

    return 0;
}



VX_uint32 VX_oct_ray_voxel_rst_fpu( VX_model * self ,
                                    float * idcam ,
                                    float * n_ray ,
                                    float * out_pt,
                                    int max_lod )
{
    /* TODO: lod support */
    /* implements single ray cast */
    VX_uint32 line_perm = VX_line_sort( *((VX_fpoint3*)n_ray) );
    VX_byte * bperm = (VX_byte*)(&line_perm);

    float hw = (float)self->dim_size.w;

    float dcam[3] = {idcam[X] , idcam[Y] , idcam[Z]};
    float out[3];
    VX_uint32 c;
    float cube[4] = {0,0,0, hw};

    if(  !POINT_IN_CUBE_FPU_P( cube , idcam )  ){
        float * bx = cube;
        float box_max[3] = {bx[X]+bx[3] , bx[Y] + bx[3] , bx[Z] + bx[3]};
        int ret = VX_cube_in_point_fpu( bx , box_max , 
                                        idcam , 
                                        n_ray ,
                                        dcam , bperm );
        
        if( ret < 0 )
            return 0;
    }


    do{
        /* TODO: look for get lod smaller */
        c = oct_step_fpu( self->root ,
                          dcam , n_ray,
                          out ,
                          cube, bperm,
                          hw, max_lod );

        if( c ){ break;}
        if( !(out[X] < hw && out[Y] < hw && out[Z] < hw &&
              out[X] >= 0.0f && out[Y] >= 0.0f && out[Z] >= 0.0f ))
            return 0;

        APPLY3( dcam , out , = );
//        dcam[X] = out[X]; dcam[Y] = out[Y]; dcam[Z] = out[Z];
    } while( 1 );

    int lc = VX_lights_count();
    VX_uint32 lcolor = VX_ambient_light();
    
    while( --lc > -1 ){
        VX_light * l = VX_lights_enumerate();
        float orig[3] = {dcam[X] , dcam[Y] , dcam[Z] };
        VX_uint32 dc = VX_oct_ray_light_rst_fpu( self->root,
                                                 orig,
                                                 l[lc].clr,
                                                 l[lc].pos,
                                                 hw,
                                                 bperm,
                                                 n_ray );
        if( dc ){
            lcolor = color_add( lcolor , l[lc].clr );
        }
    }
    
    return color_mul( c , lcolor );
}
