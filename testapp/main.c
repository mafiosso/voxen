/* Semi command line test app for the VoXen library */
#include "VX_lib.h"
#include "SDL/SDL.h"
#include <math.h>
#include <stdio.h>

ssize_t getline(char **lineptr, size_t *n, FILE *stream);
#define VERSION "0.1"

typedef struct benchmark_t{
    VX_uint32 frames_drawn;
    VX_uint32 time; /* time in miliseconds */
    VX_uint32 best_frame_dt;
    VX_uint32 worst_frame_dt;
}benchmark_t;

typedef struct s_light{
    double mid[3];
    double axis[3];
    double rads_sec; /* radians per second */
    double curr_angle;
}s_light;

typedef struct demo_frame_t{
    VX_fpoint3 cam_pos;
    VX_dpoint3 basis[3];
    VX_light * lghts_pos;
}demo_frame_t;

typedef struct demo_t{
    VX_uint32 frames_count;
    VX_uint32 lghts_count;
    VX_uint32 ambient_light;
    demo_frame_t * frames;
}demo_t;

VX_model * g_model = NULL;
VX_camera * g_camera = NULL;
demo_t * g_demo = NULL;
s_light * lghts = NULL;
char * g_model_path = NULL;

int g_raw_p = 0;
int g_fscreen_p = 0;
benchmark_t g_bench = {0,1,0,0};

void bench_rst();
void basis( double m[4][4] , double x[3] , double y[3] , double z[3] );
void bench_update( VX_uint32 frame_ms );

char * cui_read_path(){
    printf("enter path: ");
    char * in_path = NULL;
    size_t len;
    if( getline( &in_path , &len , stdin ) <= 0 ){
        return NULL;
    }

    int ml = strlen( in_path );

    if( ml <= 1 ){
        free( in_path );
        return NULL;
    }

    in_path[ml-1] = '\0';

    return in_path;
}

VX_dpoint3 matrix_col( double m[4][4] , int idx ){
    return (VX_dpoint3){ m[0][idx] , m[1][idx] , m[2][idx] };
}

void demo_frame_add(){
    demo_t * d = g_demo;
    d->ambient_light = VX_ambient_light();
    d->frames_count++;
    d->frames = realloc( d->frames , sizeof( demo_frame_t ) * d->frames_count );
    demo_frame_t * f = d->frames;
    int idx = d->frames_count-1;
    f[idx].cam_pos = (VX_fpoint3){ g_camera->position.x ,
                                   g_camera->position.y ,
                                   g_camera->position.z };
    f[idx].basis[0] = matrix_col( g_camera->rotation_matrix , 0 );
    f[idx].basis[1] = matrix_col( g_camera->rotation_matrix , 1 );
    f[idx].basis[2] = matrix_col( g_camera->rotation_matrix , 2 );

    
    if( d->lghts_count > 0 )
        f[idx].lghts_pos = malloc( sizeof(VX_light) * d->lghts_count );
    else
        f[idx].lghts_pos = NULL;

    VX_light * ls = VX_lights_enumerate();
    for( int i = 0 ; i < d->lghts_count ; i++ ){
        f[idx].lghts_pos[i] = ls[i];
    }        
}

void demo_frame_add_ex( demo_frame_t * df ){
    demo_t * d = g_demo;
    d->frames_count++;
    d->frames = realloc( d->frames , sizeof( demo_frame_t ) * d->frames_count );  
    d->frames[ d->frames_count-1 ] = *df;

}

void demo_rst(){
    if( g_demo ){
        for( int f = 0 ; f < g_demo->frames_count ; f++ ){
 
            free( g_demo->frames[f].lghts_pos );
        }
        free( g_demo->frames );
        free( g_demo );
    }

    g_demo = calloc( 1 , sizeof( demo_t ) );
    g_demo->lghts_count = VX_lights_count();
    g_demo->ambient_light = VX_ambient_light();
}

void demo_dump(){
    char * out_path = NULL;
    if( !(out_path = cui_read_path()) ){
        printf("err: bad input\n");
        return;
    }

    FILE * f = fopen( out_path , "w+" );
    if( !f ){
        printf("err: can't write file\n");
        free( out_path );
        return;
    }

    free( out_path );

    VX_uint32 magic = 0xDEEF;
    fwrite( &magic , sizeof( VX_uint32 ) , 1 , f );
    fwrite( g_demo , 12 , 1 , f );

    for( int fi = 0 ; fi < g_demo->frames_count ; fi++ ){
        demo_frame_t * df = &(g_demo->frames[fi]);
        fwrite( &(df->cam_pos) , sizeof(VX_fpoint3) , 1 , f );
	fwrite( &(df->basis) , sizeof(VX_dpoint3) , 3 , f );
        fwrite( df->lghts_pos , sizeof( VX_light ), g_demo->lghts_count, f );
    }

    fclose( f );
}

void demo_read(){
    char * in_path = NULL;
    if( !(in_path = cui_read_path()) ){
        printf("err: bad input\n");
        return;
    }

    FILE * f = fopen( in_path , "rb" );
    if( !f ){
        printf("err: can't read file due to bad input %s\n" , in_path);
        free( in_path );
        return;
    }

    free( in_path );

    VX_uint32 magic;
    if( 1 != fread( &magic , sizeof(VX_uint32) , 1 , f ) )
    {
        printf("err: can't read file bad magic number\n");
        return;
    }

    if( magic != 0xDEEF ){
        printf("err: not a proper demo file!\n");
        return;
    }
    
    demo_rst();
    VX_uint32 frames = 0;
    if( 1 != 
        fread( g_demo , 12 , 1 , f ) ){
        printf("err: can't read file (demo header)\n");
        fclose( f );
        demo_rst();
        return;
    }
    g_demo->frames = NULL;
    VX_uint32 orig_frames = g_demo->frames_count;
    g_demo->frames_count = 0;
    demo_frame_t * df = NULL;

    while( !feof( f ) && (frames++) < orig_frames  ){
        free( df );
        df = calloc( 1 , sizeof( demo_frame_t ) );
        size_t ld = fread( df , sizeof(VX_fpoint3) , 1 , f );
	ld += fread( ((VX_byte*)df) + offsetof( demo_frame_t , basis ) , sizeof(VX_dpoint3) , 3 , f );

        if( 4 != ld )
        {
            free( df );
            if( feof( f ) ){
                break;
            }
            printf("ferror: %d\n" , ferror(f) );
            printf("%u\n" , ld );
            fclose( f );
            demo_rst();
            printf("err: can't read file (frame)\n");
            free( df );
            return;
        }

        VX_light * lghts = NULL;
        
        if( g_demo->lghts_count > 0 ){
            lghts = calloc( g_demo->lghts_count , sizeof(VX_light) );
            if( g_demo->lghts_count !=
                fread( lghts , sizeof(VX_light) , g_demo->lghts_count , f ) )
            {
                fclose( f );
                demo_rst();
                printf("err: can't read file (light array)\n");
                free( lghts );
                free( df );
                return;
            }
        }
        
        df->lghts_pos = lghts;
        demo_frame_add_ex( df );
    }
}

void demo_run(){
    if( !g_demo  || g_demo->frames_count <= 0 ){
        printf("err: no reasonable demo to run\n");
        return;
    }

    VX_model * m = g_model;
    if( !m ){
        printf("info: There is no model to run on\n");
        return;
    }

    unsigned int limit_lights = 0;

    printf("Set limit of lights in scene: ");
    if( !scanf( "%u" , &limit_lights ) ){
        printf("warn: bad input - ignoring\n");
        limit_lights = g_demo->lghts_count;
    }

    if( g_fscreen_p ){
        SDL_WM_ToggleFullScreen( VX_lib.native_surface->native_surf );
    }

    bench_rst();
    VX_camera * cam = g_camera;
    demo_frame_t * df;
    VX_uint32 frame = 0;
    SDL_Event keyevent;

    VX_ambient_light_set( g_demo->ambient_light );
    VX_lights_clear();
    for( int l = 0 ; l < g_demo->lghts_count && l < limit_lights ; l++ ){
        VX_light l = {{0,0,0} , 0};
        VX_light_dynamic_add( &l );
    }

    while( frame < g_demo->frames_count )
    {
        df = &(g_demo->frames[frame++]);

        SDL_PollEvent( &keyevent );
        int ticks = SDL_GetTicks();
        Uint8 *ks = SDL_GetKeyState(NULL);
        if( ks[SDLK_ESCAPE] ){
            break;
        }
        
        double vside[4] = {df->basis[0].x , df->basis[0].y ,
                           df->basis[0].z , 0 };
        double vup[4]   = {df->basis[1].x , df->basis[1].y ,
                           df->basis[1].z , 0 };
        double vfwd[4]  = {df->basis[2].x , df->basis[2].y , 
                           df->basis[2].z , 0 };
        basis( cam->rotation_matrix , vside , vup , vfwd );

        cam->position.x = (int)df->cam_pos.x;
        cam->position.y = (int)df->cam_pos.y;
        cam->position.z = (int)df->cam_pos.z;

        VX_light * ls = VX_lights_enumerate();

        for( int l = 0; l < VX_lights_count() ; l++ ){
            ls[l] = df->lghts_pos[l];
        }

        cam->draw( cam , g_model );

        VX_uint32 tck = SDL_GetTicks() - ticks;
        bench_update( tck );
    }
    
    if( g_fscreen_p ){
        SDL_WM_ToggleFullScreen( VX_lib.native_surface->native_surf );
    }
}

void bench_rst(){
    memset( &g_bench , 0 , sizeof(benchmark_t) );
    g_bench.best_frame_dt = 0xffffffff;
}

void bench_update( VX_uint32 frame_ms ){
    g_bench.frames_drawn += 1;
    g_bench.time += frame_ms;
    if( g_bench.best_frame_dt > frame_ms )
        g_bench.best_frame_dt = frame_ms;

    if( g_bench.worst_frame_dt < frame_ms )
        g_bench.worst_frame_dt = frame_ms;
}

void init( VX_uint32 w , VX_uint32 h){
    int ret = VX_init( VX_DRV_sdl , w , h , 0 );
    if( ret != 0 ){
        printf("err: failed to establish library object\n");
        return;
    }

    g_camera = VX_camera_new( VX_lib.native_surface , NULL , 9 );
    if( !g_camera )
    {
        printf("err: failed to create camera object!\n");
        return;
    }

    SDL_WM_SetCaption( "[Powered by VoXen] Use Esc to exit, w, a, s, d keys to move, mouse to aim.", NULL ); 
}

void vxquit(){
    demo_rst();
    free( g_demo );

    if( g_model )
        g_model->free( g_model );

    g_camera->destroy( g_camera );
    VX_lib.quit( &VX_lib );
    exit(0);
}

void load_model( const char * path ){
    if( g_model )
        g_model->free( g_model );

    g_model = VX_model_octree_new();
    g_model->load( g_model , path );
}

void spheric_vector( float out[3] , float phi , float theta , float r ){
    float st = sin( theta );
    float ct = cos( theta );
    float sp = sin( phi );
    float cp = cos( phi );
    
    out[X] = r*st*cp;
    out[Y] = r*st*sp;
    out[Z] = r*ct;
}

void basis( double m[4][4] , double x[3] , double y[3] , double z[3] ){
    VX_matrix4_identity( m );
    m[0][0] = x[0];
    m[1][0] = x[1];
    m[2][0] = x[2];

    m[0][1] = y[0];
    m[1][1] = y[1];
    m[2][1] = y[2];

    m[0][2] = z[0];
    m[1][2] = z[1];
    m[2][2] = z[2];
}

void line_addressing( )
{
    #define STEP 8.0f
    VX_model * m = g_model;
    if( !m ){
        printf("info: There is no model to run on\n");
        return;
    }

    if( g_fscreen_p ){
        SDL_WM_ToggleFullScreen( VX_lib.native_surface->native_surf );
    }

    bench_rst();
    VX_camera * cam = g_camera;
    float angles[3] = {0.0f , 0.0f , 0.0f};

    /* in implicit this is the standard basis <vside , vup, vfwd>
       or other basis derived from it by rotation transformations */
    double vside[4] = { cam->rotation_matrix[0][0] , cam->rotation_matrix[1][0] , cam->rotation_matrix[2][0] , 0 };
    double vup[4]   = { cam->rotation_matrix[0][1] , cam->rotation_matrix[1][1] , cam->rotation_matrix[2][1] ,0 };
    double vfwd[4]  = { cam->rotation_matrix[0][2] , cam->rotation_matrix[1][2] , cam->rotation_matrix[2][2] , 0 };

    double rot_m[4][4];
    VX_matrix4_identity( rot_m );
    SDL_Event keyevent;
    int end_p = 0;
    
    int pos[2] = { cam->surface->w/2 , cam->surface->h/2 };

    SDL_WarpMouse( pos[X] , pos[Y] );

    while (!end_p )
    {
        float mul = 1.0f;
        SDL_PollEvent( &keyevent );

        int ticks = SDL_GetTicks();
        float direction_vect[4] = {0,0,0,0};

        Uint8 *ks = SDL_GetKeyState(NULL);

        if( ks[SDLK_LSHIFT] )
            mul = 10.0f;
        
        if( ks[SDLK_a] )
            direction_vect[X] = -STEP*mul;

        if( ks[SDLK_d] )
            direction_vect[X] = STEP*mul;

        if( ks[SDLK_q] )
            direction_vect[Y] = STEP*mul;

        if( ks[SDLK_y] )
            direction_vect[Y] = -STEP*mul;

        if( ks[SDLK_ESCAPE] ){
            if( g_fscreen_p ){
                SDL_WM_ToggleFullScreen( VX_lib.native_surface->native_surf );
            }
            return;
        }            

        if( ks[SDLK_w] )
            direction_vect[Z] = STEP*mul;

        if( ks[SDLK_s] )
            direction_vect[Z] = -STEP*mul;

        int mouse_pos[2];
        SDL_GetMouseState(&(mouse_pos[X]), &(mouse_pos[Y]));

        angles[X] = (float)(mouse_pos[Y] - pos[Y])*0.01f;
        angles[Y] = (float)(mouse_pos[X] - pos[X])*0.01f;

        SDL_WarpMouse( pos[X] , pos[Y] );

        double lr[4][4];
        VX_matrix4_rotated( lr , vup , (double)angles[Y] );

        /* NOTE: normalization is for sure 
           and for minimizing cumulative rounding errs */
        double tmpside[4];
        VX_dvector4_multiply( tmpside , lr , vside );
        memcpy( vside , tmpside , sizeof(double)*3 );
        VX_dvector3_normalize( vside );

        VX_matrix4_rotated( lr , vside , (double)angles[X] );
        double tmpup[4];
        VX_dvector4_multiply( tmpup , lr , vup );
        memcpy( vup , tmpup , sizeof(double)*3);
        VX_dvector3_normalize( vup );

        double tmpfwd[4];
        CROSS_PRODUCT( tmpfwd , vside , vup );
        memcpy( vfwd , tmpfwd , sizeof(double)*3 );
        VX_dvector3_normalize( vfwd );
        
        basis( cam->rotation_matrix , vside , vup , vfwd );

        float dvect[4];

        /* recompute direction by rotation matrix */
        VX_fvector4_multiply( dvect , cam->rotation_matrix , direction_vect );

        PRINT_POINT3( cam->position );
        
        cam->position.x += dvect[X];
        cam->position.y += dvect[Y];
        cam->position.z += dvect[Z];

        memset( angles , 0 , sizeof(float)*3 );
        
        cam->draw( cam , m );

        VX_uint32 tck = SDL_GetTicks() - ticks;
        bench_update( tck );
        demo_frame_add();

        /* update lights' positions */
        for( int l = 0; l < VX_lights_count() ; l++ ){
            if( lghts[l].rads_sec != 0.0f ){
                double lm[4][4];
                double radss = (((double)tck)/1000.0f)*lghts[l].rads_sec;

                VX_matrix4_rotated( lm , lghts[l].axis , radss );
                lghts[l].curr_angle += radss;

                VX_light * ls = VX_lights_enumerate();

                double * mid = lghts[l].mid;
                float * op = ls[l].pos;
                float phaser[4] = { op[X] - mid[X] , op[Y] - mid[Y] ,
                                    op[Z] - mid[Z] , 0.0f };

                float np[4];
                VX_fvector4_multiply( np , lm , phaser );
                ls[l].pos[X] = mid[X] + np[X];
                ls[l].pos[Y] = mid[Y] + np[Y];
                ls[l].pos[Z] = mid[Z] + np[Z];
            }
        }
    }
}

void loadaoctree(){
    char * path = NULL;

    if( !(path = cui_read_path()) ){
        printf("err: bad input\n");
        return;
    }

    VX_model * m = VX_adaptive_octree_new();
    if( m->load( m , path ) != 0 ){
        printf("err: no such model path or not a valid .aoct file\n");
        m->free( m );
        free( path );
        return;
    }

    free( path );
    if( g_model )
        g_model->free( g_model );

    g_model = m;
    g_model->inspect( g_model );
}

void loadoctree(){
    char * path = NULL;

    if( !(path = cui_read_path()) ){
        printf("err: bad input\n");
        return;
    }

    VX_model * m = VX_model_octree_new();
    if( m->load( m , path ) != 0 ){
        printf("err: no such model path or not a valid .oct file\n");
        m->free( m );
        free( path );
        return;
    }

    free( path );
    if( g_model )
        g_model->free( g_model );

    const void * addressing_methods[] = {VX_oct_ray_voxel_rst_fpu , 
                                         VX_oct_ray_voxel_bcktrk_fpu,
                                         VX_oct_ray_voxel_sstack_fpu};
    printf("Select traversal method:\n\t(1) kD-restart\n\t(2) kD-backtrack\n\t(3) kD-shortstack (half stack length)\n> ");
    int choice = 1;

    if( !scanf( "%u" , &choice ) ){
        printf("warn: bad input defaulting to option 3\n");
        choice = 3;
    }

    if( choice <= 0 || choice > 3 ){
        printf("warn: bad input defaulting to option 3\n");
        choice = 3;
    }

    choice--;
    m->ray_voxel = addressing_methods[choice];
    m->inspect( m );
    g_model = m;    
}

VX_uint32 toGRAY32( VX_format * self , VX_byte * chunk ){
    if( *chunk > 145  ){
        return 0x00000000 | *chunk << 16 | *chunk << 8 | *chunk;
    }

    return self->colorkey;
}

VX_uint32 ARGB32toARGB32( VX_format * self , VX_byte * chunk ){
    VX_uint32 out = *((VX_uint32*)chunk);
    return out;
}

VX_uint32 TCtoARGB32( VX_format * self , VX_byte * chunk ){
    /* warn: endianity problem */
    VX_uint32 out = 0 | chunk[0] << 16 | chunk[1] << 8 | chunk[2];
    return out;
}

VX_format FMT_255 = {8 , 1 , 3 , 3 , 2 , 0 , 0x0 , toGRAY32 };
VX_format FMT_ARGB32 = {32,4,8,8,8,8,0, ARGB32toARGB32 };
VX_format FMT_TRUECOLOR = {25,3,8,8,8,0 ,0 , TCtoARGB32 };

void loadraw(){
    char * path = NULL;

    if( !(path = cui_read_path()) ){
        printf("err: bad input\n");
        return;
    }

    VX_block b;
    memset( &b , 0 , sizeof(VX_block) );

    printf("enter size of octree\nWarning: bad values may lead to unexpected behavior!\n");
    printf("width: ");
    int ret = 0;
    ret += scanf("%i" , &(b.w));
    printf("height: ");
    ret += scanf("%i" , &(b.h));
    printf("depth: ");
    ret += scanf("%i" , &(b.d));

    if( ret != 3 )
    {
        printf("bad dimensions\n");
        free( path );
        return;
    }

    printf("select format: \n\t(1) Gray tones\n\t(2) True-color\n\t(3) ARGB32\n> ");

    int fmt = 0;

    if( !scanf("%u" , &fmt ) || (fmt < 1 || fmt > 3) ){
        printf("bad format selection\n");
        free( path );
        return;
    }

    VX_format * f[] = { &FMT_255 , &FMT_TRUECOLOR , &FMT_ARGB32 };
    VX_model * r = VX_model_raw_new( f[fmt-1] , b );
    if( r->load( r, path ) != 0 ){
        printf("err: loading such file %s\n" , path );
        r->free( r );
        free( path );
        return;
    }

    free( path );

    if( g_model )
        g_model->free( g_model );

    r->inspect( r );    
    g_model = r;
}

int read_fpoint3( float p[3] ){
    printf("enter vector/point components in X Y Z order\n");
    if( scanf("%f %f %f" , &(p[X]) , &(p[Y]) , &(p[Z])) != 3 ){
        printf("err: wrong input\n");
        return -1;
    }
    return 0;
}

int read_dpoint3( double p[3] ){
    printf("enter vector/point components in X Y Z order\n");
    if( scanf("%lf %lf %lf" , &(p[X]) , &(p[Y]) , &(p[Z])) != 3 ){
        printf("err: wrong input\n");
        return -1;
    }
    return 0;
}

void set_camera(){
    printf("Options:\n\t(1) Reset to default\n\t(2) Set position\n\t(3) Set matrix\n");
    VX_uint32 scan = 0;

    if( scanf("%u" , &scan) != 1 ){
        printf("err: bad input\n");
        return;
    }

    switch( scan ){
    case 1:
        VX_matrix4_identity( g_camera->rotation_matrix );
        memset( &(g_camera->position) , 0 , sizeof(VX_ipoint3) );
        break;
    case 2:
        printf("Enter new camera position\n");
        float pos[3];
        read_fpoint3( pos );
        g_camera->position.x = (int)pos[X];
        g_camera->position.y = (int)pos[Y];
        g_camera->position.z = (int)pos[Z];        
        break;
    case 3:
        printf("Add three matrix columns (defualt is (1,0,0),(0,1,0),(0,0,1))\n");
        printf("info: an orthonormal basis is highly recommended\notherwise you should know what you are doing\n");
        printf("note: vectors will be normalized\n");
        double b1[3];
        double b2[3];
        double b3[3];

        read_dpoint3( b1 );
        read_dpoint3( b2 );
        read_dpoint3( b3 );
        basis( g_camera->rotation_matrix , b1 , b2 , b3 );        
        break;
    default:
        printf("err: bad option\n");
        break;
    }
}

const char * helpmsg = "Commands are:\n"
    "quit - quits testapp\n"
    "load_octree - loads octree from .oct file\n"
    "load_aoctree - loads adaptive octree from .aoct file\n"
    "load_raw - loads raw from a raw file\n"
    "set_camera - sets camera arguments\n"
    "help - prints this message\n"
    "toggle_fullscreen - toggles fullscreen mode at next run\n"
    "list_lights - lists dynamic lights\n"
    "add_light - adds a dynamic light\n"
    "remove_light - remove a dynamic light\n"
    "set_ambient - sets ambient light's color\n"
    "inspect_ambient - prints current ambient light's color\n"
    "run - runs graphical rendering\n"
    "benchmark - prints some benchmarking info\n"
    "save_demo - saves last run path and lighting conditions\n"
    "load_demo - load a demo file\n"
    "run_demo - runs demo on current model\n"
    "\nThe graphical part runs in a window, moving camera is done\n"
    "by mouse and by keys w,a,s,d, accelerating is done by SHIFT key.\n"
    "Use ESC to go back to command line UI\n\n"
    "WARNING: running of graphical part may produce extreme heat\n"
    "please make sure your computer is well cooled before!";

void help(){
    printf( "%s" , helpmsg );
}

void toggle_fullscreen(){
    g_fscreen_p = !g_fscreen_p;
    if( g_fscreen_p )
        printf("info: fullscreen is now ON\n");
    else
        printf("info: fullscreen is now OFF\n");
}

void print_rgb( VX_uint32 argb ){
    VX_byte * c = (VX_byte*)&argb;
    printf("ARGB< %u , %u , %u , %u >\n" , c[3] , c[2] , c[1] , c[0]);
}

VX_uint32 read_rgb(){
    printf("Enter ARGB color by its components:\n");
    VX_uint32 comp[4];
    int ret = scanf("%u %u %u %u" , &(comp[3]) , &(comp[2]) ,
                    &(comp[1]) , &(comp[0]) );

    if( ret != 4 )
    {
        printf("err: bad input defaulting to black\n");
        return 0;
    }


    for( int i = 0 ; i < 4 ; i++ ){
        if( comp[i] > 255 )
        {
            printf("err: components must be lower than 256, defaulting to black\n");
            return 0;
        }
    }

    return (comp[3] << 24) | (comp[2] << 16) | (comp[1] << 8) | comp[0];
}

void list_lights(){
    VX_light * l = VX_lights_enumerate();
    int cnt = VX_lights_count();

    for( int i = 0 ; i < cnt ; i++ ){
        printf("Light index (%d)\n" , i);
        print_rgb( l[i].clr );
        PRINT_RAW_FPOINT3( (l[i].pos) );
        printf("------------------\n");
    }
}

void add_light(){
    /* sorry for spaghetti code */
    printf("Light creation\n");
    VX_uint32 c = read_rgb();
    float pos[3];

    printf("Enter light position:\n");

    if( read_fpoint3( pos ) ){
        printf("err: light creation failed\n");
        return;
    }

    VX_light l = { {pos[X] , pos[Y] , pos[Z]} , c };;

    VX_light_dynamic_add( &l );
    lghts = realloc( lghts , VX_lights_count() * sizeof(s_light) );

    double mid[3];
    double ax[3];
    double rs = 0.0f;

    printf("Should be this light truly dynamic? [1,0]\n");
    int inp;

    if( !scanf( "%d" , &inp ) ){
        goto fld;
    }
    if( inp ){
        printf("Enter radius midpoint:\n");
        if( read_dpoint3( mid ) )
            goto fld;

        printf("Enter rotation axis:\n");
        if( read_dpoint3( ax ) ){
            goto fld;
        }

        VX_dvector3_normalize( ax );
        printf("Enter rotation speed in radians/second\n");
        if( !scanf( "%lf" , &rs ) )
            goto fld;

        goto succ;
    }
fld:
    memset( mid , 0 , sizeof(double)*3 );
    memset( ax , 0 , sizeof(double)*3 );
    rs = 0.0f;
succ:

    lghts[ VX_lights_count() -1 ] = (s_light){ {mid[X],mid[Y],mid[Z]},
                                               {ax[X],ax[Y],ax[Z]},
                                               rs , 0.0f };
}

void remove_light(){
    int cnt = VX_lights_count();
    if( cnt < 1 )
    {
        printf("info: no dynamic light - can not remove any\n");
        return;
    }

    printf("Insert index from range 0...%d\n" , cnt-1 );
    VX_uint32 idx = 0;
    if( !scanf("%u" , &idx ) ){
        printf("err: bad input\n");
        return;
    }

    if( idx >= cnt )
    {
        printf("err: bad index\n");
        return;
    }

    VX_light_remove( (int)idx );
    int after = (cnt-idx-1);

    if( after > 0 ){
        memmove( &(lghts[idx]) , &(lghts[idx+1]) , (after)*sizeof(s_light) );
    }

    if( VX_lights_count() ){
        lghts = realloc( lghts , VX_lights_count() * sizeof(s_light) );
    }
    else{
        free( lghts );
        lghts = NULL;
    }
}

void set_ambient(){
    printf("Set ambient light color\n");
    VX_uint32 c = read_rgb();
    VX_ambient_light_set( c );
}

void inspect_ambient(){
    printf("Ambient light\n");
    print_rgb( VX_ambient_light() );
}

void run(){

    demo_rst();
    line_addressing();
}

void benchmark(){
    printf("Benchmark info about last session:\n\tFrames drawn: %u\n\tTime of last run was: %u\n\tAverage FPS: %f\n\tWorst time: %u ms\n\tBest time: %u ms\n" , g_bench.frames_drawn , 
           g_bench.time ,
           (float)g_bench.frames_drawn/((float)g_bench.time / 1000.0f),
           g_bench.worst_frame_dt,
           g_bench.best_frame_dt );
}

void empty(){}

/*                    abbrev callback fullname callback */
void * commands[] = { "q" , vxquit , "quit" , vxquit ,
                      "lo" , loadoctree , "load_octree" , loadoctree,
                      "loa" , loadaoctree , "load_aoctree" , loadaoctree ,
                      "lr" , loadraw , "load_raw" , loadraw ,
                      "cam" , set_camera , "set_camera" , set_camera ,
                      "h" , help , "help" , help,
                      "f" , toggle_fullscreen ,
                      "toggle_fullscreen" , toggle_fullscreen,
                      "ll" , list_lights,
                      "list_lights", list_lights,
                      "add_light" , add_light ,
                      "remove_light" , remove_light ,
                      "set_ambient" , set_ambient ,
                      "inspect_ambient" , inspect_ambient ,
                      "r" , run , "run" , run ,
                      "b" , benchmark , "benchmark" , benchmark ,
                      "save_demo" , demo_dump ,
                      "load_demo" , demo_read ,
                      "run_demo" , demo_run ,
                      "" , empty ,
                      NULL};

void headhole(){
    VX_model * hh = VX_model_octree_new();
    hh->load( hh , "../data/myhead.oct" );

    VX_oct_fill_region( hh , 
                        (VX_ms_block){154,0,0,154+128,234,128},
                        0x00000000 );

    hh->dump( hh , "../data/myheadhole.oct" );
    hh->free( hh );
    exit( 0 );
}

int main( int argc , char * argv[] ){
//    headhole();

    VX_uint32 res[2] = {640 , 480};
    int fsp = 0;
    int arg = 1;

    while( arg < argc ){
        if( !strcmp( argv[arg] , "-g" ) ||
            !strcmp( argv[arg] , "--gpu" )){
            fsp = 1;
            arg++;
        }
        else if( !strcmp( argv[arg] , "-r" ) ||
                 !strcmp( argv[arg] , "--resolution" )){
            arg++;
            if( arg >= argc ){
                printf("err: parsing arguments\n");
                goto help;
            }

            if( !sscanf( argv[arg] , "%u" , &(res[X]) ) ){
                printf("err: parsing arguments\n");
                goto help;
            }
            arg++;

            if( arg >= argc ){
                printf("err: parsing arguments\n");
                goto help;
            }

            if( !sscanf( argv[arg] , "%u" , &(res[Y]) ) ){
                printf("err: parsing arguments\n");
                goto help;
            }
            arg++;
        }
        else if( !strcmp( argv[arg] , "-h" ) ||
                 !strcmp( argv[arg] , "--help" ) ){
        help:
            printf("The VoXen test application\nWritten by Pavel Prochazka (C) 2011-2013\n\nUsage:\n\t--gpu, -g enables rendering on gpu via OpenCL, must be present\n\t--resolution, -r W H, where W is width of screen and H is height of screen\n\t-v, --version prints info about version and ends\n");
            return 0;
        }
        else if( !strcmp( argv[arg] , "-v" ) ||
                 !strcmp( argv[arg] , "--version" )){
            printf("Version: %s\n" , VERSION );
            return 0;
        }
        else{
            printf("err: unexpected expresion %s here\n" , argv[arg] );
            goto help;
        }
    }

    init( res[X] , res[Y] );

    char * cmd = NULL;
    size_t cmd_len = 0;
    int len = 0;
    printf("> ");

    while( (len = getline( &cmd , &cmd_len , stdin )) != -1 ){
        cmd[len-1] = '\0';

        void ** cmds = commands;
       
        while( *cmds ){
            if( !strcmp( *cmds , cmd ) ){
                cmds++;
                void (*func)() = *cmds;
                func();
                break;
            }
            cmds +=2;
        }

        if( !*cmds ){
            printf("err: no such command %s\n" , cmd );
            help();
        }
        printf("\n> ");
    }

    vxquit();

    return 0;
}
