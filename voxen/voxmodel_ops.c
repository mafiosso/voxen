#include "VX_lib.h"

/* 
   blit one slice - image s to one of z coords of model m
*/

void VX_blit_slice( VX_model * m ,  VX_surface * s , VX_uint32 z ){
    for( VX_uint32 y = 0 ; y < s->h ; y++ ){
        for( VX_uint32 x = 0 ; x < s->w ; x++ ){
            VX_uint32 color = s->get_pixel( s , x , y );
            m->set_voxel( m , x , y , z , color );
        }
    }
}
