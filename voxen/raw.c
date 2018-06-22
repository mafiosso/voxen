#include "VX_lib.h"
#include <math.h>

/* TODO: broken - make from raw model 
         better format functionality (callbacks to format conversions)
*/
typedef struct raw_data{
    VX_uint32 pitchw;
    VX_uint32 pitchd;
}raw_data;

typedef struct VX_raw{
		VX_byte * chunk;
		VX_block bounding_box;
		VX_format fmt;
		VX_uint32 pitchd;
		VX_uint32 pitchw;
}VX_raw;

int VX_raw_load( VX_model * self , const char * path );
VX_byte * VX_raw_get_voxel_ptr( VX_model * self , VX_uint32 x , VX_uint32 y , VX_uint32 z );


static void raw_recompute_pitch( VX_model * self ){
    free( self->data );
    raw_data * rd = malloc(sizeof(raw_data));
    rd->pitchw = self->dim_size.w * self->fmt->byte_pp;
    rd->pitchd = rd->pitchw * self->dim_size.h;
    self->data = rd;    
}

void VX_raw_inspect (VX_model * self )
{
    printf("Raw model stats: total B used: %u\n" , self->dim_size.w * self->dim_size.h * self->dim_size.d * self->fmt->byte_pp );
}

/* TODO: make raw format more user friendly (add header with size and fmt) */
int VX_raw_load( VX_model * self,
                 const char * path )
{
	FILE * f = fopen( path , "rb" );

	if( !f )
	{
		printf("warn: raw resource %s cannot be loaded!\n" , path );
		return -1;
	}

  VX_block size = self->dim_size;
  VX_format fmt = *(self->fmt);

	VX_uint32 pixels = (size.w * size.h * size.d);
	VX_uint64 sz = pixels * fmt.byte_pp;

	if( sz > MAX_ALLOCABLE )
	{
		printf("warn: max allocable size exceeded - this file must be split\n");
		fclose( f );
		return -1;
	}

  fseek( f , 0 , SEEK_END );
  size_t fsz = ftell( f );
  rewind( f );

  if( fsz != sz ){
      printf("err: this file can not have such dimensions\n");
      printf("info: pixels count: %u, fmt byte_pp: %d\n" , pixels , fmt.byte_pp );
      fclose( f );
      return -1;
  }

	self->chunk_ptr = malloc( sz );
  
  raw_recompute_pitch( self );

	if( !fread( self->chunk_ptr , fmt.byte_pp , pixels , f ) )
	{
		printf("err: file can not been loaded!\n");
		free( self->chunk_ptr );
    self->chunk_ptr = NULL;
		fclose( f );
		return -1;
	}

	return 0;
}

int VX_raw_dump( VX_model * self , const char * path ){
    FILE * f = fopen( path , "w+" );

    if( !f ){
        printf("err: cannot dump raw - can not operate with disk\n");
        return -1;
    }

    if( !fwrite( self->chunk_ptr , sizeof(VX_byte) , self->dim_size.w * self->dim_size.h * self->dim_size.d * self->fmt->byte_pp , f ) )
    {
        printf("err: cannot write to file - not enough space?\n");
        fclose( f );
        return -1;
    }

    return 0;
}

/* reads voxel from a raw and returns standard RGB value */  
VX_byte * VX_raw_get_voxel_ptr( VX_model * self , VX_uint32 x , VX_uint32 y , VX_uint32 z )
{
    if( x < self->dim_size.w && y < self->dim_size.h && z < self->dim_size.d ){
        raw_data * d = self->data;
        return (self->chunk_ptr + (z*d->pitchd) + (y*d->pitchw) + (x*self->fmt->byte_pp));
    }

    // printf("obtaining bad address! %d , %d ,  %d\n" , x , y , z);
    return NULL;
}

int VX_raw_compile( VX_model * self , VX_model * m ){
    VX_uint32 pixels = (m->dim_size.w * m->dim_size.h * m->dim_size.d);
    VX_uint32 size = m->fmt->byte_pp * pixels;
    int lod = 0;

    if( size > MAX_ALLOCABLE )
    {
        printf("warn: max allocable size exceeded - can not compile\n");
        return -1;
    }

    self->fmt = m->fmt;
    self->dim_size = m->dim_size;
    free( self->chunk_ptr );
    self->chunk_ptr = malloc( size );
    raw_recompute_pitch( self );

    for( int z = 0 ; z < m->dim_size.d ; z++ ){
        for( int y = 0 ; y < m->dim_size.h ; y++ ){
            for( int x = 0 ; x < m->dim_size.w ; x++ ){
                lod = 0xff;
                VX_byte * ptr = self->get_voxel( self , x , y , z , &lod );
                lod = 0xff;
                VX_byte * ptr_src = m->get_voxel( m , x , y , z , &lod );
                memcpy( ptr , ptr_src , self->fmt->byte_pp );

                /*printf("byte_pp_ %d\n" , self->fmt->byte_pp );

               
                printf("%u != %u\n" , *((VX_uint32*)ptr) ,
                       *((VX_uint32*)VX_raw_get_voxel_ptr( self , x , y , z )));
                       exit(1);*/
            }
        }
    }
    
    return 0;
}

void VX_raw_free( VX_model * self )
{
    free( self->data );
    free( self->chunk_ptr );
    free( self );
}
		    
VX_byte * VX_raw_get_voxel( VX_model * self , VX_uint32 x , VX_uint32 y , VX_uint32 z , int * lod )                           
{
    return VX_raw_get_voxel_ptr( self , x , y , z );
}

static void VX_raw_set_voxel( VX_model * self ,
                              VX_uint32 x,
                              VX_uint32 y,
                              VX_uint32 z,
                              VX_uint32 color)
{
    VX_byte * ptr = VX_raw_get_voxel_ptr( self , x , y , z );
    memcpy( ptr , &color , self->fmt->byte_pp );    
}

VX_uint32 VX_raw_ray_get_voxel( VX_model * self , float dcam[3] , 
                                float n_ray[3] , float end_point[3],
                                int max_lod ){
    /* TODO: make better DDA - this so naive and buggy */
    /* n_ray is normalized so when dividing big error occurs  */
    n_ray[X] *= 1000.0f;
    n_ray[Y] *= 1000.0f;
    n_ray[Z] *= 1000.0f;

    float x = dcam[X];
    float y = dcam[Y];
    float z = dcam[Z];
    
    float xlen = dcam[X] - n_ray[X];
    int ylen = (int)(dcam[Y] - n_ray[Y]);
    int zlen = (int)(dcam[Z] - n_ray[Z]);
    float len = MAX( fabs(xlen) , MAX( fabs(ylen) , fabs(zlen) ) );

    float stepx = xlen / len;
    float stepy = ylen / len;
    float stepz = zlen / len;

    for(int i = 0; i < (int)len; i++){
        VX_byte * chunk = VX_raw_get_voxel( self , (int)x , (int)y , (int)z , &max_lod );
        if( !chunk )
            break;

        VX_uint32 color = self->fmt->ARGB32( self->fmt , chunk );
        if( color != self->fmt->colorkey ){
            return color;
        }

         x -= stepx;
         y -= stepy;
         z -= stepz;

         if( !(dcam[Z] < self->dim_size.d && dcam[Y] < self->dim_size.h && dcam[X] < self->dim_size.w &&
               dcam[X] >= 0.0f && dcam[Y] >= 0.0f && dcam[Z] >= 0.0f) ){
             break;
         }
    }

    return self->fmt->colorkey;
}

static void raw_present( struct VX_model * self , VX_camera * c,
                              VX_rect clip_rect ){

 VX_fpoint3 pos =  { (float)c->position.x , (float)c->position.y , 
                       (float)c->position.z };

    for( int y = clip_rect.y ; y < (clip_rect.y+clip_rect.h) ; y++ ){
        for( int x = clip_rect.x ; x < (clip_rect.x+clip_rect.w) ; x++ ){
            VX_fpoint3 ray = c->rg( c , x , y );
            VX_uint32 pix = self->ray_voxel( self , 
                                             (float*)&pos ,
                                             (float*)&ray , 
                                             NULL , 0xff );
            c->surface->set_pixel( c->surface , x , y , pix );            
        }
    }
}

/* allocates memory after first set_voxel call */
static void lazy_alloc(VX_model * self ,
                       VX_uint32 x,
                       VX_uint32 y,
                       VX_uint32 z,
                       VX_uint32 color)
{
    if( !self->chunk_ptr ){
        VX_block size = self->dim_size;
        VX_format fmt = *(self->fmt);
        
        VX_uint32 pixels = (size.w * size.h * size.d);
        VX_uint64 sz = pixels * fmt.byte_pp;
        
        if( sz > MAX_ALLOCABLE )
        {
            printf("warn: max allocable size exceeded - this file must be split\n");
            return;
        }
        
        self->chunk_ptr = calloc( sz , 1 );
        raw_recompute_pitch( self );
    }

    self->set_voxel = VX_raw_set_voxel;
    self->set_voxel( self , x , y , z , color );
}

VX_model * VX_model_raw_new( VX_format * fmt , VX_block size )
{
    VX_model * out = malloc(sizeof(VX_model));

    raw_data * extra_dta = (raw_data*)malloc(sizeof(raw_data));
    
    *out = (VX_model){ NULL , NULL , size , fmt ,
                       VX_raw_compile , VX_raw_dump , VX_raw_inspect,
                       VX_raw_get_voxel,
                       lazy_alloc,
                       VX_raw_ray_get_voxel,
                       raw_present,
                       VX_raw_load , VX_raw_free , extra_dta
    };
    
    return out;
}
