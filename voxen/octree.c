#include "VX_lib.h"
#include <stddef.h>
#include <math.h>
#include "octree_traversal.h"


/* octree OpenCL code compiled into somethin like MAKEINTRESOURCE 
   on Windows */
extern char * _binary___cltmp_octree_traversal_cl_cl_start;

#define CL_OCT_CODE _binary___cltmp_octree_traversal_cl_cl_start


/* 
 *  TODO: make octree saving struct platform independent 32bit vs 64bit!! LITTLE ENDIAN vs BIG ENDIAN 
 */
static void oct_free_end( VX_oct_node * root );

static int oct_buffered_item_p( VX_model * m , VX_oct_node * item){
    VX_oct_info * inf = m->data;
    return ((VX_byte*)item >= (VX_byte*)m->chunk_ptr) && 
        ((VX_byte*)item < (((VX_byte*)m->chunk_ptr) + inf->buffer_size ));
}

static int optimal_p( VX_oct_node * root ){
    if( !root->flags ){ /* all subnodes are NULL */
        return 1;
    }
    
    if( root->flags < 255 ){
        return 0;
    }

    for( int i = 0; i < 8 ; i++ ){
        /* warn: old oct models can have incosistent flags:/ */
        if( !root->childs[i] || root->childs[i]->flags ){
            /* can not be nonleaf */
            return 0;
        }
        else if( root->childs[i]->color != root->childs[0]->color ){
            /* color must be same */
            return 0;
        }
    }

    return 2;
}

static void recomp_lod( VX_oct_node * root ){
    VX_uint32 argb[4] = {0,0,0,0};
    for( int i = 0 ; i < 8 ; i++ ){
        VX_oct_node * in = root->childs[i];

        if( in ){
            VX_byte * clr = (VX_byte*)&(in->color);
            
            argb[A] += clr[A];
            argb[R] += clr[R];
            argb[G] += clr[G];
            argb[B] += clr[B];
        }
        else{
            argb[A] += 255;
        }
    }

    argb[A] /= 8;
    argb[R] /= 8;
    argb[G] /= 8;
    argb[B] /= 8;

    root->color = 0x00000000 | ((argb[A] << 24) | (argb[R] << 16) |
                       (argb[G] << 8) | argb[B]);    

}

static VX_oct_node * optimize( VX_model * self , VX_oct_node * root , int recomp_lod_p ){
    int optimality = optimal_p( root );
    VX_uint32 color;

    switch( optimality ){
    case 0:
        if( recomp_lod_p ){
            recomp_lod( root );
        }

        return root;
        break;
    case 1: /* empty case */
        if( !oct_buffered_item_p( self , root ) )
            free( root );
        return NULL;
        break;
    case 2: /* same color case */
        color = root->childs[0]->color;

        if( !oct_buffered_item_p( self , root ) )
             oct_free_end( root );

        root = malloc( sizeof( VX_oct_node_leaf ) );
        root->flags = 0;
        root->color = color;
        return root;
        break;
    default:
        return root;
        break;
    }

    return root;
}

static VX_oct_node * oct_fill_region_end( VX_model * self ,
                                          VX_oct_node * n , 
                                          VX_ms_block node_block,
                                          VX_ms_block * b ,
                                          VX_uint32 color )
{
    if( !VX_cubes_intersects_p( node_block , *b )){
        return n;
    }

    int nw = (node_block.x2 - node_block.x1);
    VX_ms_block is = VX_cubes_intersection( node_block , *b );
    /* whole node n is in b */
    if( !memcmp( &is , &node_block , sizeof(VX_ms_block)) ){
        if( !oct_buffered_item_p( self , n ) ){
            oct_free_end( n );
        }

        if( color & 0x00ffffff ){/* empty check */
            n = malloc( sizeof(VX_oct_node_leaf) );
            n->flags = 0;
            n->color = color;
            return n;
        }
        return NULL;
    }

    /* leaf voxel and not intersects */
    if( nw <= 1 ){
        return n;
    }

    /* hm only partial intersection go down though */
    nw >>= 1;
    
    if( !n || !n->flags ){
        free( n );
        n = calloc( 1 , sizeof(VX_oct_node) );
    }

    VX_ms_block nb = node_block;
    
    for( int z = 0 ; z < 2 ; z++ )
    {
        for( int y = 0 ; y < 2 ; y++ )
        {
            for( int x = 0 ; x < 2 ; x++ )
            {
                VX_uint32 idx = (z*4)+(y*2)+x;
                VX_block tmp = {nb.x1 + (x*nw) , nb.y1 + (y*nw),
                                nb.z1 + (z*nw) , nw , nw , nw };
                
                VX_oct_node * in = oct_fill_region_end( self ,
                                                        n->childs[idx] , 
                                                        BOX_TO_MS_BLOCK( tmp ) ,
                                                        b , color );

                n->childs[idx] = in;
                n->flags |= (in ? 1 : 0) << idx;
            }
        }
    }

    return optimize( self , n , 1 );
}

void oct_update_stack_len( VX_model * self ){
    int lvl = 1;
    int sz = self->dim_size.w;

    while( (sz = sz >> 1) ){
        lvl++;
    }

    ((VX_oct_info*)self->data)->stack_len = lvl;
}

int VX_oct_set_size( VX_model * self , int size ){
    if( self->dim_size.w <= 0 ){
        self->dim_size.w = 1;
        self->dim_size.h = 1;
        self->dim_size.d = 1;
    }

    if( size <= self->dim_size.w ){
        if( !self->root )
        {
            self->root = calloc( 1 , sizeof(VX_oct_node) );
        }
        return 1;
    }

    if( self->root ){
        VX_oct_node * root = calloc( 1 , sizeof( VX_oct_node ) );
        root->childs[0] = self->root;
        root->flags = (self->root ? 1 : 0);
        self->root = root;
    }

    self->dim_size.w = self->dim_size.w * 2;
    self->dim_size.h = self->dim_size.w;
    self->dim_size.d = self->dim_size.w;

    oct_update_stack_len( self );

    return VX_oct_set_size( self , size );
}

int VX_oct_fill_region( VX_model * self , VX_ms_block b , VX_uint32 color){
    VX_ms_block region = BOX_TO_MS_BLOCK( self->dim_size );
    VX_ms_block isection = VX_cubes_intersection( region , b );

    /* enlarge when needed */
    if( memcmp( &isection , &b , sizeof( VX_ms_block ) )){
        VX_block t = MS_BLOCK_TO_BOX( b );
        int max_w = MAX( t.x , MAX( t.y , t.z ) );
        max_w = nearest_pow2( max_w );
        VX_oct_set_size( self , max_w );
    }

    self->root =  oct_fill_region_end( self ,
                                       self->root , 
                                       region,
                                       &b ,
                                       color );
    return 1;
}

static VX_oct_node * oct_compile_impl( VX_model * self, 
                                       VX_model * m ,
                                       VX_block bbox )

{
    VX_oct_node * out = NULL;
    VX_format * fmt = m->fmt;
    
    if( bbox.w <= 1 )
    {
        int lod = 0xff;
        VX_byte * color_ptr = m->get_voxel(  m , bbox.x , bbox.y , bbox.z , &lod );
        if( !color_ptr ){
            return NULL;
        }
        
        VX_uint32 color = fmt->ARGB32( fmt , color_ptr );
        
        /* empty */
        if( color == fmt->colorkey )
            return NULL;
        
        out = calloc( 1 , sizeof(VX_oct_node_leaf) );
        out->color = color;

        return out;
    }

    VX_block c = bbox;

    c.w /= 2;
    c.h /= 2;
    c.d /= 2;

    out = malloc( sizeof(VX_oct_node) );
    out->flags = 0;

    for( int z = 0 ; z < 2 ; z++ )
    {
        for( int y = 0 ; y < 2 ; y++ )
        {
            for( int x = 0 ; x < 2 ; x++ )
            {
                VX_uint32 idx = (z*4)+(y*2)+x;
                VX_oct_node * in = oct_compile_impl( self , m ,
                                                     (VX_block){
                                                         bbox.x+(x*c.w) ,
                                                             bbox.y+(y*c.h) ,
                                                             bbox.z+(z*c.d) ,
                                                             c.w, c.w , c.w } 
                    );

                out->childs[idx] = in;
                out->flags |= ((in != NULL) << idx);
            }
        }
    }

    return optimize( self , out , 1);
}

int VX_oct_compile( VX_model * self ,
                    VX_model * in )
{
    VX_uint32 w = MAX( in->dim_size.d , 
                       MAX( in->dim_size.w , in->dim_size.h ) );

    w = nearest_pow2( w );

    VX_block bbox = { 0,0,0, w , w, w };

    printf( "octree size: %d\n" , w );
    self->root = oct_compile_impl( self , in , bbox );
    self->dim_size = bbox;

    oct_update_stack_len( self );

    return (self->root ? 0 : -1);
}

static void oct_inspect_impl( VX_oct_node * n , int * voxels , int * leaf )
{
    if( !n )
        return;

    *voxels = (*voxels + 1);

    if( !n->flags )
    {
        (*leaf)++;
        return;
    }

    for( int i = 0; i < 8 ; i++ )
    {
        oct_inspect_impl( n->childs[i] , voxels , leaf );
    }
}

struct VX_OCT_header{
    VX_block dim_size;
    VX_uint32 control_size;
};

/* Due to x86 and AMD64 interportability */
typedef struct _oct_file_node{
    VX_uint32 flags;
    VX_uint32 color;
    VX_uint32 childs_offsets[8];
}_oct_file_node;

typedef struct _oct_file_node_leaf{
    VX_uint32 flags;
    VX_uint32 color;
}_oct_file_node_leaf;

static int oct_dump_end( VX_oct_node * root , FILE * descriptor ,
                          VX_uint32 * idx )
{
    /* WARN: endianity issue */
    if( !root->flags ) /* its leaf node */{
        /* leaf node is exactly as same as VX_oct_node_leaf */
        /* and platform independent (32bit 64bit)*/
        return fwrite( root , sizeof( VX_oct_node_leaf ) , 1 , descriptor );
    }

    _oct_file_node dump_node = {root->flags , root->color};
    int out = 1;

    for( int i = 0 ; i < 8 ; i++ )
    {
        if( root->childs[i] )
        {
            out &= oct_dump_end( root->childs[i] , descriptor , idx );
            dump_node.childs_offsets[i] = (*idx);
            
            if( root->childs[i]->flags == 0 )
                (*idx) += sizeof(_oct_file_node_leaf);
            else
                (*idx) += sizeof(_oct_file_node);
        }
    }

    out &= fwrite( &dump_node , sizeof(_oct_file_node) , 1 , descriptor );
    return out;
}

int VX_oct_dump( VX_model * self , const char * out_file )
{
    FILE * f = fopen( out_file , "w+" );

    if( !f )
        return -1;
    
    struct VX_OCT_header header = { self->dim_size , 
                                    (self->dim_size.w*self->dim_size.h*self->dim_size.d)+0xbeaf };

    if( !fwrite(&header , sizeof(struct VX_OCT_header) , 1 , f ) )
    {
        fclose( f );
        return -1;
    }

    VX_uint32 stack_top = sizeof(header);
    int ret = oct_dump_end( self->root , f , &stack_top );
    fclose( f );

    printf("info: stack-top: %d\n" , stack_top );
    return (ret ? 0 : -1);
}

void VX_oct_inspect( VX_model * self )
{
    int voxels = 0;
    int leaf_vox = 0;

    oct_inspect_impl( self->root , &voxels , &leaf_vox );

    int size = (voxels*sizeof(VX_oct_node))-(leaf_vox*(sizeof(VX_oct_node)-sizeof(VX_oct_node_leaf)));

    printf("Octree info: size %d x %d x %d \n" , self->dim_size.w , self->dim_size.h , self->dim_size.d );
    printf("Octree stats: nodes %d total B used: %d leaf: %d\n" ,
           voxels ,
           size ,
           leaf_vox );
}

/* 
   octree loading - rebuild only tree from file - 
   faster will be a self allocation structure but very complicated
*/
static VX_oct_node * oct_load_end( _oct_file_node * pool_root ,
                                   VX_byte * pool )
{
    VX_oct_node * out = NULL;
    /* leaf FILE node -> allocate leaf RAM node */
    if( !(pool_root->flags) ){
        out = malloc( sizeof(VX_oct_node_leaf) );
        out->flags = 0;
        out->color = pool_root->color;
        return out;
    }

    out = malloc( sizeof(VX_oct_node) );
    out->flags = pool_root->flags;
    out->color = pool_root->color;

    for( int i = 0 ; i < 8 ; i++ ){
        VX_byte * addr = pool + (pool_root->childs_offsets[i]);
        if( addr != pool ){
            out->childs[i] = oct_load_end( (_oct_file_node*)addr , pool );
        }
        else{
            out->childs[i] = NULL;
        }
    }

    return out;
}

int VX_oct_load( VX_model * self , const char * path )
{
    FILE * f = fopen( path , "r" );

    if( !f )
        return -1;

    fseek( f , 0 , SEEK_END );
    size_t sz = ftell( f );
    rewind( f );

    /* read header */
    struct VX_OCT_header header;
    if( !fread( &header , sizeof( struct VX_OCT_header ) , 1 , f ) )
    {
        fclose( f );
        return -1;
    }

    /* compute header's checksum */
    if( ((header.dim_size.w * header.dim_size.h * header.dim_size.d)+0xbeaf) != header.control_size ){
        printf("err: not correct oct file!\n");
        fclose( f );
        return -1;
    }

    sz -= sizeof( struct VX_OCT_header );
    void * voxel_pool = malloc(sz);

    if( !fread( voxel_pool , sizeof(VX_byte) , sz , f ) )
    {
        fclose( f );
        free( voxel_pool );
        printf("err: reading failed!\n");
        return -1;
    }

    self->dim_size = header.dim_size;
    
    /* model with only one voxel */
    if( sz == sizeof( _oct_file_node_leaf ) ) 
    {
        self->root = oct_load_end( voxel_pool , voxel_pool - sizeof(header));
    }
    else
    {
        _oct_file_node * pr = (voxel_pool+sz-sizeof(_oct_file_node));
        self->root = oct_load_end( pr , voxel_pool - sizeof(header) );
    }

    free( voxel_pool );
    fclose( f );

    oct_update_stack_len( self );
    ((VX_oct_info*)self->data)->buffer_size = 0;
    return 0;
}

static VX_uint32 _oct_black = 0;
static inline VX_byte * oct_get_voxaddr_impl( VX_oct_node * root ,
                                              VX_uint32 w,
                                              VX_uint32 x,
                                              VX_uint32 y,
                                              VX_uint32 z,
                                              VX_ms_block * b,
                                              int * lod )
{
    *b = (VX_ms_block){ 0 , 0 , 0 , w , w , w };
    VX_uint32 hw = w;
    VX_uint32 z_order, y_order, x_order; /* uint32 is faster than byte */

    while( root && root->flags && (*lod > 0) )
    {
        hw >>= 1;
        z_order = (z >= (b->z1 + hw));
        y_order = (y >= (b->y1 + hw));
        x_order = (x >= (b->x1 + hw));

        /* 0 or 1 * hw will give hw or 0 => ifless benefit */
        /* recycle add variable - give the compiler chance */
        VX_uint32 add = (x_order) * hw;
        b->x1 += add;
        b->x2 += add - hw;

        add = (y_order) * hw;
        b->y1 += add;
        b->y2 += add - hw;

        add = (z_order) * hw;
        b->z1 += add;
        b->z2 += add - hw;

        /* | faster than + but in this case equivalent */
        root = root->childs[ (z_order<<2) | (y_order<<1) | (x_order) ];
        *lod = (*lod)-1;
    }

    if( root )
        return (VX_byte*)(&root->color);
    return (VX_byte*)(&_oct_black);
}

/* adressing methods - should be extremly efficient! */
static VX_byte * VX_oct_get_voxel( VX_model * self ,
                                   VX_uint32 x ,
                                   VX_uint32 y ,
                                   VX_uint32 z ,
                                   int * lod )
{
    VX_ms_block b;
    return oct_get_voxaddr_impl( self->root , self->dim_size.w , x , y , z , &b , lod );
}

/*
  warn: performance issue!
*/
static void VX_oct_set_voxel( VX_model * self ,
                              VX_uint32 x ,
                              VX_uint32 y ,
                              VX_uint32 z ,
                              VX_uint32 color )
{
    VX_oct_fill_region( self , (VX_ms_block){x,y,z,x+1,y+1,z+1} ,
                        color);
}

/* allocation methods */
static void oct_free_end( VX_oct_node * root )
{
    if( !root )
        return;

    if( root->flags )
    {
        for( int i = 0; i < 8 ; i++ )
        {
            oct_free_end( root->childs[i] );
        }
    }

    free( root );
}

void VX_oct_free( VX_model * self )
{
    if( self->chunk_ptr )
        free( self->chunk_ptr );
    else
        oct_free_end( self->root );

    free( self->data );
    free( self );
}

static void oct_gpu_present(  struct VX_model * self , VX_camera * c,
                              VX_rect clip_rect )
{
    /* stub-call to OpenCL */
    /* TODO: write OpenCL kernel for fast traversing and then update back 
       to camera - handle this and start only one camera thread due to
       inefficiency of concurrent OpenCL computations
    */
}

static void oct_sw_present( struct VX_model * self , VX_camera * c,
                            VX_rect clip_rect )
{
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

static VX_uint32 toARGB32( VX_format * self , VX_byte * chunk ){
    VX_uint32 out = *((VX_uint32*)chunk);
    return out;
}
static VX_format FMT_ARGB32 = {32,4,8,8,8,8,0, toARGB32 };

VX_model * VX_model_octree_new()
{
	VX_model * out = malloc(sizeof(VX_model));
  VX_block b = {0,0,0,0,0,0};

  *out = (VX_model){ NULL , NULL , b , &FMT_ARGB32,
                     VX_oct_compile , VX_oct_dump , VX_oct_inspect,
                     VX_oct_get_voxel,
                     VX_oct_set_voxel,
                     VX_oct_ray_voxel_sstack_fpu,
                     oct_sw_present,
                     VX_oct_load , VX_oct_free , NULL
  };

  out->data = calloc( 1 , sizeof(VX_oct_info) );
  *((VX_oct_info*)out->data) = 
      (VX_oct_info){ 
      0 ,
      sizeof(VX_block) + sizeof( VX_oct_node* ) };

	return out;
}
