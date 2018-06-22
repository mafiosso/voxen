#ifndef VX_LIB_H
#define VX_LIB_H

/*! \file VX_lib.h
 * \brief VX_lib.h is the only one header file,
 *  which can be explicitly included in your project using the VoXen library
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef WIN32

#include "windows.h"

#else

#include <pthread.h>
#include <semaphore.h>

#endif

#define NO_OPENCL 1

#ifndef NO_OPENCL
#if defined __APPLE__ || defined(MACOSX)
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif
#endif

#include "vx_fundamental.h"
#include "peasant.h"
#include "vxocl.h"
#include "slab.h"
#include "matrix.h"
#include "display.h"
#include "camera.h"
#include "voxel.h"
#include "lighting.h"
#include "raw.h"
#include "fpu_octree_traversal.h"
#include "octree.h"
#include "ao_traversal.h"
#include "adaptive_octree.h"
#include "vxlfmt.h"

#endif
