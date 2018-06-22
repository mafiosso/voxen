#include "VX_lib.h"
#include <stddef.h>
#include <math.h>

/* DDA only */
int isect_box( VX_fpoint3 orig , VX_fpoint3 dir ,
               VX_fpoint3 box_min, VX_fpoint3 box_max)
{
    VX_fpoint3 tmin = { (box_min.x - orig.x)/dir.x,
                        (box_min.y - orig.y)/dir.y,
                        (box_min.z - orig.z)/dir.z };

    VX_fpoint3 tmax = { (box_max.x - orig.x)/dir.x,
                        (box_max.y - orig.y)/dir.y,
                        (box_max.z - orig.z)/dir.z };

    VX_fpoint3 real_min = { MIN( tmin.x , tmax.x ),
                            MIN( tmin.y , tmax.y ),
                            MIN( tmin.z , tmax.z ) };

    VX_fpoint3 real_max = { MAX( tmin.x , tmax.x ),
                            MAX( tmin.y , tmax.y ),
                            MAX( tmin.z , tmax.z ) };

    float minmax = MIN( MIN(real_max.x, real_max.y), real_max.z);
    float maxmin = MAX( MAX(real_min.x, real_min.y), real_min.z);

    return (minmax >= maxmin);
}

typedef struct VOX_box{
	int x,y,z,w;
}VOX_box;

static int hit = 0;

static VX_uint32 oct_ray_get_voxel_impl_rec( VX_oct_node * root , 
                                             VOX_box b, VX_fpoint3 * dcam ,
                                             VX_fpoint3 * n_ray ,
                                             int * good_p , int max_lod )
{
	if( !max_lod || !root->flags ){
		if( root->color ){
			*good_p = 1;
      hit++;
		}

		return root->color;
	}

	b.w = b.w / 2;
	/* scan cube in hardcoded order :/ TODO: order by ray */
	for( int z = 0 ; z < 2 ; z++ ){
		for( int y = 0; y < 2 ; y++ ){
			for( int x = 0; x < 2 ; x++){
				VX_oct_node * child = root->childs[ (z*4) + (y*2) + x ];
				if( !child ){ continue; }

				VOX_box tmp = {b.x + (x*b.w) , b.y + (y*b.w) , b.z + (z*b.w) , b.w };
				VX_fpoint3 box_min = { (float)tmp.x , (float)tmp.y , (float)tmp.z};
				VX_fpoint3 box_max = { box_min.x + (float)b.w ,
                               box_min.y + (float)b.w ,
                               box_min.z + (float)b.w };

				if( isect_box( *dcam , *n_ray , box_min , box_max ) ){
					VX_uint32 c = oct_ray_get_voxel_impl_rec( child , tmp , dcam , n_ray , good_p , max_lod-1 );
					if( *good_p )
						return c;
				}
			}
		}
	}

	return 0;
}

VX_uint32 VX_oct_ray_get_voxel_dda( VX_model * self , VX_fpoint3 dcam ,
                                       VX_fpoint3 n_ray , int max_lod )
{
    VX_fpoint3 box_min = { (float)self->dim_size.x , (float)self->dim_size.y , (float)self->dim_size.z};
    VX_fpoint3 box_max = { box_min.x + (float)self->dim_size.w , box_min.y + (float)self->dim_size.w , box_min.z + (float)self->dim_size.w };
    
    if( !isect_box( dcam , n_ray , box_min , box_max ) ){
        return 0;
    }
    
    int good_p = 0;
    VX_fpoint3 mdcam = dcam;
    VX_fpoint3 mn_ray = n_ray;
    
    hit = 0;
    
    VX_uint32 c = oct_ray_get_voxel_impl_rec( self->root ,
                                              (VOX_box){self->dim_size.x,
                                                      self->dim_size.y,
                                                      self->dim_size.z,
                                                      self->dim_size.w},
                                              &mdcam ,
                                              &mn_ray , 
                                              &good_p , 
                                              10 );

  return c;
}

static int edge_case_p( VX_int32 * box_min , VX_int32 * box_max ,
                        float    * point ){
    int out = 0;
    out += box_min[X] == point[X];
    out += box_min[Y] == point[Y];
    out += box_min[Z] == point[Z];

    out += box_max[X] == point[X];
    out += box_max[Y] == point[Y];
    out += box_max[Z] == point[Z];

    return out > 1;
}


/***************************************/
/* STACKLESS kD-restart-like algorithm */
VX_uint32 oct_get_vox_stackless( VX_oct_node * root ,
                                          VX_uint32 hw,
                                          VX_uint32 x,
                                          VX_uint32 y,
                                          VX_uint32 z,
                                          VX_cube * b,
                                          int * lod )
{
    *b = (VX_cube){ 0 , 0 , 0 , hw };
    VX_uint32 z_order, y_order, x_order; /* uint32 is faster than byte */

    while( (root->flags != 0) & ((*lod) > 0) )
    {
        hw >>= 1;
        z_order = (z >= (b->z + hw));
        y_order = (y >= (b->y + hw));
        x_order = (x >= (b->x + hw));

        /* 0 or 1 * hw will give hw or 0 => ifless benefit */
        /* recycle add variable - give the compiler chance */
        VX_uint32 add = (x_order) * hw;
        b->x += add;
        add = (y_order) * hw;
        b->y += add;
        add = (z_order) * hw;
        b->z += add;

        b->w = hw;

        /* | may be faster than + but in this case equivalent */
        root = root->childs[ (z_order<<2) | (y_order<<1) | (x_order) ];
        (*lod)--;

        if( !root )
            return 0;
    }
    
    return root->color;
}

VX_uint32 VX_oct_ray_get_voxel_stackless( VX_model * self ,
                                          float * idcam ,
                                          float * n_ray ,
                                          float * end_point,
                                          int   max_lod )
{
    /* TODO: check if dcam is in octree else either skip it or clip */
    VX_uint32 line_perm = VX_line_sort( *((VX_fpoint3*)n_ray) );
    VX_cube b;
    float tmp[3];
    VX_int32 iw = self->dim_size.w;
    VX_ipoint3 tp;
    VX_byte * bperm = (VX_byte*)(&line_perm);
    int ip [3] = {idcam[X] , idcam[Y] , idcam[Z]};
    
    do{
        int lod_tmp = max_lod;
        VX_uint32 pixel = oct_get_vox_stackless( self->root , iw ,
                                                 ip[X] ,
                                                 ip[Y] ,
                                                 ip[Z] ,
                                                 &b,
                                                 &lod_tmp );
        if( pixel ){ return pixel; }

        tp = (VX_ipoint3){b.x+b.w , b.y+b.w , b.z+b.w};
        
        int ret = VX_cube_out_point( (int*)(&b) , (int*)&tp ,
                                     (VX_fpoint3*)idcam ,
                                     (VX_fpoint3*)n_ray ,
                                     tmp , bperm );

        if( ret < 0 ){ return 0; } /* early termination */
        
        ip[X] = tmp[X]; ip[Y] = tmp[Y]; ip[Z] = tmp[Z];

        /* test if weird case */
        if( edge_case_p( (int*)&b , (int*)&tp , tmp ) ){
            tmp[X] += n_ray[X]*0.1f;
            tmp[Y] += n_ray[Y]*0.1f;
            tmp[Z] += n_ray[Z]*0.1f;

            ip[X] = tmp[X]; ip[Y] = tmp[Y]; ip[Z] = tmp[Z];
        }
        else if( n_ray[ret] < 0.0f ){
            ip[ret] -= 1;
        }
        

    }while( !((tmp[X] >= iw) | (tmp[Y] >= iw) | (tmp[Z] >= iw) |
              (tmp[X] <= 0) | (tmp[Y] <= 0) | (tmp[Z] <= 0) ));


    return 0;
}

/*******************************************/
/* kD-backtrack-like algorithm */

typedef struct _stack_item{
    VX_cube b;
    VX_oct_node * n;
}_stack_item;

#define POINT_IN_CUBE_P( C , P ) \
    ((((VX_uint32)P[X] - (VX_uint32)C.x) < C.w ) &&                     \
     (((VX_uint32)P[Y] - (VX_uint32)C.y) < C.w ) &&                     \
     (((VX_uint32)P[Z] - (VX_uint32)C.z) < C.w ))                       \

static VX_uint32 oct_get_vox_backtrack( _stack_item path[],
                                        VX_uint32 * level,
                                        VX_int32 p[3],
                                        int lod )
{
    VX_uint32 my_lvl = *level;
    /* do backtrack first */
    while( !(POINT_IN_CUBE_P( path[my_lvl].b , p )) && (my_lvl > 0) ){
        my_lvl--;
    }

    /* then forward steps */
    VX_uint32 z_order, y_order, x_order;
    VX_oct_node * root = path[my_lvl].n;
    VX_cube b = path[my_lvl].b;
    VX_uint32 hw = b.w;

    while( (root->flags != 0) & (lod-- > 0) )
    {
        hw >>= 1;
        z_order = (p[Z] >= (b.z + hw));
        y_order = (p[Y] >= (b.y + hw));
        x_order = (p[X] >= (b.x + hw));

        /* helping gcc */
        VX_uint32 add = (x_order) * hw;
        b.x += add;
        add = (y_order) * hw;
        b.y += add;
        add = (z_order) * hw;
        b.z += add;
        b.w = hw;

        root = root->childs[ (z_order<<2) | (y_order<<1) | x_order ];

        /* do not move behind if checking root */
        my_lvl++;
        path[my_lvl] = (_stack_item){ b , root };

        if( !root ){ /* empty voxel skipping */
            *level = my_lvl;
            return 0;
        }
    }

    *level = my_lvl;
    return root->color;
}

/* TODO: port better stackless here - some weird bug ahead */
VX_uint32 VX_oct_ray_get_voxel_backtrack( VX_model * self ,
                                          float idcam[3] ,
                                          float n_ray[3] ,
                                          float end_point[3],
                                          int max_lod )
{
    /* Build array first */
    _stack_item stack [((VX_oct_info*)self->data)->stack_len]; /* thx C99 */
    VX_uint32 lvl = 0;

    /* TODO!!!! */
    stack[0] = (_stack_item){ *((VX_cube*)&(self->dim_size)) , self->root };

    /* TODO: check if dcam is in octree else either skip or clip */
    float dcam[3] = {idcam[X] , idcam[Y] , idcam[Z]};
    VX_uint32 line_perm = VX_line_sort( *((VX_fpoint3*)n_ray) );
    VX_byte ret;
    VX_cube b;
    float tmp[3] = {dcam[X] , dcam[Y] , dcam[Z]};
    VX_int32 iw = self->dim_size.w;
    VX_byte * bperm = (VX_byte*)(&line_perm);
    int ip [3] = {dcam[X] , dcam[Y] , dcam[Z]};

    do{
        VX_uint32 pixel = oct_get_vox_backtrack( stack , &lvl ,
                                                 ip ,
                                                 max_lod );
        if( pixel ){ return pixel; }
        b = stack[lvl--].b;

        dcam[X] = tmp[X]; dcam[Y] = tmp[Y]; dcam[Z] = tmp[Z];
        int tp[3] = {b.x+b.w , b.y+b.w , b.z+b.w};

        ret = VX_cube_out_point( (int*)(&b) , tp ,
                                 (VX_fpoint3*)dcam ,
                                 (VX_fpoint3*)n_ray ,
                                 tmp ,
                                 bperm );

        if( ret < 0 ){ return 0; } /* early termination */

        ip[X] = tmp[X]; ip[Y] = tmp[Y]; ip[Z] = tmp[Z];

        if( edge_case_p( (int*)&b , tp , tmp ) ){
            tmp[X] += n_ray[X]*0.1f;
            tmp[Y] += n_ray[Y]*0.1f;
            tmp[Z] += n_ray[Z]*0.1f;
            
            ip[X] = tmp[X]; ip[Y] = tmp[Y]; ip[Z] = tmp[Z];
        }
        else if( n_ray[ret] < 0.0f ){
            ip[ret] -= 1;
        }

        /* assertion for weird cases - why they exist? */
        if( !memcmp( &dcam , tmp , sizeof( VX_fpoint3 )) )
            return 0x00ff00ff;

    }while( (tmp[X] < iw) & (tmp[Y] < iw) & (tmp[Z] < iw) &
            (tmp[X] >= 0) & (tmp[Y] >= 0) & (tmp[Z] >= 0) );
        
    return 0;
}

/* Kd-jump implementation */

/* STACK implementation */
VX_uint32 VX_oct_ray_get_voxel_stack( VX_model * m,
                                      VX_fpoint3 dcam ,
                                      VX_fpoint3 n_ray ,
                                      int max_lod )
{
    return 0;
}
