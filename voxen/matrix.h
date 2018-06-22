#ifndef MATRIX_H
#define MATRIX_H
/**
 * @file matrix.h
 */

/** Normalize inout double raw vector.
 * \param inout double raw vector, after execution it is the same vector of length 1.0f
 */
void VX_dvector3_normalize( double inout[3] );

/** Multiplies matrix matrix with vector in double precision, like this matrix*vector and stores output in outv.
 *  \param outv output vector in format float[4]
 *  \param matrix matrix in format double[4][4]
 *  \param vector vector in format double[4]
 */
void VX_dvector4_multiply( double outv[4] , double matrix [4][4] , double vector[4] );

/** Initialize out to a matrix of rotation around an arbitrary axis (single precision) v and angle.
    \param out output matrix
    \param v arbitrary axis
    \param angle angle in radians
*/
void VX_matrix4_rotate( double out[4][4] , float v[3] , float angle );

/** Initialize out to a matrix of rotation around an arbitrary axis (double precision) v and angle.
    \param out output matrix
    \param v arbitrary axis
    \param angle angle in radians
*/
void VX_matrix4_rotated( double out[4][4] , double v[3] , double angle );

/*! \fn Initialize out to a matrix of orthonormal vectors, one them is directing to at coordination.
    \param out output matrix
    \param at point at we are looking
    \param pos position of camera
    \param up should be orthonormal to vector at-pos
*/
void VX_matrix4_lookat( double out[4][4] , float at[3] ,
                        float pos[3],
                        float up[3] );

#endif
