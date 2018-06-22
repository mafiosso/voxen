#include "VX_lib.h"
#include "assert.h"

#define POINT_IN_CUBE_FPU_P( C , P )                                    \
    ((P[X] >= C[X] &&( P[X] < C[X] + C[3])) &&                          \
     (P[Y] >= C[Y] && (P[Y] < C[Y] + C[3])) &&                          \
     (P[Z] >= C[Z] && (P[Z] < C[Z] + C[3])))                            \

typedef struct oct_stack_item{
    float cube_point[4];
    VX_oct_node * n;
}oct_stack_item;

static int _min_length = 0;

static VX_uint32 oct_addr_sstack_fpu( oct_stack_item si[] , 
                                      VX_int32 * lvl,
                                      float addr[3],
                                      int lod)
{
    int idx = *lvl;
    idx--;

    while( (idx >= _min_length) && 
           !(POINT_IN_CUBE_FPU_P( si[idx].cube_point , addr )))
    {
        idx--;
    }

    if( idx < _min_length ){
        idx = 0;
    }

    float cube_point[4] = {si[idx].cube_point[X],
                           si[idx].cube_point[Y],
                           si[idx].cube_point[Z],
                           si[idx].cube_point[3]};
    float hw = cube_point[3];

    VX_uint32 order;
    VX_oct_node * root = si[idx].n;

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
        cube_point[3] = hw;

        root = root->childs[ (pord[Z]<<2) | (pord[Y]<<1) | pord[X] ];
        lod--;

        idx++;
        oct_stack_item sit = {{cube_point[X] , cube_point[Y] , cube_point[Z] ,
                              hw} , root };
        si[idx] = sit;

        if( !root ){
            *lvl = idx;
            return 0;
        }
    }

    *lvl = idx;
    return root->color;
}

static inline VX_uint32 oct_step_fpu( float * point_cam,
                                      float * n_ray,
                                      float * point_out ,
                                      VX_byte * lperm,
                                      oct_stack_item * si,
                                      int * level,
                                      int lod )
{
    
    VX_uint32 color =  oct_addr_sstack_fpu(  si ,
                                             level ,
                                             point_cam,
                                             lod);

    float cube_point[4];
    memcpy( cube_point , &(si[*level].cube_point) , sizeof(float)*4 );

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


#define LIGHT_TEST( RAY , DRAY )                                \
    (FLOAT_SIGNUM( (RAY[X]) ) == FLOAT_SIGNUM( (DRAY[X]) )) &&  \
    (FLOAT_SIGNUM( (RAY[Y]) ) == FLOAT_SIGNUM( (DRAY[Y]) )) &&  \
    (FLOAT_SIGNUM( (RAY[Z]) ) == FLOAT_SIGNUM( (DRAY[Z]) ))     \

static VX_uint32 VX_oct_ray_light_sstack_fpu( float origin[3],
                                              VX_uint32  lcolor,
                                              float      li_pos[3],
                                              VX_byte *  bperm,
                                              float      old_ray[3],
                                              oct_stack_item * s ,
                                              int * level )
{
    float ray[3] = { -old_ray[X] , -old_ray[Y] , -old_ray[Z] };
    float dray[3];
    float hw = s[0].cube_point[3];

    float out[3] = {0,0,0};
    int stp = 0;

    oct_step_fpu( origin ,
                  ray, out ,
                  bperm,
                  s,
                  level,
                  0xff );

    origin[X] = out[X]; origin[Y] =  out[Y]; origin[Z] = out[Z];

    ray[X] = li_pos[X] - origin[X];
    ray[Y] = li_pos[Y] - origin[Y];
    ray[Z] = li_pos[Z] - origin[Z];

    VX_uint32 line_perm = VX_line_sort( *((VX_fpoint3*)ray) );
    bperm = (VX_byte*)(&line_perm);

    do{
        /* TODO: look for get lod smaller */
        VX_uint32 c = oct_step_fpu( origin ,
                                    ray, out ,
                                    bperm,
                                    s,
                                    level,
                                    0xff );
        
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



VX_uint32 VX_oct_ray_voxel_bcktrk_fpu( VX_model * self ,
                                       float * idcam ,
                                       float * n_ray ,
                                       float * out_pt,
                                       int max_lod )
{
    oct_stack_item stack[((VX_oct_info*)self->data)->stack_len];
    int level = 0;
    float hw = (float)self->dim_size.w;
    stack[0] = (oct_stack_item){{0,0,0,hw} , self->root };
    float dcam[3] = {idcam[X] , idcam[Y] , idcam[Z]};


    VX_uint32 line_perm = VX_line_sort( *((VX_fpoint3*)n_ray) );
    VX_byte * bperm = (VX_byte*)(&line_perm);

    if(  !POINT_IN_CUBE_FPU_P( stack[0].cube_point , idcam )  ){
        float * sbx = stack[0].cube_point;
        float bx [4] = {sbx[X] , sbx[Y] , sbx[Z] , sbx[3]};
        float box_max[3] = {bx[X]+bx[3] , bx[Y] + bx[3] , bx[Z] + bx[3]};
        int ret = VX_cube_in_point_fpu( bx , box_max , 
                                        idcam , 
                                        n_ray ,
                                        dcam , bperm );

          if( ret < 0 )
          return 0;
    }

    _min_length = 0;
    float out[3];
    VX_uint32 c;

    do{        
        /* TODO: look for get lod smaller */
        c = oct_step_fpu( dcam, n_ray, out ,
                          bperm, stack ,
                          &level, max_lod );

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
        VX_uint32 dc = VX_oct_ray_light_sstack_fpu( orig,
                                                    l[lc].clr,
                                                    l[lc].pos,
                                                    bperm,
                                                    n_ray,
                                                    stack,
                                                    &level );
        if( dc ){
            lcolor = color_add( lcolor , l[lc].clr );
        }
    }

    return color_mul( c , lcolor );
}


VX_uint32 VX_oct_ray_voxel_sstack_fpu( VX_model * self ,
                                       float * idcam ,
                                       float * n_ray ,
                                       float * out_pt,
                                       int max_lod )
{
    int stack_len = ((VX_oct_info*)self->data)->stack_len;
    oct_stack_item stack[stack_len];
    int level = 0;
    float hw = (float)self->dim_size.w;
    stack[0] = (oct_stack_item){{0,0,0,hw} , self->root };
    float dcam[3] = {idcam[X] , idcam[Y] , idcam[Z]};

    VX_uint32 line_perm = VX_line_sort( *((VX_fpoint3*)n_ray) );
    VX_byte * bperm = (VX_byte*)(&line_perm);

    if(  !POINT_IN_CUBE_FPU_P( stack[0].cube_point , idcam )  ){
        float * sbx = stack[0].cube_point;
        float bx [4] = {sbx[X] , sbx[Y] , sbx[Z] , sbx[3]};
        float box_max[3] = {bx[X]+bx[3] , bx[Y] + bx[3] , bx[Z] + bx[3]};
        int ret = VX_cube_in_point_fpu( bx , box_max , 
                                        idcam , 
                                        n_ray ,
                                        dcam , bperm );

          if( ret < 0 )
          return 0;
    }

    _min_length = stack_len >> 1;
    float out[3];
    VX_uint32 c;

    do{        
        c = oct_step_fpu( dcam, n_ray, out ,
                          bperm, stack ,
                          &level, max_lod );

        if( c ){ break;}
        if( !(out[X] < hw && out[Y] < hw && out[Z] < hw &&
              out[X] >= 0.0f && out[Y] >= 0.0f && out[Z] >= 0.0f ))
            return 0;

        APPLY3( dcam , out , = );
    } while( 1 );

    int lc = VX_lights_count();
    VX_uint32 lcolor = VX_ambient_light();
    
    while( --lc > -1 ){
        VX_light * l = VX_lights_enumerate();
        float orig[3] = {dcam[X] , dcam[Y] , dcam[Z] };
        VX_uint32 dc = VX_oct_ray_light_sstack_fpu( orig,
                                                    l[lc].clr,
                                                    l[lc].pos,
                                                    bperm,
                                                    n_ray,
                                                    stack,
                                                    &level );
        if( dc ){
            lcolor = color_add( lcolor , l[lc].clr );
        }
    }

    return color_mul( c , lcolor );
}
