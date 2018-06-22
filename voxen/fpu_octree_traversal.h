#ifndef FPU_OCT_TR
#define FPU_OCT_TR

VX_uint32 VX_oct_ray_voxel_rst_fpu( VX_model * self ,
                                    float * idcam ,
                                    float * n_ray ,
                                    float * out,
                                    int max_lod );

VX_uint32 VX_oct_ray_voxel_bcktrk_fpu( VX_model * self ,
                                       float * idcam ,
                                       float * n_ray ,
                                       float * out_pt,
                                       int max_lod );

VX_uint32 VX_oct_ray_voxel_sstack_fpu( VX_model * self ,
                                       float * idcam ,
                                       float * n_ray ,
                                       float * out_pt,
                                       int max_lod );

#endif
