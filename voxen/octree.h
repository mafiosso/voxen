#ifndef OCTREE_H
#define OCTREE_H

/**
 * @file octree.h
 */

/** Private undocumented structure holding octree node */
typedef struct VX_oct_node{
    VX_uint32 flags;
    VX_uint32 color;
    struct VX_oct_node * childs[8];
}VX_oct_node;

/** Private undocumented structure holding leaf octree node */
typedef struct VX_oct_node_leaf{
    VX_uint32 flags;
    VX_uint32 color;
}VX_oct_node_leaf;

/** Private undocumented structure holding information about octree */
typedef struct VX_oct_info{
    VX_uint32 stack_len;
    VX_uint32 item_len;
    VX_uint32 buffer_size; /* size of the huge buffer */
}VX_oct_info;

/** Additional function that modifies size of valid octree.
 *  \param self
 *  \param size size of all edges
 *  \return 1
*/
int VX_oct_set_size( VX_model * self , int size );

/** Additional function for octree for fast filling of large regions.
 * \param self
 * \param b block to fill
 * \param color color to fill region b with
*/
int VX_oct_fill_region( VX_model * self , VX_ms_block b , VX_uint32 color);

/** Constructor of octree model format, color format can not be modified and is set to ARGB32.
    \return valid VX_model pointer (when malloc can not fail - on UNIX)
*/
VX_model * VX_model_octree_new();

#endif
