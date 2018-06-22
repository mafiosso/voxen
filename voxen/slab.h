#ifndef SLAB_H
#define SLAB_H

/**
 * @file slab.h 
*/

/** Returns sorting permutation of floating point vector dir,
    the index with the biggest value will be in the third byte of returned value, the index with the second biggest value will be in the second byte, and index with the least value is in the first byte of returned value.
    \param dir floating point 3D vector
    \return sorting permutation, where zero byte is index of the least value, byte 1 is index of the second biggest value and byte 2 holds the index with the biggest value.    
*/
extern VX_uint32 (*VX_line_sort)( VX_fpoint3 dir );

/** Prints information about sorting permutation to stdout 
 * \param p a valid sorting permutation
 */
void VX_permutation_inspect(VX_uint32 p );

/** Computes output point for a ray from cube 
 *  defined by box_min and box_max points in mostly integer arithmetics
 * \param box_min integer raw point representing minimal box point
 * \param box_max integer raw point representing maximal box point
 * \param point float raw point representing position of observer
 * \param ray float raw direction of ray
 * \param out output raw float point
 * \param line_perm sorting permutation of ray given by VX_line_sort
 * \return -1 if no intersection found, 0-2 which is index in which intersection occured
*/
int VX_cube_out_point( int box_min[3] , int box_max[3] ,
                       VX_fpoint3 * point,
                       VX_fpoint3 * ray ,
                       float out[3],
                       VX_byte line_perm[3] );


/** Computes output point for a ray from cube 
 *  defined by box_min and box_max points in floating point arithmetics
 * \param box_min integer raw point representing minimal box point
 * \param box_max integer raw point representing maximal box point
 * \param point float raw point representing position of observer
 * \param ray float raw direction of ray
 * \param out output raw float point
 * \param line_perm sorting permutation of ray given by VX_line_sort
 * \return -1 if no intersection found, 0-2 which is index in which intersection occured
*/
int VX_cube_out_point_fpu( float box_min[3] , float box_max[3] ,
                           VX_fpoint3 * point,
                           VX_fpoint3 * ray ,
                           float out[3],
                           VX_byte line_perm[3] );

/** Computes input point for a ray to a cube 
 *  defined by box_min and box_max points in floating point arithmetics
 * \param box_min integer raw point representing minimal box point
 * \param box_max integer raw point representing maximal box point
 * \param point float raw point representing position of observer
 * \param ray float raw direction of ray
 * \param out output raw float point
 * \param line_perm sorting permutation of ray given by VX_line_sort
 * \return -1 if no intersection found, 0-2 which is index in which intersection occured
*/
int VX_cube_in_point_fpu( float box_min[3] , float box_max[3] ,
                          float point[3],
                          float ray[3] ,
                          float out[3],
                          VX_byte line_perm[3] );

#endif
