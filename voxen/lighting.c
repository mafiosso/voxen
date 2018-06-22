#include "VX_lib.h"


/* TODO?: stick to VX_lib object? */

static VX_uint32 amb_light = 0xffffffff;
static VX_light * lights = NULL;
static int lights_count   = 0;
static int lights_len     = 0;

int VX_light_dynamic_add( VX_light * l ){
    lights_count++;

    if( lights_count > lights_len ){
        lights_len = lights_count * 2;

        lights = realloc( lights ,  sizeof( VX_light ) * lights_len );
    }

    int out = lights_count-1;

    lights[ lights_count-1 ] = *l;
    return out;
}

void VX_light_remove( int idx ){
    if( idx >= lights_count || idx < 0 )
        return;

    int a = lights_count - idx - 1;
    if( a > 0 ){
        memmove( &(lights[ idx ]) , &(lights[ idx+1 ]) ,
                 a*sizeof(VX_light) );
    }

    lights_count--;
}

void VX_lights_clear(){
    free( lights );
    lights = NULL;
    lights_count = 0;
    lights_len = 0;
}

VX_uint32 VX_lights_count(){
    return lights_count;
}

VX_light * VX_lights_enumerate(){
    return lights;
}

void VX_ambient_light_set( VX_uint32 mask ){
    amb_light = mask;
}

VX_uint32 VX_ambient_light(){
    return amb_light;
}
