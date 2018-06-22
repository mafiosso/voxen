#include "SDL/SDL.h"
#include "VX_lib.h"

static VX_surface * internal_mk_surface( SDL_Surface * s );

static int sdl_init ( VX_machine * self , VX_rect resolution , VX_uint32 flags )
{
    printf("info: initializing SDL driver\n");

    VX_machine_default_init( self );

    if( SDL_Init (SDL_INIT_VIDEO) )
    {
        printf("err: SDL can not be created!\n");
        return -1;
    }

    Uint32 native_flags = ( (flags & VX_FULLSCREEN) != 0 ? SDL_FULLSCREEN : 0 );

    SDL_Surface * hw_surf = SDL_SetVideoMode (resolution.w, resolution.h , 32, native_flags | SDL_SWSURFACE );

    if( !hw_surf )
    {
        SDL_Quit();
        return -1;
    }    
    
    self->resolution = resolution;
    self->flags = flags;
    self->native_surface = internal_mk_surface( hw_surf );

//    atexit( SDL_Quit );

    return 0;
}

static int sdl_quit( VX_machine * self )
{
    if( self->native_surface )
        self->native_surface->free( self->native_surface );

    SDL_Quit();
    return 0;
}

static void sdl_surface_free( VX_surface * self )
{
    SDL_FreeSurface( self->native_surf );
    free( self );
}

static VX_uint32 sdl_surface_get_pixel( VX_surface * self , VX_uint32 x , VX_uint32 y )
{
    SDL_Surface * s = self->native_surf;
    //    SDL_LockSurface( s );
    Uint8 * addr = s->pixels;
    Uint32 pixel = 0;
    addr += s->pitch*y;
    addr += s->format->BytesPerPixel * x;
    memcpy( &pixel , addr , s->format->BytesPerPixel );
    //SDL_UnlockSurface( s );

    return pixel;
}

static void sdl_surface_set_pixel( VX_surface * self , VX_uint32 x , VX_uint32 y , VX_uint32 color )
{
    SDL_Surface * s = self->native_surf;
    //    SDL_LockSurface( s );
    Uint8 * addr = s->pixels;
    Uint32 pixel = color;
    addr += s->pitch*y;
    addr += s->format->BytesPerPixel * x;
    memcpy( addr , &pixel , s->format->BytesPerPixel );
    //SDL_UnlockSurface( s );
}

static void * sdl_surface_lock( VX_surface * self )
{
    SDL_Surface * s = self->native_surf;
    SDL_LockSurface( s );
    return s->pixels; 
}

static void sdl_surface_unlock( VX_surface * self )
{
    SDL_UnlockSurface( self->native_surf );
}

static VX_surface * internal_mk_surface( SDL_Surface * s )
{
    VX_surface * out = malloc(sizeof(VX_surface));

    *out = (VX_surface){ sdl_surface_free , sdl_surface_get_pixel , sdl_surface_lock,
                         sdl_surface_unlock, sdl_surface_set_pixel , s->w , s->h , s , NULL };

    return out;
}


static VX_surface * sdl_mk_surface( VX_uint32 w , VX_uint32 h )
{
    SDL_Surface *screen = SDL_GetVideoSurface ();
    SDL_PixelFormat *PixelFormat = screen->format;
    SDL_Surface *surf;

    surf = SDL_CreateRGBSurface (SDL_SWSURFACE, w, h,
                                 PixelFormat->BitsPerPixel,
                                 PixelFormat->Rmask, PixelFormat->Gmask,
                                 PixelFormat->Bmask, PixelFormat->Amask);

    if (!surf)
        return NULL;

    SDL_FillRect (surf, NULL, SDL_MapRGB (surf->format, 255, 0, 255));

    return internal_mk_surface( surf );
}

static void sdl_redraw( VX_machine * self )
{

//    SDL_FillRect (self->native_surface->native_surf, NULL, 0);
    SDL_Flip( self->native_surface->native_surf );
}

VX_machine VX_DRV_sdl = { .init = sdl_init ,
                          .quit = sdl_quit ,
                          .make_surface = sdl_mk_surface,
                          .redraw = sdl_redraw
};
