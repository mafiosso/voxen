#include "VX_lib.h"

#define VISUALIZE_COMPLEXITY

#define AOCT_EMPTY 0
#define AOCT_RAW   1
#define AOCT_NODE  2

#define POINT_IN_CUBE_FPU_P( C , P )                                    \
    ((P[X] >= C[X] && (P[X] < C[X] + C[W])) &&                          \
     (P[Y] >= C[Y] && (P[Y] < C[Y] + C[W])) &&                          \
     (P[Z] >= C[Z] && (P[Z] < C[Z] + C[W])))                            \


static void * ao_addr( VX_oct_node * root ,
                       float * addr,
                       float * cube_point,
                       VX_uint32 * type,
                       VX_uint32 * color )
{
    VX_uint32 order;
    register VX_byte * pord = (VX_byte*)&order;

    while ((root->flags != 0))
    {
        if( (root->flags & (1 << 31) ) ){
            /* its a raw subnode */
            *type = AOCT_RAW;
            return root;
        }

        float w = cube_point[W] * 0.5f;

        pord[Z] = (addr[Z] >= (cube_point[Z] + w));
        pord[Y] = (addr[Y] >= (cube_point[Y] + w));
        pord[X] = (addr[X] >= (cube_point[X] + w));

        cube_point[X] += ( pord[X] ? w : 0.0f );
        cube_point[Y] += ( pord[Y] ? w : 0.0f );
        cube_point[Z] += ( pord[Z] ? w : 0.0f );
        cube_point[W] = w;

        root = root->childs[ (pord[Z]<<2) | (pord[Y]<<1) | pord[X] ];

        if( !root ){
            *type = AOCT_EMPTY;
            return NULL;
        }

        #ifdef VISUALIZE_COMPLEXITY
        const VX_uint32 cstep = 1;
        *color += ((*color < 0x00FF) ? cstep
                : ((*color < 0xFF00) ? cstep << 8
                : cstep << 16));
        if (*color > 0x00FFFFFF) {
            *color = 0;
        }
        #endif
    }

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
    float tp[W] = {cube_point[X] + cube_point[W] ,
                   cube_point[Y] + cube_point[W] ,
                   cube_point[Z] + cube_point[W] };

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

typedef struct raw_params {
    float stepx;
    float stepy;
    float stepz;
} raw_params;

static VX_uint32 raw_step( VX_araw_node * node ,
                           float * bbox , raw_params * rp,
                           float * p ){
    /* TODO: make better grid traversal */
    VX_uint32 sz = bbox[W];
    VX_uint32 powsz = sz*sz;
    VX_uint32 pow3sz = powsz * sz;

    VX_uint32 clr;

    while( POINT_IN_CUBE_FPU_P( bbox , p ) ){
        VX_uint32 idx = (powsz*(VX_uint32)(p[Z] - bbox[Z]))
            + (sz*(VX_uint32)(p[Y] - bbox[Y]))
            + (VX_uint32)(p[X] - bbox[X]);

        clr = *(((VX_uint32*)node) + idx +1);

        p[X] += rp->stepx;
        p[Y] += rp->stepy;
        p[Z] += rp->stepz;

        if( (clr & 0x00FFFFFF) ){
            goto end;
        }
    }

    clr = 0xFF000000;

  end:
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

    float dcam[W] = {idcam[X] , idcam[Y] , idcam[Z]};
    float out[W];
    VX_uint32 c = 0x0;
    float cube[4] = {0,0,0, hw};

    if( !POINT_IN_CUBE_FPU_P( cube , idcam ) ){
        float * bx = cube;
        float box_max[W] = {bx[X]+bx[W] , bx[Y] + bx[W] , bx[Z] + bx[W]};
        int ret = VX_cube_in_point_fpu( bx , box_max ,
                                        idcam ,
                                        n_ray ,
                                        dcam , bperm );

        if( ret < 0 )
            return 0;
    }

    APPLY3( out , dcam , = );

    float len = MAX( fabs(n_ray[X]) , MAX( fabs(n_ray[Y]) , fabs(n_ray[Z]) ) );

    float stepx = n_ray[X] / len;
    float stepy = n_ray[Y] / len;
    float stepz = n_ray[Z] / len;

    raw_params rp = {stepx, stepy, stepz};


    VX_uint32 color = 0x0;

    VX_uint32 type;

    do {
        cube[X] = 0.0f;
        cube[Y] = 0.0f;
        cube[Z] = 0.0f;
        cube[W] = hw;

        void * node = ao_addr(self->root, dcam, cube, &type, &color);

        if( type == AOCT_RAW ){
            /* Do grid traversal */
            //return 0x0000ff00;
            //APPLY3( out , n_ray , += );
            c = raw_step( node,
                          cube, &rp,
                          dcam);
            APPLY3( out, dcam , = ); /* TODO: merge ... */
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
        else if (type == AOCT_NODE) {
            c = ((VX_aoct_node*)node)->color;
        }

        if (c & 0x00FFFFFF) {
            #ifdef VISUALIZE_COMPLEXITY
            c = color;
            #endif
            break;
        }

        if( !(out[X] < hw && out[Y] < hw && out[Z] < hw &&
              out[X] >= 0.0f && out[Y] >= 0.0f && out[Z] >= 0.0f )) {
            return 0;
        }

        APPLY3( dcam , out , = );

    } while (1);

    return c;
}
