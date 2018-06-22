#include "VX_lib.h"
#include "SDL/SDL.h"
#include "SDL/SDL_image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Tested in white-box manner 08-04-2013 
   - everything was ok on i386, no valgrind problems present
   - no Segmentation fault
   - minor memory leaks
*/

#define CONV_VERSION "0.1"

char *strdup(const char *str);

VX_uint32 toGRAY32(VX_format * self, VX_byte * chunk)
{
    if (*chunk > 145) {
        return 0x00000000 | *chunk << 16 | *chunk << 8 | *chunk;
    }

    return self->colorkey;
}

VX_uint32 ARGB32toARGB32(VX_format * self, VX_byte * chunk)
{
    VX_uint32 out = *((VX_uint32 *) chunk);
    return out;
}

VX_uint32 TCtoARGB32(VX_format * self, VX_byte * chunk)
{
    /*
     * warn: endianity problem 
     */
    VX_uint32 out = 0 | chunk[0] << 16 | chunk[1] << 8 | chunk[2];
    return out;
}

VX_format FMT_255 = { 8, 1, 3, 3, 2, 0, 0x0, toGRAY32 };
VX_format FMT_ARGB32 = { 32, 4, 8, 8, 8, 8, 0, ARGB32toARGB32 };
VX_format FMT_TRUECOLOR = { 25, 3, 8, 8, 8, 0, 0, TCtoARGB32 };

const char *help_msg =
    "\t*** CONVERSION TOOL BETWEEN RAW VOXEL FORMATS AND .OCT FORMAT***\n"
    "\tPavel Prochazka (C) 2011-2013\n" "Usage:\n" "Syntax of voxconv\n"
    "voxconv INPUT_TYPE filename1 OUTPUT_TYPE filename2\n"
    "Options: (INPUT_TYPE)\n" "\t-vxl\tInput image is in .vxl format\n"
    "\t-oct\tInput image is in octree format\n"
    "\t-aoct\tInput image is in adaptive octree format\n"
    "\t-raw\tInput image is in raw format, exactly after this must follow information WIDTH HEIGHT DEPTH,\n"
    "which is size of raw and then must follow option -g (black-white image), -t (true-color) or -argb (ARGB32 color)\n"
    "\t-set\tInput 2D images are in a supported format by SDL_image library,\n"
    "size will be used by the first image and DEPTH will be same as count of images\n"
    "\nOptions: (OUTPUT_TYPE)\n"
    "\t-oct\tOutput image is in octree format\n"
    "\t-aoct\tOutput image is in adaptive octree format\n"    
    "\t-raw\tOutput image is in raw format (with defaulted format to ARGB32)\n"
    "Options: (OTHER)\n" "\t-h, --help\tPrints this message\n"
    "\t-v, --version\tPrints information about version\n";

void print_help()
{
    printf("%s\n", help_msg);
}

void print_version()
{
    printf("%s\n", CONV_VERSION);
}

#define OCT 0
#define RAW 1
#define VXL 2
#define SET 3
#define AOCT 4

int translate_type(char *type)
{
    if (!strcmp(type, "-oct"))
        return OCT;
    if (!strcmp(type, "-raw"))
        return RAW;
    if (!strcmp(type, "-vxl"))
        return VXL;
    if (!strcmp(type, "-set"))
        return SET;
    if (!strcmp(type, "-aoct"))
        return AOCT;
    return -1;
}

int check_idx(int idx, int argc)
{
    if (idx >= argc) {
        printf("err: bad syntax\n");
        exit(-1);
    }
    return 1;
}

void syntax_err()
{
    printf("err: bad syntax\n");
    exit(-1);
}

VX_format select_fmt(char *arg)
{
    if (!strcmp(arg, "-g"))
        return FMT_255;
    if (!strcmp(arg, "-t"))
        return FMT_TRUECOLOR;
    if (!strcmp(arg, "-argb"))
        return FMT_ARGB32;

    syntax_err();
    return FMT_255;
}

VX_model *set_to_model(char **paths, int len)
{
    SDL_Surface *s = IMG_Load(paths[0]);
    if (!s) {
        printf("err: can not load such file %s\n", paths[0]);
        exit(-1);
    }

    VX_block b = { 0, 0, 0, s->w, s->h, len };
    int Bpp = s->format->BytesPerPixel;
    if (Bpp < 3) {
        printf
            ("err: acceptiing images only in TRUECOLOR or ARGB32 format\n");
        SDL_FreeSurface(s);
        exit(-1);
    }

    SDL_FreeSurface(s);

    size_t fsz = b.w * b.h * len * Bpp;

    VX_model *m = NULL;

    if (fsz > (1024 * 1024 * 1024)) {
        m = VX_model_octree_new();
    } else {
        m = VX_model_raw_new(&FMT_ARGB32, b);
    }

    for (int i = 0; i < len; i++) {
        s = IMG_Load(paths[i]);
        if (s->w != b.w || s->h != b.h) {
            printf("err: images must have equal size\n");
            SDL_FreeSurface(s);
            m->free(m);
            exit(-1);
        }

        if (!s) {
            printf("err: can not load such file %s\n", paths[0]);
            m->free(m);
            exit(0);
        }

        VX_byte *pix = s->pixels;

        for (int y = 0; y < b.h; y++) {
            for (int x = 0; x < b.w; x++) {
                VX_uint32 color = 0;
                /*
                 * warn: ENDINANITY 
                 */
                memcpy(&color, pix, Bpp);
                pix += Bpp;

                m->set_voxel(m, x, y, i, color);
            }
        }

        SDL_FreeSurface(s);
    }

    return m;
}

int main(int argc, char *argv[])
{
    if (argc <= 1) {
        print_help();
        return -1;
    }

    int idx = 1;

    if (!strcmp(argv[idx], "-h") || !strcmp(argv[idx], "--help")) {
        print_help();
        return 0;
    }

    if (!strcmp(argv[idx], "-v") || !strcmp(argv[idx], "--version")) {
        print_version();
        return 0;
    }

    char **in = NULL;
    unsigned int size[3];
    VX_format fmt;
    int in_cnt = 1;

    int in_type = translate_type(argv[idx++]);
    check_idx(idx, argc);

    switch (in_type) {
    case AOCT:
    case OCT:
        in = malloc(2 * sizeof(char *));
        in[0] = strdup(argv[idx++]);
        in[1] = NULL;
        break;
    case RAW:
        in = malloc(2 * sizeof(char *));
        in[0] = strdup(argv[idx++]);
        in[1] = NULL;

        if (argc < 6) {
            syntax_err();
        }

        VX_uint32 ret = 0xffffffff;
        ret &= sscanf(argv[idx++], "%u", &(size[0]));
        ret &= sscanf(argv[idx++], "%u", &(size[1]));
        ret &= sscanf(argv[idx++], "%u", &(size[2]));

        if ( !ret )
        {
            free( in[0] );
            free( in );
            syntax_err();
        }

        fmt = select_fmt(argv[idx++]);
        break;
    case SET:
        {
            int len = 0;
            int mi = idx;

            while (translate_type(argv[mi]) == -1) {
                printf("%s\n", argv[mi]);

                len++;
                mi++;

                if (mi >= argc) {
                    syntax_err();
                }
            }

            if (mi >= argc) {
                syntax_err();
            }

            in = malloc((len + 1) * sizeof(char *));
            in[len] = NULL;
            for (int i = 0; i < len; i++) {
                in[i] = strdup(argv[idx++]);
            }
            in_cnt = len;
            printf("in_count: %d\n", in_cnt);

        }
        break;
    case VXL:
        check_idx(idx, argc);
        in = malloc(sizeof(char *) * 2);
        in[1] = NULL;
        in[0] = strdup(argv[idx++]);
        break;
    default:
        printf("err: unknown input type\n");
        return -1;
        break;
    }

    check_idx(idx, argc);
    int out_type = translate_type(argv[idx++]);
    char *out = strdup(argv[idx++]);

    if (idx != argc) {
        printf("warn: ignoring some ending part of argument list!\n");
    }

    VX_model *m_in = NULL;
    VX_model *m_out = NULL;

    switch (in_type) {
    case OCT:
        m_in = VX_model_octree_new();
        if (m_in->load(m_in, in[0])) {
            printf("err: cant load octree model %s\n", in[0]);
            m_in->free(m_in);
            return -1;
        }
        m_in->inspect(m_in);
        break;
    case AOCT:
        m_in = VX_adaptive_octree_new();
        if( m_in->load(m_in , in[0]) ){
            printf("err: cant load adaptive octree model %s\n", in[0]);
            m_in->free(m_in);
            return -1;
        }
        m_in->inspect(m_in);
        break;
    case RAW:
        {
            VX_block b = { 0, 0, 0, size[X], size[Y], size[Z] };
            m_in = VX_model_raw_new(&fmt, b);
            if (m_in->load(m_in, in[0])) {
                printf("err: cant load raw model %s\n", in[0]);
                m_in->free(m_in);
                return 0;
            }
        }
        break;
    case VXL:
        {
            VX_block b = { 0, 0, 0, 1024, 1024, 256 };
            /*
             * prefer to cache into 1GB raw model - 10x faster but memory
             * cost is high 
             */
            m_in = VX_model_raw_new(&FMT_ARGB32, b);
            if (VX_vxlfmt_load(m_in, in[0])) {
                printf("err: cant load vxl file %s\n", in[0]);
                m_in->free(m_in);
                return -1;
            }
        }
        break;
    case SET:
        m_in = set_to_model(in, in_cnt);
        break;
    default:
        printf("err: this shouldnt be - instruction memory corruption?\n");
        return -1;
        break;
    }

    switch (out_type) {
    case OCT:
        m_out = VX_model_octree_new();
        break;
    case AOCT:
        m_out = VX_adaptive_octree_new();
        break;
    case RAW:
        m_out = VX_model_raw_new(&FMT_ARGB32, m_in->dim_size);
        if (!m_out) {
            printf("err: cant allocate such a huge raw model\n");
            m_in->free(m_in);
            return -1;
        }
        break;
    default:
        printf("err: this shouldnt be - instruction or memory corruption?\n");
        return -1;
        break;
    }

    if (m_out->compile(m_out, m_in)) {
        printf("err: compilation failed!\n");
        return -1;
    }

    m_in->free(m_in);

    if (m_out->dump(m_out, out)) {
        printf("err: saving failed\n");
        return -1;
    }

    m_out->inspect(m_out);
    m_out->free(m_out);

    char **po = in;

    while (*po) {
        free(*po);
        po++;
    }
    free(in);
    free(out);

    return 0;
}
