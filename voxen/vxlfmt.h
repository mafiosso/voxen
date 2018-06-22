#ifndef VXL_FMT_H
#define VXL_FMT_H

/**
 * @file vxlfmt.h
 */


/** Loader of Ken Silverman's .vxl format, thanks to Ken.
 * \param m destination model (octree will be very slow ~5-10 minutes!)
 * \param path to a valid .vxl file
 * \return 0 on success, not 0 when fail and let m unmodified
*/
VX_int32 VX_vxlfmt_load(VX_model * m , const char * path);

#endif
