#ifndef CAMERA_H
#define CAMERA_H

/**
 * @file camera.h
 */

/** Structure representing camera in raytracing model. */
typedef struct VX_camera{
    /** holds position of camera */
    VX_ipoint3 position;    
    /** stores 4x4 viewing matrix, last row and column should be 0 */
    double rotation_matrix[4][4];
    /** Function that generates rays for given x and y coordinates
        \param self
        \param x x-position of camera
        \param y y-position of camera
        \return transformed VX_fpoint3 by rotation matrix for <x,y> point.
     */
    VX_fpoint3 (*rg)( struct VX_camera * self , VX_uint32 x, VX_uint32 y );
    
    /** surface output surface - framebuffer (read_only) */
    VX_surface * surface;

    /** thread object VX_field (read_only) */
    VX_field * workers;

    /** Runs raytracing for camera on model. 
     *  \param self
     *  \param model model to run raytracing on
     */
    void (*draw)( struct VX_camera * self , void * model );
    /** Frees camera object.
        \param self
     */
    void (*destroy)( struct VX_camera * self );

    /** Commonly used buffer for storing rays used by rg function */
    VX_fpoint3 * buffer;
    /** Other user data */
    void * data;
}VX_camera;

/** Constructor for VX_camera object.
    \param surf some surface of type VX_surface, can not be NULL, otherwise undefined behavior occurs
    \param ray_generator function used for generating rays, may be NULL, then standard perspective correction will be used
    \param cpus_count specifying number of threads for raytracing, when value is less than 1 is passed, then camera will create as same threads as your CPU has, otherwise the passed value will be used
    \return valid VX_camera pointer on success, NULL on fail
*/
VX_camera * VX_camera_new( VX_surface * surf ,
                           VX_fpoint3 (*ray_generator)( 
                               VX_camera * self , VX_uint32 x, VX_uint32 y ) ,
                           int cpus_count );

#endif

