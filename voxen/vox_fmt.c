/*
 * Code for loading .VOX files by Ken Silverman (TODO)
*/

#include "VX_lib.h"

typedef struct vxl_header{
    VX_int32 magic_number, width, height;
    VX_dpoint3 cam, right_vect, down_vect, forward_vect;
}vxl_header;

VX_uint32 filelen( FILE * f ){
    VX_uint32 init_pos = ftell( f );
    fseek( f , 0 , SEEK_END );
    VX_uint32 out = ftell( f );
    fseek( f , init_pos , SEEK_SET );
    return out;
}

VX_uint32 vxlARGB32( VX_format * self , VX_byte * chunk ){
    VX_uint32 out = *((VX_uint32*)chunk);
    return out;
}

VX_format FMT_vxlARGB32 = {32,4,8,8,8,8,0, vxlARGB32 };

/* Ken Silverman's .vxl loader */
VX_int32 VX_vxlfmt_load(VX_model * m , const char * path){

    FILE * f = fopen( path , "rb" );
    if( !f ){
        printf("err: no such file %s\n" , path );
        return -1;
    }

    vxl_header h;

    if( fread( &h , sizeof( vxl_header ) , 1 , f ) == 0 ){
        fclose( f );
        return -1;
    }
    
    if( h.magic_number != 0x09072000 || 
        h.width != 1024 || h.height != 1024 ){ 
        printf("warn: file %s is not proper vxl format\n" , path );
        fclose( f );
        return -1;
    }

    VX_uint32 len = filelen( f ) - sizeof( vxl_header );
    VX_byte * vbuf = (VX_byte*)malloc( len );
    VX_byte * v = vbuf;

    if( fread( vbuf , len , 1 , f ) == 0 ){
        free( vbuf );
        fclose( f );
        return -1;
    }

    fclose( f );

    /* force .vxl color fmt and TODO: size */
    m->fmt = &FMT_vxlARGB32;

    for( int y = 0 ; y < 1024 ; y++ ){
        for( int x = 0 ; x < 1024 ; x++ ){
            int z = 0;

            while( 1 ){
                for( int i = z ; i <= v[1] ; i++ ){
                    /* first is information about free space */
                    m->set_voxel( m , x , y , i , 0x0 );
                }
                /* read the solid area ... 
                   color information is behind v[3] */
                for( z = v[1] ; z <= v[2] ; z++ ){
                    VX_uint32 clr = *(VX_uint32*)&v[(z-v[1]+1) * 4];
                    m->set_voxel( m , x , y , z , clr );
                }

                if( !v[0] ){ break; }

                z = v[2] - v[1] - v[0] + 2;
                v += v[0]*4;

                for( z+= v[3] ; z < v[3] ; z++ ){
                    VX_uint32 clr = *(VX_uint32*)&v[(z-v[3]) * 4];
                    m->set_voxel( m , x , y , z , clr );
                }

            }
            v += ((VX_uint32)v[2] - (VX_uint32)v[1] + 2) * 4;
        }
    }

    free( vbuf );

    return 0;
}
