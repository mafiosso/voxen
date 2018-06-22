#include "VX_lib.h"
#include <math.h>

/*
  SLAB
*/
/*
  returns permutation of sorted items of a vector coded in UINT
  EXAMPLE: v( 50.0f , -10.0f , 15.0f ) => 
  (VX_byte*){ 0x00 , 0b0000001 , 0b00000011 (Z) ,
  0b00000000 (X - biggest)}

*/

/* input: [Z < Y] [Y < X] [Z < X] */
static VX_uint32 PERM_LOOKUP[2][2][2];

static VX_uint32 line_sort( VX_fpoint3 dir ){
    dir.y = fabs(dir.y);
    dir.z = fabs(dir.z);
    dir.x = fabs(dir.x);
    return PERM_LOOKUP[ dir.z < dir.y ][ dir.y < dir.x ][ dir.z < dir.x ];
}

/* WARN: Little endianity! */
/* lazy technique computation ... only first use will compute lookup */
static VX_uint32 perm_table_create( VX_fpoint3 dir ){
    for( int zy_cmp = 0 ; zy_cmp < 2 ; zy_cmp++ ){
        for( int yx_cmp = 0 ; yx_cmp < 2 ; yx_cmp++ ){
            for( int zx_cmp = 0 ; zx_cmp < 2 ; zx_cmp++ ){

                VX_uint32 tmp_y = Y << (8*(zy_cmp +(1-yx_cmp)));
                VX_uint32 tmp_x = X << (8*(yx_cmp + zx_cmp));
                VX_uint32 tmp_z = Z << (8*((1-zy_cmp) + (1-zx_cmp)));
                PERM_LOOKUP[zy_cmp][yx_cmp][zx_cmp] =
                    0x00000000 | tmp_z | tmp_y | tmp_x;                    
            }
        }
    }

    VX_line_sort = line_sort; /* all next calls will jump to line_sort */
    return line_sort( dir );
}

void VX_permutation_inspect(VX_uint32 p ){
    VX_byte * perm = (VX_byte*)(&p);

    printf("perm(");

    for( int i = 0; i < 4 ; i++ ){
        VX_uint32 tmp = perm[i];
        printf("%d " , tmp);
    }

    printf(")\n");
}

/* using only coordinate axes centered planes/slabs -> optimization */
/* EXAMPLE:
int z_slab_isection( VX_fpoint3 dir , VX_fpoint3 dcam ,
                     int d , VX_ipoint3 * out_pt )
{
    if( dir.z == 0.0f ){
        return 0;
    }

    float div = -1.0f / dir.z; 
    float u = (dcam.z - d) * div;
    float dist = dcam.z - (float)d;

    if( FLOAT_SIGNUM(dist) != FLOAT_SIGNUM( u ) )
        return 0; //lookin elsewhere

    *out_pt= (VX_ipoint3){ (int)(dir.x*u) , (int)(dir.y*u) , (int)d };
    return 1;
}
*/

/* macro for creating slab functions */
//        if( d == dcam->DIRECTIVE_AXE ){printf("assert\n"); return 0; }; 
#define MAKE_SLAB_FUNC( FUNC_NAME , DIRECTIVE_AXE , UA1 , UA2 , ND, NUA1 , NUA2 ) \
    int FUNC_NAME( VX_fpoint3 * dir , VX_fpoint3 * dcam ,               \
                   int d , float * out_pt ,                               \
                   int box_min[3] , int box_max[3]  )                   \
    {                                                                   \
        if( dir->DIRECTIVE_AXE == 0.0f ){ return 0; }                   \
                                                                        \
        float dist = (float)d - dcam->DIRECTIVE_AXE ;                   \
        float u = (dist / dir->DIRECTIVE_AXE);                          \
                                                                        \
        out_pt[NUA1] = dcam->UA1 + (dir->UA1*u);                        \
        out_pt[NUA2] = dcam->UA2 + (dir->UA2*u);                        \
        out_pt[ND] = (float)d;                                          \
                                                                        \
        return  CHECK_INTERVAL(out_pt[NUA1] , box_min[NUA1] , box_max[NUA1] ) && \
            CHECK_INTERVAL( out_pt[NUA2] , box_min[NUA2] , box_max[NUA2] ); \
    }                                                                   \
    

MAKE_SLAB_FUNC( z_slab_func , z , x , y , Z , X , Y );
MAKE_SLAB_FUNC( y_slab_func , y , x , z , Y , X , Z );
MAKE_SLAB_FUNC( x_slab_func , x , y , z , X , Y , Z );

static int (*slab_vtbl[])(VX_fpoint3* , VX_fpoint3* , int , float* , int* , int* ) =
{
    x_slab_func,
    y_slab_func,
    z_slab_func
};

int VX_cube_clip( int box_min [3] , int box_max[3] ,
                  VX_fpoint3 * dcam , VX_fpoint3 * dir,
                  float out[2][3] , VX_uint32 perm )
{
    VX_byte * bp = (VX_byte*)(&perm);
    int p = 2;
    int i = 0;

    do{
        VX_uint32 dim = bp[p];
        i += slab_vtbl[dim](dir , dcam , 
                            box_min[dim] , out[i] , box_min , box_max);

	//        PRINT_RAW_POINT3( out[i] );
        if( i >= 2 ){ return i; }
        i += slab_vtbl[dim](dir , dcam , 
                             box_max[dim] , out[i] , box_min , box_max);
        //PRINT_RAW_POINT3( out[i] );
    }while( (p-- > 0) && (i < 2 ));

    return i;
}

/* returns successive coordination if pass -1 otherwise */
int VX_cube_out_point( int box_min[3] , int box_max[3] ,
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
        ret = slab_vtbl[ dim ]( ray , point ,
                                d_near , out ,
                                box_min , box_max);
        it--;
    }

    return (ret ? dim : -1);
}

/* initialization of VX_line_sort */
VX_uint32 (*VX_line_sort)(VX_fpoint3) = &perm_table_create;
