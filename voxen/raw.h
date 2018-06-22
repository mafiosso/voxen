#ifndef RAW_H
#define RAW_H

/**
 * @file raw.h
 */

/** Constructor for raw models. Raw model has some limitations, the load argument has no impact, set_voxel and get_voxel should write only voxels in respect to model size (otherwise expect undefined behavior.
    \param fmt valid pointer to VX_format, any other will lead to unexpected behavior
    \param size size of raw model, only w, h, d values will be used
    \return valid VX_model pointer on success, NULL when fail
*/
VX_model * VX_model_raw_new( VX_format * fmt , VX_block size );

#endif
