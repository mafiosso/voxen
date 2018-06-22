#ifndef ADAPTIVE_OCTREE_H
#define ADAPTIVE_OCTREE_H

typedef struct VX_aoct_node{
    VX_uint32 flags;
    VX_uint32 color;
    struct VX_aoct_node * childs[8];
}VX_aoct_node;

/** Private undocumented structure holding leaf octree node */
typedef struct VX_aoct_node_leaf{
    VX_uint32 flags;
    VX_uint32 color;
}VX_aoct_node_leaf;

typedef struct VX_araw_node{
    VX_uint32 flags;
}VX_araw_node;


VX_model * VX_adaptive_octree_new();

#endif
