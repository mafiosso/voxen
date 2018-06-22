#include "VX_lib.h"
#include <math.h>

/* some utilities */
double
rad_to_degrees (double rad)
{
  return (rad * 180.0f) / PI;
}

double
deg_to_radians (double deg)
{
  return deg * (PI / 180.0f);
}

double
dvector2_length( VX_dpoint2 v )
{
    return sqrt( (v.x*v.x) + (v.y*v.y) );
}

double
dvector2_angle (VX_dpoint2 v)
{
  return asin (v.y / dvector2_length (v));
}

VX_dpoint2
dvector2_normalize (VX_dpoint2 v, float len)
{
  float k = sqrt ((len * len) / ((v.x * v.x) + (v.y * v.y)));
  VX_dpoint2 out = { (v.x * k), (v.y * k) };
  return out;
}

VX_dpoint2
dvector2_rotate (VX_dpoint2 v, double rad)
{
  VX_dpoint2 out;
  out.x = (v.x * cos (rad)) - (v.y * sin (rad));
  out.y = (v.x * sin (rad)) + (v.y * cos (rad));
  return out;
}

static inline
void dpoint2_print( VX_dpoint2 fp ){
    printf("dp2: <%f , %f>\n" , fp.x , fp.y );
}

static inline
void fpoint3_print( VX_fpoint3 fp ){
    printf("dp2: <%f , %f, %f>\n" , fp.x , fp.y , fp.z );
}

static inline VX_fpoint3 fpoint3_make( VX_dpoint2 d1 , VX_dpoint2 d2 )
{
    float z = sqrt( 100.0f - (d1.x*d1.x) - (d2.y*d2.y) );
    return (VX_fpoint3){ d1.x , d2.y , z };
}

VX_fpoint3 fvector3_normalize (VX_fpoint3 v, float len)
{
  double k = sqrt ((len * len) / ((v.x * v.x) + (v.y * v.y) + (v.z*v.z)));
  return (VX_fpoint3){ (v.x * k), (v.y * k) , (v.z * k) };
}

void fvector3_raw_normalize (float v[3], float len)
{
  double k = sqrt ((len * len) / ((v[X] * v[X]) + (v[Y] * v[Y]) + (v[Z]*v[Z])));
  v[X] = (v[X] * k); v[Y] = (v[Y] * k); v[Z] = (v[Z] * k);
}


typedef struct _cam_thread_t{
    VX_model * m;
    VX_camera * c;
    VX_rect clip_rect;
}_cam_thread_t;

static void cam_thread( _cam_thread_t * arg ){
    arg->m->present( arg->m , arg->c , arg->clip_rect );
}

/* screen scan algorithm */
void VX_camera_draw( VX_camera * self , VX_model * m ){
    /* present model here */

    int pc = self->workers->peasants_count;
    void (*cam_threads[pc])(void *);
    _cam_thread_t args[pc];
    _cam_thread_t * pargs[pc];
    int scan_lines_per_thread = self->surface->h / pc;
    int last_line = 0;

    for( int j = 0 ; j < pc ; j++ ){
        cam_threads[j] =(void (*) (void*)) cam_thread;
        args[j] = (_cam_thread_t){ m , 
                                   self , 
                                   (VX_rect){ 0 , last_line ,
                                              self->surface->w ,
                                              scan_lines_per_thread }
        };
        pargs[j] = &(args[j]);
        last_line += scan_lines_per_thread;
    }

    /* handle integer error in slpt */
    int addition = self->surface->h - (pargs[pc-1]->clip_rect.y + pargs[pc-1]->clip_rect.h);

    pargs[pc-1]->clip_rect.h += addition;

    self->surface->lock_surface( self->surface );
    self->workers->enter( self->workers , (void**)pargs , cam_threads );
    self->surface->unlock_surface( self->surface );

    VX_lib.redraw( &VX_lib );
}

void VX_camera_destroy( VX_camera * self ){
    self->workers->destroy( self->workers );
    free( self->buffer );
    free( self );
}

static void def_frustum_build(VX_camera * self )
{
    float dz = (float)self->surface->w;
    float x_half = dz/2.0f;
    float y_half = (float)self->surface->h/2.0f;
    dz /= 2.0f;
    
    for( int y = 0 ; y < self->surface->h ; y++){
        for( int x = 0 ; x < self->surface->w ; x++ ){
            VX_fpoint3 fp = { -x_half + (float)x ,
                              y_half  - (float)y ,
                              dz};
            
            fp = fvector3_normalize( fp , 1.0f );
            ((VX_fpoint3*)self->buffer)[(y*self->surface->w)+x] = fp;
        }
    }
}

static VX_fpoint3 def_frustum_get( VX_camera * self , VX_uint32 x ,
                             VX_uint32 y )
{
    float fvray[4]; /* bottleneck! */
    float tr_out[4];

    VX_fpoint3 ray = ((VX_fpoint3*)self->buffer)[ (y*self->surface->w) + x ];
    memcpy( fvray , &ray , sizeof(VX_fpoint3) );
    fvray[3] = 0.0f;
    VX_fvector4_multiply( tr_out , self->rotation_matrix , fvray );

    return (VX_fpoint3){ tr_out[X] , tr_out[Y] , tr_out[Z] };
}

VX_camera * VX_camera_new( VX_surface * surf ,
                           VX_fpoint3 (*ray_generator)( 
                               VX_camera * self , VX_uint32 x, VX_uint32 y ) ,
                           int cpus_count )
{
    VX_camera * out = calloc( 1 , sizeof(VX_camera));
    out->buffer = malloc(sizeof(VX_fpoint3)*surf->w*surf->h);

    out->surface = surf;
    if( !ray_generator ){ /* build default */
        def_frustum_build( out );
        out->rg = def_frustum_get;
    }
    else{
        out->rg = ray_generator;
    }

    out->draw = (void (*))VX_camera_draw;
    out->destroy = VX_camera_destroy;

    /* buffer for normals */
    VX_matrix4_identity( out->rotation_matrix );

    /* start threaded performing */
    int cores = (cpus_count < 1 ? VX_lib.cores_count : cpus_count);
    VX_field * f = VX_field_new( cores );
    if( !f ){
        free( out->destroy );
        return NULL;
    }

    out->workers = f;

    return out;
}
