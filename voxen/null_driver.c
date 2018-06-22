#include "VX_lib.h"

/*
  REFERENCE DRIVER
*/

static void null_surface_free( VX_surface * s )
{
    free( s->native_surf );
    free( s );
}

static VX_uint32 null_surface_get_pixel( VX_surface *self ,  VX_uint32 x ,
                                         VX_uint32 y ){
    VX_uint32 * ptr = (VX_uint32*)self->native_surf + (self->w*y) + x ;
    return *ptr;
}

static void * null_surface_lock( VX_surface * self ){
    return self->native_surf;
}

static void null_surface_unlock( VX_surface * self ){
}

static void null_surface_set_pixel( VX_surface * self , VX_uint32 x , VX_uint32 y , VX_uint32 color )
{
    VX_uint32 * ptr = (VX_uint32*)self->native_surf + (self->w*y) + x ;
    *ptr = color;
}

static VX_surface * null_mk_surface( VX_uint32 w , VX_uint32 h )
{
    VX_surface * out = malloc( sizeof(VX_surface) );
    VX_uint32 * surface = calloc( w*h , sizeof( VX_uint32 ) );

    *out = (VX_surface){ null_surface_free , null_surface_get_pixel ,
                         null_surface_lock , null_surface_unlock , 
                         null_surface_set_pixel , w , h , surface , NULL };

    return out;
}


static int null_init ( VX_machine * self , VX_rect resolution , VX_uint32 flags )
{
    printf("info: initializing NULL_Drv driver\n");

    VX_machine_default_init( self );

    self->resolution = resolution;
    self->flags = flags;
    self->native_surface = null_mk_surface( resolution.w , resolution.h );

    if( !self->native_surface ){
        return -1;
    }

    return 0;
}

static int null_quit( VX_machine * self )
{
    if( self->native_surface )
        self->native_surface->free( self->native_surface );
    memset( self , 0 , sizeof( VX_machine ) );

    return 0;
}


static void null_redraw( VX_machine * self )
{

}

VX_machine VX_DRV_null = { .init = null_init ,
                           .quit = null_quit ,
                           .make_surface = null_mk_surface,
                           .redraw = null_redraw
};
