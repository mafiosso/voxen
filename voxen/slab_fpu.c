#include "VX_lib.h"

/* macro for creating slab functions */
//        if( d == dcam->DIRECTIVE_AXE ){printf("assert\n"); return 0; }; 
#define MK_SLAB_FUNC_FPU( FUNC_NAME , DIRECTIVE_AXE , UA1 , UA2 , ND, NUA1 , NUA2 ) \
    static int FUNC_NAME( VX_fpoint3 * dir , VX_fpoint3 * dcam ,        \
                          float d , float * out_pt ,                    \
                          float box_min[3] , float box_max[3]  )        \
    {                                                                   \
        if( dir->DIRECTIVE_AXE == 0.0f ){ return 0; }                   \
                                                                        \
        float dist = d - dcam->DIRECTIVE_AXE;                           \
        /* TODO: possibly dist * (1.0f - dir->DIRECTIVE_AXE) */         \
        float u = dist / dir->DIRECTIVE_AXE;                            \
                                                                        \
        out_pt[NUA1] = dcam->UA1 + (dir->UA1*u);                        \
        out_pt[NUA2] = dcam->UA2 + (dir->UA2*u);                        \
        out_pt[ND] = d;                                                 \
                                                                        \
        return  CHECK_INTERVAL(out_pt[NUA1] , box_min[NUA1] , box_max[NUA1] ) && \
            CHECK_INTERVAL( out_pt[NUA2] , box_min[NUA2] , box_max[NUA2] ); \
    }                                                                   \
    

MK_SLAB_FUNC_FPU( z_func_fpu , z , x , y , Z , X , Y );
MK_SLAB_FUNC_FPU( y_func_fpu , y , x , z , Y , X , Z );
MK_SLAB_FUNC_FPU( x_func_fpu , x , y , z , X , Y , Z );

static int (*slab_vtbl_fpu[])(VX_fpoint3* , VX_fpoint3* , float ,
                              float* , float* , float* ) =
{
    x_func_fpu,
    y_func_fpu,
    z_func_fpu
};

/* TODO: this isnt really kosher yet! check for right direction! */
int VX_cube_in_point_fpu( float box_min[3] , float box_max[3] ,
                          float point[3],
                          float ray[3] ,
                          float out[3],
                          VX_byte line_perm[3] )
{
    int it = 2;
    VX_byte ret = 0;
    VX_byte dim;

    while( (it >= 0) && !ret ){
        dim = line_perm[it];
        int d_near = (ray[dim] < 0.0f ? box_max[dim] : box_min[dim]);

        float dpn = d_near - point[dim];

        if( FLOAT_SIGNUM((dpn)) != FLOAT_SIGNUM( (ray[dim]) ) ){
            ret = 0;
            it--;
            continue;
        }        

        ret = slab_vtbl_fpu[ dim ]( (VX_fpoint3*)ray , (VX_fpoint3*)point ,
                                    d_near , out ,
                                    box_min , box_max);
        it--;
    }

    return (ret ? dim : -1);
}


/* returns successive coordination if pass -1 otherwise */
int VX_cube_out_point_fpu( float box_min[3] , float box_max[3] ,
                       VX_fpoint3 * point,
                       VX_fpoint3 * ray ,
                       float out[3],
                       VX_byte line_perm[3] )
{
    int it = 2;
    VX_byte ret = 0;
    VX_byte dim;

    while( (it >= 0) && !ret ){
        dim = line_perm[it];
        int d_near = (((float*)ray)[dim] < 0.0f ? box_min[dim] : box_max[dim]);
        ret = slab_vtbl_fpu[ dim ]( ray , point ,
                                    d_near , out ,
                                    box_min , box_max);
        it--;
    }

    return (ret ? dim : -1);
}
