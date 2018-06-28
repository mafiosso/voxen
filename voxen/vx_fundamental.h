#ifndef FUND_H
#define FUND_H

/**
 * @file vx_fundamental.h
 */

#define FAST_AND &
#define FAST_OR  |

/*! Ludolf constant. */
#define PI 3.14159265
/*! Print VX_fpoint3. */
#define PRINT_POINT3( X ) {printf("p( %f , %f , %f )\n" , (float)X.x ,(float)X.y,(float)X.z );}
/*! Print array of 3 integers. */
#define PRINT_RAW_POINT3( P ) {printf("p_raw( %i , %i , %i )\n" , P[X] , P[Y], P[Z] );}
/** Print array of 3 floats. */
#define PRINT_RAW_FPOINT3( P ) {printf("p_raw( %f , %f , %f )\n" , P[X] , P[Y], P[Z] );}

/** Print a VX_ms_block structure. */
#define PRINT_MS_BLOCK( B ) {printf("ms_block( %d , %d , %d , %d , %d , %d )\n" , B.x1 , B.y1 , B.z1 , B.x2 , B.y2 , B.z2 );}
/**  Print a VX_block structure. */
#define PRINT_BLOCK( B ) {printf("block( %d , %d , %d , %d , %d , %d )\n" , B.x , B.y , B.z , B.w , B.h , B.d );}
/** Conversion VX_block to VX_ms_block. */
#define BOX_TO_MS_BLOCK( BOX ) (VX_ms_block){ BOX.x, BOX.y, BOX.z , BOX.x + BOX.w , BOX.y + BOX.h , BOX.z + BOX.d }
/** Conversion VX_ms_block to VX_block. */
#define MS_BLOCK_TO_BOX( BLO ) (VX_block){ BLO.x1 , BLO.y1 , BLO.z1 , BLO.x2 - BLO.x1 , BLO.y2 - BLO.y1 , BLO.z2 - BLO.z1 }
/** Print VX_cube structure. */
#define PRINT_CUBE( C ){ printf("cube( %d , %d , %d , .w = %d )\n" , C.x , C.y , C.z , C.w ); }
/** Checking if INSIDE_P lies inside interval [LEFT,RIGHT]*/
#define CHECK_INTERVAL( INSIDE_P , LEFT , RIGHT ) \
    (INSIDE_P <= RIGHT && INSIDE_P >= LEFT)

/** Applies operation op on all items of 3-item array lvar and rvar */
#define APPLY3( lvar , rvar , op )                              \
    lvar[X] op rvar[X]; lvar[Y] op rvar[Y]; lvar[Z] op rvar[Z]  \
                                                                \
/** Compute vector cross product a is output 3-item array
 *  b and c are input 3-item vectors encoded by arrray
 */
#define CROSS_PRODUCT(a,b,c)                        \
    (a)[0] = (b)[1] * (c)[2] - (c)[1] * (b)[2];     \
       (a)[1] = (b)[2] * (c)[0] - (c)[2] * (b)[0];  \
       (a)[2] = (b)[0] * (c)[1] - (c)[0] * (b)[1];  \

/** Constant of maximal allocable bytes - 3GB
 */
#define MAX_ALLOCABLE 0x80000000

#define VX_int32  int32_t
#define VX_int16 int16_t
#define VX_uint16 uint16_t

#define VX_byte   uint8_t

#define VX_uint32 uint32_t
#define VX_uint64 uint64_t

/** Index X in 3-item vector array */
#define X 0
/** Index Y in 3-item vector array */
#define Y 1
/** Index Z in 3-item vector array */
#define Z 2

/** Index A in 4-item color array */
#define A 3
/** Index R in 4-item color array */
#define R 2
/** Index G in 4-item color array */
#define G 1
/** Index B in 4-item color array */
#define B 0

/** Maximum macro. */
#define MAX( x , y ) (x > y ? x : y)
/** Minimum macro. */
#define MIN( x , y ) (x < y ? x : y)
/** Faster signum by using type punning - returns 1 << 31 if N is negative 0 otherwise. */
#define FLOAT_SIGNUM( N ) (*((VX_uint32*)((void*)&N)) & (0x1 << 31))

/** Structure representing a 2D rectangle,
    <x,y> is left-top corner and w is width and h is height.
 */
typedef struct VX_rect{
		VX_int32 x,y,w,h;
}VX_rect;

/** Structure representing cube in 3D,
 *  <x,y,z> is left bottom near corner,
 *  w is width.
 */
typedef struct VX_cube{
  VX_int32 x,y,z,w;
}VX_cube;

/** Structure representing cube in 3D
 *  <x,y,z> is left bottom near corner,
 *   <w,h,d> is width, height and depth of cube.
 */
typedef struct VX_block{
    VX_int32 x,y,z,w,h,d;
}VX_block;

/** Structure representing block in 3D in Microsoft way
 *  (like RECT from WinAPI) <x1,y1,z1> is minimal corner and
 * <x2,y2,z2> is maximal corner.
 */
typedef struct VX_ms_block{
    VX_int32 x1,y1,z1,x2,y2,z2;
}VX_ms_block;

/** Structure representing integer point in 2D. */
typedef struct VX_ipoint2{
    int x, y;
}VX_ipoint2;

/** Structure representing integer line in 2D. */
typedef struct VX_iline2{
    VX_ipoint2 p1, p2;
}VX_iline2;

/** Structure representing integer point in 3D. */
typedef struct VX_ipoint3{
    int x,y,z;
}VX_ipoint3;

/** Structure representing float point in 2D. */
typedef struct VX_fpoint2{
    float x,y;
}VX_fpoint2;

/** Structure representing float point in 3D. */
typedef struct VX_fpoint3{
    float x,y,z;
}VX_fpoint3;

/** Structure representing integer line in 3D. */
typedef struct VX_iline3{
    VX_ipoint3 p1,p2;
}VX_iline3;

/** Structure representing float line in 3D. */
typedef struct VX_fline3{
    VX_fpoint3 p1,p2;
}VX_fline3;

/** Structure representing double point in 2D. */
typedef struct VX_dpoint2{
    double x,y;
}VX_dpoint2;

/** Structure representing double point in 3D. */
typedef struct VX_dpoint3{
    double x,y,z;
}VX_dpoint3;

/** Structure representing double line in 3D. */
typedef struct VX_dline3{
    VX_dpoint3 p1,p2;
}VX_dline3;

/** Structure representing color format. */
typedef struct VX_format{
    /** count of bits used by one pixel. */
    VX_byte bit_pp;
    /** count of bytes used by pixel */
		VX_byte byte_pp;
    /**  count of bits used for red */
    VX_uint32 Rsz;
    /**  count of bits used for green */
    VX_uint32  Gsz;
    /**  Bsz count of bits used for blue */
    VX_uint32 Bsz;
    /**  Asz count of bits used for alpha */
		VX_uint32 Asz;
    /** colorkey is colorkey value used as transparent color */
    VX_uint32 colorkey;
    /** Conversion function, from this format to ARGB32 standard format,
     *  this function must be set at least once by user.
     *  \param self this pointer
     *  \param ptr  pointer to color
     *  \return valid ARGB32 color
     */
    VX_uint32 (*ARGB32)( struct VX_format * , VX_byte * ptr );
}VX_format;

/** 2D surface device independent interface. */
typedef struct VX_surface{
    /** Destructor of surface.
        \param self
     */
    void      (*free)( struct VX_surface * );
    /** Returns pixel in ARGB32 format from x, y position. 
     *  \param self
     *  \param x x position of pixel
     *  \param y y position of pixel
     *  \return ARGB32 color
     */
    VX_uint32 (*get_pixel)( struct VX_surface * , VX_uint32 x , VX_uint32 y);
    /** Locks surface for writing, returns raw pointer to image data. 
     *  \param self
     *  \return raw pointer to image data
     */
    void *    (*lock_surface)( struct VX_surface * );
    /** Unlocks surface for writing, returns raw pointer to image data. 
     *  \param self
     */
    void      (*unlock_surface)( struct VX_surface * );
    /** Sets pixel to color (ARGB32 format) on x, y position. 
     *  \param self
     *  \param x x position of pixel
     *  \param y y position of pixel
     *  \param color valid ARGB32 color
     */
    void      (*set_pixel)(struct VX_surface * self , VX_uint32 x , VX_uint32 y , VX_uint32 color );

    /** width of surface */
    VX_uint32 w;
    /** height of surface */
    VX_uint32 h;
    /** pointer to device dependent surface,
     *       user may modify this only if writing driver
    */
    void * native_surf;

    /**  pointer for any other data used by driver,
     *       user may modify this only if writing driver
     */
    void * custom_dta;
}VX_surface;

/** Print matrix on stdout.
 *  \param m matrix in format double[4][4]
 */
void VX_matrix4_print( double m[4][4] );

/** Print vecotr on stdout.
 *  \param v vector in format float[4]
 */
void VX_vector4_print( float v[4] );

/** Multiplies matrix matrix with vector, like this matrix*vector and stores output in outv.
 *  \param outv output vector in format float[4]
 *  \param matrix matrix in format double[4][4]
 *  \param vector vector in format float[4]
 */
void VX_fvector4_multiply( float outv[4] , double matrix [4][4] ,
                           float vector[4] );

/** Multiplication of two matrices m1 and m2 storing output to out
    multiplication is done in order out = m1 . m2.
 * \param out output matrix
 * \param m1 left matrix
 * \param m2 right matrix
 */
void VX_matrix4_multiply( double out[4][4] , double m1[4][4] ,
                          double m2[4][4] );

/** Addition of two matrices m1 and m2 storing output to out
    addition is done in order out = m1 + m2.
 * \param out output matrix
 * \param m1 left matrix
 * \param m2 right matrix
 */
void VX_matrix4_add( double out [4][4] , double m1 [4][4] , double m2 [4][4] );

/** Initialize matrix 4x4 out to identity matrix.
 *  \param out output matrix
 */
void VX_matrix4_identity( double out [4][4] );

/** Initialize matrix to a rotation matrix around x axis (left handed system).
    \param out output matrix
    \param rad_angle angle of rotation in radians
*/
void VX_matrix4_xrotation( double out[4][4] , double rad_angle );
/** Initialize matrix to a rotation matrix around y axis (left handed system).
    \param out output matrix
    \param rad_angle angle of rotation in radians
*/
void VX_matrix4_yrotation( double out[4][4] , double rad_angle );

/** Initialize matrix to a rotation matrix around z axis (left handed system).
    \param out output matrix
    \param rad_angle angle of rotation in radians
*/
void VX_matrix4_zrotation( double out[4][4] , double rad_angle );

/** Converts radians to degrees.
 * \param rad angle in radians
 * \returns angle in degrees
*/
double rad_to_degrees (double rad);

/** Converts degrees to radians.
 * \param deg angle in degrees
 * \returns angle in radians
*/
double deg_to_radians (double deg);

/** Compute log_2(a) in integer precision.
    \param a an unsigned integer
    \return log_2(a)    
*/
VX_uint32 nearest_pow2( VX_uint32 a );

int ilog2( VX_uint32 a );

/** Test if point p lies inside VX_ms_block b.
 * \param b a valid VX_ms_block
 * \param p integer point in 3D
 * \return 1 if p lies inside b, 0 otherwise.
*/
int VX_point_in_cube_p( VX_ms_block b , VX_ipoint3 p );

/** Test if two blocks b1 and b2 intersects.
 * \param b1 valid VX_ms_block
 * \param b2 valid VX_ms_block
 * \return 1 if intersection occurs, 0 otherwise.
 */
int VX_cubes_intersects_p( VX_ms_block b1 , VX_ms_block b2 );

/** Returns block of intersection between two blocks b1 and b2,
 *  blocks b1 and b2 must overlap.
 * \param b1 a valid VX_ms_block
 * \param b2 a valid VX_ms_block
 * \return block of intersection between b1 and b2.
*/
VX_ms_block VX_cubes_intersection( VX_ms_block b1 , VX_ms_block b2 );

/** Makes vector v of length len 
 * \param input/output vector
 * \param len needed length of vector (commonly 1.0f)
*/
void fvector3_raw_normalize( float v[3] , float len );

/** Adds two ARGB32 colors together by components, 
    values are checked for not to overlap 255.
    \param c1 ARGB32 color
    \param c2 ARGB32 color
    \return ARGB32 color created by adition c1 and c2.
*/
VX_uint32 color_add( VX_uint32 c1 , VX_uint32 c2 );

/** Multiplies two ARGB32 colors together by components, 
    values never overlaps 255 due to modular arithmetics.
    \param c1 ARGB32 color
    \param c2 ARGB32 color
    \return ARGB32 color created by adition c1 and c2.
*/
VX_uint32 color_mul( VX_uint32 c1 , VX_uint32 c2 );

#endif
