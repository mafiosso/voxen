#ifndef VOXEL_H
#define VOXEL_H

/**
 * @file voxel.h
 */

/* Interface for variants of octree */
#define VX_node void *

/** VX_model is general interface for implementing various models and doing raytracing on them. */
typedef struct VX_model{
    /** \var root some kind of data entry point to model */
    VX_node root;
    /** \var chunk_ptr usually buffered data */
    void * chunk_ptr;
    /** \var dim_size size of model */
    VX_block dim_size;
    /** \var fmt color format of model */
    VX_format *fmt;

    /** Compiles model m into model self (meaning is to copy one model to other - the models can be different type).
        \param self
        \param m arbitrary model to compile into self
        \return 0 on success, not 0 on fail
     */
    int (*compile)(struct VX_model * self , struct VX_model * m );

    /** Saves model self into file named file.
     * \param self
     * \param path to file which will be created or rewritten.
     * \return 0 on success, not 0 on fail
     */
    int (*dump)(struct VX_model * self , const char * file );

    /** Prints some information and stats about model on the stdout 
     * \param self
     */
    void (*inspect)(struct VX_model * self);

    /** Obtains pointer to color of voxel placed on position <x,y,z> 
     *  \param self
     *  \param x
     *  \param y
     *  \param z
     *  \param lod input/output specifies level of detail, in some models (raw) must not be implemented, but for other models should be big enough. Output is then *lod minus (how much levels was traversed).
     *   \return pointer to color
     */
    VX_byte* (*get_voxel)(struct VX_model * self , VX_uint32 x , VX_uint32 y , VX_uint32 z , int * lod );

    /** Sets voxel at <x,y,z> with color. 
        \param self
        \param x
        \param y
        \param z
        \param color
    */
    void (*set_voxel)(struct VX_model * self , VX_uint32 x , VX_uint32 y , VX_uint32 z , VX_uint32 color );

    /** Doing ray trace for ray, that starts in position, with vector norm_vect and with lod limit.
     *  \param self
     *  \param position ray starting point
     *  \param norm_vect firection of ray
     *  \param end_point unused
     *  \param lod level of detail, may restrict using small voxels (quality drop occurs then)
     *  \return color in ARGB32 format
     */
    VX_uint32 (*ray_voxel)( struct VX_model * self , float * position ,
                            float * norm_vect , float * end_point ,
                            int lod );

    /** Doing raytracing for camera on model self and on area specified as clip_rect. This method may be replaced any other well implementing presenting process (For example OpenCL implementation).
        \param self
        \param camera
        \param clip_rect specifies area of framebuffer which should be redrawn
     */
    void (*present)( struct VX_model * self , VX_camera * ,
                     VX_rect clip_rect );

    /** Loads model from file path. 
        \param self
        \param path path to a file
        \return 0 on success, not 0 on fail
     */
    int (*load)(struct VX_model * self , const char * path );

    /** Correctly frees memory allocated by model.
        \param self        
     */
    void (*free)(struct VX_model * self );

    /** User data */
    void * data;
}VX_model;

#endif
