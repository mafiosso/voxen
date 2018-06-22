#include "VX_lib.h"

VX_machine VX_lib;

int VX_init( VX_machine driver , VX_uint32 w , VX_uint32 h , VX_uint32 flags )
{
    VX_lib = driver;
    return VX_lib.init( &VX_lib , (VX_rect){0,0,w,h} , flags );
}
