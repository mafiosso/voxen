#ifndef OCT_TRAVERSAL
#define OCT_TRAVERSAL

VX_uint32 VX_oct_ray_get_voxel_dda( VX_model * self , VX_fpoint3 dcam ,
                                           VX_fpoint3 n_ray , int max_lod );

VX_uint32 VX_oct_ray_get_voxel_stackless( VX_model * self ,
                                          float * dcam ,
                                          float * n_ray ,
                                          float * end_point,
                                          int max_lod );

VX_uint32 VX_oct_ray_get_voxel_backtrack( VX_model * self ,
                                          float dcam[3] ,
                                          float n_ray[3] ,
                                          float end_point[3],
                                          int max_lod );


#endif
