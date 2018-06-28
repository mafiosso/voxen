/*
 * Code for loading .VOX files by Ken Silverman (TODO)
*/

#include "VX_lib.h"

typedef struct vxl_header{
    VX_int32 magic_number, width, height;
    VX_dpoint3 cam, right_vect, down_vect, forward_vect;
}vxl_header;

typedef struct file_content{
    VX_byte * buffer;
    VX_uint32 len;
}file_content;

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

static file_content * load_file( const char * path){
    FILE * f = fopen( path , "rb" );
    if( !f ){
        printf("err: no such file %s\n" , path );
        return NULL;
    }

    file_content * out = (file_content*)malloc( sizeof(file_content) );
    VX_uint32 len = filelen( f );
    
    out->len = len;
    out->buffer = (VX_byte*)malloc( len );
    VX_byte * vbuf = out->buffer;
    
    if( fread( vbuf , len , 1 , f ) == 0 ){
        free( vbuf );
        fclose( f );
        free( out );
        return NULL;
    }

    fclose( f );
    return out;
}


/* Ken Silverman's .vxl loader */
VX_int32 VX_vxlfmt_load(VX_model * m , const char * path){
    file_content * fc = load_file( path );
                                   
    if( !fc ){
        return -1;
    }

    if( fc->len <= sizeof(vxl_header) ){
        printf("err: file too small.");
        return -1;
    }

    vxl_header h;
    memcpy(&h, fc->buffer, sizeof(vxl_header));
    
    if( h.magic_number != 0x09072000 || 
        h.width != 1024 || h.height != 1024 ){ 
        printf("warn: file %s is not proper vxl format\n" , path );
        free( fc->buffer );
        free( fc );
        return -1;
    }

    VX_uint32 len = fc->len - sizeof( vxl_header );
    VX_byte * vbuf = fc->buffer;
    VX_byte * v = vbuf;

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
    free( fc );

    return 0;
}

/* KV6 */

/*long loadkv6 (char *filnam)
{
	long i, j, x, y, z;
	unsigned short u;
	voxtype *vptr, *vend;
	FILE *fil;

	if (!(fil = fopen(filnam,"rb"))) return(0);
	fread(&i,4,1,fil); if (i != 0x6c78764b) return(0); //Kvxl
	fread(&xsiz,4,1,fil); fread(&ysiz,4,1,fil); fread(&zsiz,4,1,fil);
	if ((xsiz > MAXXSIZ) || (ysiz > MAXYSIZ) || (zsiz > LIMZSIZ)) { fclose(fil); return(0); }
	fread(&xpiv,4,1,fil); fread(&ypiv,4,1,fil); fread(&zpiv,4,1,fil);
	fread(&numvoxs,4,1,fil);

		//Added 06/30/2007: suggested palette (optional)
	fseek(fil,32+numvoxs*8+(xsiz<<2)+((xsiz*ysiz)<<1),SEEK_SET);
	fread(&i,4,1,fil);
	if (i == 0x6c615053) //SPal
	{
		fread(fipalette,768,1,fil);
		initclosestcolorfast(fipalette);
		fseek(fil,32,SEEK_SET);
		goto kv6_loadrest;
	} else fseek(fil,32,SEEK_SET);

	for(i=767;i>=0;i--) if (fipalette[i]) break;
	if (i < 0)
	{
		long palnum = 0;
		memset(closestcol,-1,sizeof(closestcol));
		for(vptr=voxdata,vend=&voxdata[numvoxs];vptr<vend;vptr++)
		{
			fread(&i,4,1,fil);
			i = ((i>>6)&0x3f000)+((i>>4)&0xfc0)+((i>>2)&0x3f);
			if (closestcol[i] != 255) vptr->col = closestcol[i];
			else if (palnum < 256)
			{
				fipalette[palnum*3+0] = ((i>>12)&63);
				fipalette[palnum*3+1] = ((i>> 6)&63);
				fipalette[palnum*3+2] = ( i     &63);
				vptr->col = palnum;
				closestcol[i] = palnum;
				palnum++;
			}
			else vptr->col = 255;

			vptr->z = fgetc(fil); fgetc(fil);
			vptr->vis = fgetc(fil); vptr->dir = fgetc(fil);
		}
		initclosestcolorfast(fipalette);
	}
	else
	{
kv6_loadrest:;
		for(vptr=voxdata,vend=&voxdata[numvoxs];vptr<vend;vptr++)
		{
			fread(&i,4,1,fil); vptr->col = closestcol[((i>>6)&0x3f000)+((i>>4)&0xfc0)+((i>>2)&0x3f)];
			vptr->z = fgetc(fil); fgetc(fil);
			vptr->vis = fgetc(fil); vptr->dir = fgetc(fil);
		}
	}
	for(x=0;x<xsiz;x++) { fread(&i,4,1,fil); xlen[x] = i; }
	for(x=0;x<xsiz;x++)
		for(y=0;y<ysiz;y++) { fread(&u,2,1,fil); ylen[x][y] = u; }
	fclose(fil);

	memset(vbit,-1,sizeof(vbit));
	vptr = voxdata;
	for(x=0;x<xsiz;x++)
		for(y=0;y<ysiz;y++)
		{
			j = ((x*MAXYSIZ)+y)*BUFZSIZ;
			vend = &vptr[ylen[x][y]];
			for(;vptr<vend;vptr++)
			{
				if (vptr->vis&16) z = vptr->z;
				if (vptr->vis&32) setzrange0(j,z,vptr->z+1);
			}
		}

	return(1);
}
*/

/* unused */
typedef struct kvx_header{
    VX_uint32 nb;
    VX_uint32 w;
    VX_uint32 h;
    VX_uint32 d;

    VX_uint32 reserved1;
    VX_uint32 reserved2;
    VX_uint32 reserved3;
}kvx_header;

#define READ(type, file) \
    ({ type v; size_t r = fread(&v, sizeof(v), 1, file); (void)r; v;})

static double clamp(double d, double min, double max) {
  const double t = d < min ? min : d;
  return t > max ? max : t;
}

/* Build engine .kvx loader */
/*
 * this code is adapted from https://github.com/guillaumechereau/goxel/blob/master/src/formats/voxlap.c 
 * TODO: refactor
 */
int VX_kvxfmt_load(VX_model * m , const char * path)
{
    FILE *file;
    int i, r, ret = 0, nb, w, h, d, x, y, z, lastz = 0, len, visface;
    uint8_t color = 0;
    uint8_t (*palette)[4] = NULL;
    uint32_t *xoffsets = NULL;
    uint16_t *xyoffsets = NULL;
    long datpos;
    (void)r;

    file = fopen(path, "rb");
    if( !file ){
        printf("err: no such file or directory: %s.\n", path);
        return -1;
    }
    
    nb = READ(uint32_t, file); (void)nb;
    w = READ(uint32_t, file);
    h = READ(uint32_t, file);
    d = READ(uint32_t, file);

    READ(uint32_t, file);
    READ(uint32_t, file);
    READ(uint32_t, file);

    xoffsets = calloc(w + 1, sizeof(*xoffsets));
    xyoffsets = calloc(w * (h + 1), sizeof(*xyoffsets));
    for (i = 0; i < w + 1; i++)        xoffsets[i] = READ(uint32_t, file);
    for (i = 0; i < w * (h + 1); i++) xyoffsets[i] = READ(uint16_t, file);

    datpos = ftell(file);

    // Read the palette at the end of the file first.
    fseek(file, -256 * 3, SEEK_END);
    palette = calloc(256, sizeof(*palette));
    for (i = 0; i < 256; i++) {
        palette[i][2] = clamp(round(READ(uint8_t, file) * 255 / 63.f), 0, 255);
        palette[i][1] = clamp(round(READ(uint8_t, file) * 255 / 63.f), 0, 255);
        palette[i][0] = clamp(round(READ(uint8_t, file) * 255 / 63.f), 0, 255);
        palette[i][3] = 255;
    }
    fseek(file, datpos, SEEK_SET);

    for (x = 0; x < w; x++)
    for (y = 0; y < h; y++) {
        if (xyoffsets[x * (h + 1) + y + 1] < xyoffsets[x * (h + 1) + y]){
            printf("err: invalid format\n");
            ret = -1;
            goto end;
        }
        nb = xyoffsets[x * (h + 1) + y + 1] - xyoffsets[x * (h + 1) + y];
        while (nb > 0) {
            z = READ(uint8_t, file);
            len = READ(uint8_t, file);
            visface = READ(uint8_t, file);

            if( !(z + len - 1  < d) ){
                printf("err: invalid assertion\n");
                ret = -1;
                goto end;
            }
            
            for (i = 0; i < len; i++) {
                color = READ(uint8_t, file);
                VX_uint32 clr = *((VX_uint32*)palette[color]);
                m->set_voxel(m, x, y, z + i, clr);
            }
            nb -= len + 3;
            // Fill
            if (visface & 0x10) lastz = z + len;
            if (visface & 0x20) {
                for (i = lastz; i < z; i++){
                    if( m->get_voxel(m, x, y, i, NULL)[3] == 0 ){
                        VX_uint32 clr = *((VX_uint32*)palette[color]);
                        m->set_voxel(m, x, y, i, clr);
                    }
                }
            }
        }
    }

end:
    free(palette);
    free(xoffsets);
    free(xyoffsets);
    fclose(file);
    return ret;
}
