#include "VX_lib.h"

#define AOCT_EMPTY 0
#define AOCT_RAW   1
#define AOCT_NODE  2

#define POINT_IN_CUBE_FPU_P( C , P )                                    \
    ((P[X] >= C[X] &&( P[X] < C[X] + C[3])) &&                          \
     (P[Y] >= C[Y] && (P[Y] < C[Y] + C[3])) &&                          \
     (P[Z] >= C[Z] && (P[Z] < C[Z] + C[3])))                            \


static void * ao_addr( VX_oct_node * root ,
                          float hw,
                          float * addr,
                          float * cube_point,
                          VX_uint32 * type )
{
    cube_point[X] = 0.0f; cube_point[Y] = 0.0f; cube_point[Z] = 0.0f;
    cube_point[3] = hw;
    VX_uint32 order;
    register VX_byte * pord = (VX_byte*)&order;

    while( (root->flags != 0) )
    {
        if( (root->flags & (1 << 31) ) ){
            /* its a raw subnode */
            *type = AOCT_RAW;
            cube_point[3] = hw;
            return root;
        }

        hw = hw * 0.5f;
        pord[Z] = (addr[Z] >= (cube_point[Z] + hw));
        pord[Y] = (addr[Y] >= (cube_point[Y] + hw));
        pord[X] = (addr[X] >= (cube_point[X] + hw));

        cube_point[X] += ( pord[X] ? hw : 0.0f );
        cube_point[Y] += ( pord[Y] ? hw : 0.0f );
        cube_point[Z] += ( pord[Z] ? hw : 0.0f );

        root = root->childs[ (pord[Z]<<2) | (pord[Y]<<1) | pord[X] ];

        if( !root ){
            cube_point[3] = hw;
            *type = AOCT_EMPTY;
            return NULL;
        }
    }

    cube_point[3] = hw;
    *type = AOCT_NODE;
    return root;
}

static inline VX_uint32 oct_step_fpu( VX_aoct_node * node ,
                                      float * point_cam,
                                      float * n_ray,
                                      float * point_out ,
                                      float * cube_point,
                                      VX_byte * lperm)
{
    float tp[3] = {cube_point[X] + cube_point[3] ,
                   cube_point[Y] + cube_point[3] ,
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

    return (node && node->color) || 0x0;
}

static VX_uint32 raw_step( VX_araw_node * node , VX_model * model,
                           float * bbox ,
                           float * dcam , float * out ){
    /* TODO: make better grid traversal */
    VX_uint32 sz = bbox[3];

    float p[3] = {dcam[X], dcam[Y], dcam[Z]};
    
    float xlen = p[X] - out[X];
    float ylen = p[Y] - out[Y];
    float zlen = p[Z] - out[Z];

    float len = MAX( fabs(xlen) , MAX( fabs(ylen) , fabs(zlen) ) );

    float stepx = xlen / len;
    float stepy = ylen / len;
    float stepz = zlen / len;
    VX_uint32 powsz = sz*sz;
    VX_uint32 clr;
    VX_uint32 lod = 1000;
    for( int i = 0 ; POINT_IN_CUBE_FPU_P( bbox , p ) ; i++ ){
        VX_byte * rclr = model->get_voxel(model, p[X], p[Y], p[Z], &lod);

        clr = model->fmt->ARGB32( model->fmt, rclr );

        if( (clr & 0x00FFFFFF) ){
            goto end;
        }
        p[X] -= stepx;
        p[Y] -= stepy;
        p[Z] -= stepz;
    }

    clr = 0xFF000000;

  end:    
    APPLY3( out, p, = );

    return clr;    
}

VX_uint32 VX_ao_ray( VX_model * self ,
                     float * idcam ,
                     float * n_ray ,
                     float * out_pt,
                     int max_lod )
{
    VX_uint32 line_perm = VX_line_sort( *((VX_fpoint3*)n_ray) );
    VX_byte * bperm = (VX_byte*)(&line_perm);
    
    float hw = (float)self->dim_size.w;

    float dcam[3] = {idcam[X] , idcam[Y] , idcam[Z]};
    float out[3];
    VX_uint32 c = 0x0;
    float cube[4] = {0,0,0, hw};

    if( !POINT_IN_CUBE_FPU_P( cube , idcam ) ){
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
        VX_uint32 type;
        void * node = ao_addr( self->root , hw,
                               dcam ,cube, &type );

        if( type == AOCT_RAW ){
            /* Do grid traversal */
            //return 0x0000ff00;
            APPLY3( out , n_ray , += );
            c = raw_step( node, self,
                          cube,
                          dcam,
                          out);
            //return 0x0000ff00;
        }
        else if( type == AOCT_EMPTY ){
            /* do octree step */
            c = oct_step_fpu( (VX_aoct_node*)node ,
                          dcam,
                          n_ray,
                          out,
                          cube,
                          bperm);
        }
        else if( type == AOCT_NODE ){
            c = ((VX_aoct_node*)node)->color;
        }
        
        if( c & 0x00FFFFFF ){ break;} /* mask causes endless looping */
        if( !(out[X] < hw && out[Y] < hw && out[Z] < hw &&
              out[X] >= 0.0f && out[Y] >= 0.0f && out[Z] >= 0.0f ))
            return 0;
        
        APPLY3( dcam , out , = );
    } while( 1 );
}
