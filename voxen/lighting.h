#ifndef LIGHTING_H
#define LIGHTING_H

/**
 * @file lighting.h
 */

/** Dynamic light structure */
typedef struct VX_light{
    /** \var pos position of the dynamic light */
    float pos[3];
    /** \var clr color of light*/
    VX_uint32  clr;
}VX_light;

/** Adds a dynamic light to scene.
    \param l valid pointer to light, invalid may lead to unexpected behavior of raytracing (NULL is also invalid pointer)
    \return index/identifier of light    
*/
int VX_light_dynamic_add( VX_light * l );

/** Removes dynamic light from scene identified by idx.
    \param idx identifier of light, if invalid, no light will be removed.
 */
void VX_light_remove( int idx );
/** Removes all lights in scene.   
*/
void VX_lights_clear();

/** Obtains number of lights in scene.
    \return count of lights
*/
VX_uint32 VX_lights_count();

/** Enumerates all dynamic lights in scene by returning array of lights
    \return array of lights, NULL will be returned iff VX_lights_count() returns 0;
*/
VX_light * VX_lights_enumerate();

/** Sets ambient color for current scene (default is 0xffffffff)
    \param mask color of ambient light in ARGB32 format
 */
void VX_ambient_light_set( VX_uint32 mask );

/** Inspects current ambient light.
    \return ARB32 color of ambient light
*/
VX_uint32 VX_ambient_light();

#endif
