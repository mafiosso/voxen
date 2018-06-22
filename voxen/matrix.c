#include <math.h>
#include "VX_lib.h"

void VX_matrix4_print( double m[4][4] )
{
    for( int y = 0 ; y < 4 ; y++ )
    {
        printf("|");
        for( int x = 0; x < 4 ; x++ )
        {
            printf(" %f" , m[y][x] );
        }
        printf(" |\n");        
    }
}

void VX_vector4_print( float v[4] )
{
    printf("v4|");
    for( int x = 0 ; x < 4 ; x++ )
    {
        printf(" %f" , v[x] );
    }

    printf(" |\n");
}

void VX_dvector4_multiply( double outv[4] , double matrix [4][4] , double vector[4] )
{
    for( int i = 0; i < 4 ; i++ )
    {
        float sum = 0.0f;
        
        for( int y = 0 ; y < 4 ; y++ )
        {
            sum += (matrix[i][y]*vector[y]);
        }
        
        outv[i] = sum;
    }
}

void VX_dvector3_normalize( double inout[3] ){
    double len = sqrt( inout[X] * inout[X] + inout[Y]*inout[Y] + inout[Z]*inout[Z]);
    inout[X] = inout[X]/len;
    inout[Y] = inout[Y]/len;
    inout[Z] = inout[Z]/len;
}

void VX_fvector4_multiply( float outv[4] , double matrix [4][4] , float vector[4] )
{
    for( int i = 0; i < 4 ; i++ )
    {
        float sum = 0.0f;
        
        for( int y = 0 ; y < 4 ; y++ )
        {
            sum += (matrix[i][y]*vector[y]);
        }
        
        outv[i] = sum;
    }
}

void VX_matrix4_multiply( double out[4][4] , double m1[4][4] , double m2[4][4] )
{
    for(int y = 0 ; y < 4; y++)
    {
        for(int k = 0; k < 4 ; k++)
        {
            double sum = 0.0f;
            for(int x = 0 ; x < 4 ;x++)
            {
                sum +=(m1[y][x]*m2[x][k]);
            }

            out[y][k] = sum;
        }
    }
}

void VX_matrix4_add( double out [4][4] , double m1 [4][4] , double m2 [4][4] )
{
    for( int y = 0 ; y < 4 ; y++ )
    {
        for( int x = 0 ; x < 4 ; x++ )
        {
            out[y][x] = m1[y][x] + m2[y][x];
        }
    }
}

void VX_matrix4_identity( double out [4][4] )
{
    out[0][0] = 1.0f; out[0][1] = 0.0f; out[0][2] = 0.0f; out[0][3] = 0.0f;
    out[1][0] = 0.0f; out[1][1] = 1.0f; out[1][2] = 0.0f; out[1][3] = 0.0f;
    out[2][0] = 0.0f; out[2][1] = 0.0f; out[2][2] = 1.0f; out[2][3] = 0.0f;
    out[3][0] = 0.0f; out[3][1] = 0.0f; out[3][2] = 0.0f; out[3][3] = 1.0f;
}

/* rotation around x axis */
void VX_matrix4_xrotation( double out[4][4] , double rad_angle ){
    double c = cos( rad_angle );
    double s = sin( rad_angle );

    VX_matrix4_identity( out );
  
    out[1][1] = c;  out[1][2] = s;
    out[2][1] = -s; out[2][2] = c;
    /*
    out[1][1] = c;  out[1][2] = -s;
    out[2][1] = s;  out[2][2] = c;
    */

}

/* rotation around y axis */
void VX_matrix4_yrotation( double out[4][4] , double rad_angle ){
    double c = cos( rad_angle );
    double s = sin( rad_angle );

    VX_matrix4_identity( out );

    out[0][0] = c;  out[0][2] = -s;
    out[2][0] = s;  out[2][2] = c;
/*

    out[0][0] = c;  out[0][2] = s;
    out[2][0] = -s;  out[2][2] = c;
*/
}

void VX_matrix4_zrotation( double out[4][4] , double rad_angle ){
    double c = cos( rad_angle );
    double s = sin( rad_angle );

    VX_matrix4_identity( out );
    out[0][0] = c; out[0][1] = -s;
    out[1][0] = s; out[1][1] = c;
}

void VX_matrix4_rotate( double out[4][4] , float v[3] , float angle ){
    float c = cos(angle);
    float s = sin(angle);
    float t = 1.0f-c;

    out[0][0] = t*v[X]*v[X]+c;
    out[0][1] = t*v[X]*v[Y]-(s*v[Z]);
    out[0][2] = t*v[X]*v[Z]+(v[Y]*s);
    out[0][3] = 0.0f;

    out[1][0] = v[Y]*v[X]*t+(v[Z]*s);
    out[1][1] = v[Y]*v[Y]*t+c;
    out[1][2] = v[Y]*v[Z]*t-(v[X]*s);
    out[1][3] = 0.0f;

    out[2][0] = v[X]*v[Z]*t-v[Y]*s;
    out[2][1] = v[Y]*v[Z]*t+(v[X]*s);
    out[2][2] = v[Z]*v[Z]*t+c;
    out[2][3] = 0.0f;

    out[3][0] = 0.0f;
    out[3][1] = 0.0f;
    out[3][2] = 0.0f;
    out[3][3] = 1.0f;
}

void VX_matrix4_rotated( double out[4][4] , double v[3] , double angle ){
    double c = cos(angle);
    double s = sin(angle);
    double t = 1.0f-c;

    out[0][0] = t*v[X]*v[X]+c;
    out[0][1] = t*v[X]*v[Y]-(s*v[Z]);
    out[0][2] = t*v[X]*v[Z]+(v[Y]*s);
    out[0][3] = 0.0f;

    out[1][0] = v[Y]*v[X]*t+(v[Z]*s);
    out[1][1] = v[Y]*v[Y]*t+c;
    out[1][2] = v[Y]*v[Z]*t-(v[X]*s);
    out[1][3] = 0.0f;

    out[2][0] = v[X]*v[Z]*t-v[Y]*s;
    out[2][1] = v[Y]*v[Z]*t+(v[X]*s);
    out[2][2] = v[Z]*v[Z]*t+c;
    out[2][3] = 0.0f;

    out[3][0] = 0.0f;
    out[3][1] = 0.0f;
    out[3][2] = 0.0f;
    out[3][3] = 1.0f;
}

void VX_matrix4_lookat( double out[4][4] , float at[3] ,
                        float pos[3],
                        float up[3] ){
    VX_matrix4_identity( out );
    float fwd[3] = {at[X] - pos[X] , at[Y] - pos[Y] , at[Z] - pos[Z]};
    fvector3_raw_normalize( fwd , 1.0f );

    float side[3];
    CROSS_PRODUCT( side , fwd , up );
    fvector3_raw_normalize( side , 1.0f );

    float pup[3];
    CROSS_PRODUCT( pup , side , fwd );
    
    out[0][1] = side[0];
    out[1][1] = side[1];
    out[2][1] = side[2];

    out[0][2] = pup[0];
    out[1][2] = pup[1];
    out[2][2] = pup[2];

    out[0][0] = -fwd[0];
    out[1][0] = -fwd[1];
    out[2][0] = -fwd[2];
}
