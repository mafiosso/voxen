#include "VX_lib.h"

#ifdef _WIN32
#include <windows.h>
#elif MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#else
#include <unistd.h>
#endif

int VX_point_in_cube_p( VX_ms_block b , VX_ipoint3 p ){
    return (p.x >= b.x1) && (p.y >= b.y1) && (p.z >= b.z1) &&
        (p.x < b.x2) && (p.y < b.y2) && ( p.z < b.z2 );
}

/* correct output only if intersects */
VX_ms_block VX_cubes_intersection( VX_ms_block b1 , VX_ms_block b2 ){
    VX_ms_block out;
    
    out.x1 = MAX( b1.x1 , b2.x1 );
    out.y1 = MAX( b1.y1 , b2.y1 );
    out.z1 = MAX( b1.z1 , b2.z1 );

    out.x2 = MIN( b1.x2 , b2.x2 );
    out.y2 = MIN( b1.y2 , b2.y2 );
    out.z2 = MIN( b1.z2 , b2.z2 );

    return out;
}

int VX_cubes_intersects_p( VX_ms_block b1 , VX_ms_block b2 ){
    VX_ms_block ib = VX_cubes_intersection( b1 , b2 );
    return (ib.x1 < ib.x2) && (ib.y1 < ib.y2) && (ib.z1 < ib.z2);
}

VX_uint32 nearest_pow2( VX_uint32 a )
{
    VX_uint32 out = 1;

    while( out < a ){
        out <<= 1;
    }

    return out;
}

int ilog2( VX_uint32 a ){
    int out = 0;
    while( (a = (a >> 1)) ){
        out++;
    }

    return out;
}

/* taken from http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine under cc-wiki license */
int VX_cores_count() {
#ifdef WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#elif MACOS
    int nm[2];
    uint32_t count;
    size_t len = 4;

    nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
    sysctl(nm, 2, &count, &len, NULL, 0);

    if(count < 1) {
        nm[1] = HW_NCPU;
        sysctl(nm, 2, &count, &len, NULL, 0);
        if(count < 1) { count = 1; }
    }
    return count;
#else
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

/* all redundant init actions perform here 
   to get writing driver more user-friendly */
void VX_machine_default_init( VX_machine * m ){
    m->cores_count = VX_cores_count();
}


VX_uint32 color_add( VX_uint32 c1 , VX_uint32 c2 ){
    VX_uint32 out;
    VX_byte * cp1 = (VX_byte*)&c1;
    VX_byte * cp2 = (VX_byte*)&c2;
    VX_byte * cpo = (VX_byte*)&out;

    VX_uint32 tmp = cp1[0] + cp2[0];
    cpo[0] = (tmp > 255 ? 255 : tmp);
    tmp = cp1[1] + cp2[1];
    cpo[1] = (tmp > 255 ? 255 : tmp);
    tmp = cp1[2] + cp2[2];
    cpo[2] = (tmp > 255 ? 255 : tmp);

    return out;
}

VX_uint32 color_mul( VX_uint32 c1 , VX_uint32 c2 ){
    VX_uint32 out;
    VX_byte * cp1 = (VX_byte*)&c1;
    VX_byte * cp2 = (VX_byte*)&c2;
    VX_byte * cpo = (VX_byte*)&out;

    VX_uint32 tmp = (cp1[0] * cp2[0])/256;
    cpo[0] = (VX_byte)tmp;
    tmp = (cp1[1] * cp2[1])/256;
    cpo[1] = (VX_byte)tmp;
    tmp = (cp1[2] * cp2[2])/256;
    cpo[2] = (VX_byte)tmp;

    return out;
}
