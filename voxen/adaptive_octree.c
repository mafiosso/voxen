#include "VX_lib.h"
#include "ao_traversal.h"

static int aoptimal_p( VX_aoct_node * root ){
    /* raw case */
    if( root->flags & (1 << 31) )
        return 0;
    
    /* oct case - warn: test only the first byte in future! */
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

static void aoct_free_end( VX_aoct_node * root )
{
    if( !root )
        return;

    VX_uint32 nflags = root->flags;

    /* raw free */
    if( nflags & ( 1 << 31  ) ){
        free( root );
        return;
    }

    if( root->flags )
    {
        for( int i = 0; i < 8 ; i++ )
        {
            aoct_free_end( root->childs[i] );
        }
    }

    free( root );
}

static VX_aoct_node * aoptimize( VX_model * self , VX_aoct_node * root ,
                                 VX_uint32 * bytes_used , VX_uint32 lvl ,
                                 VX_model * src , 
                                 VX_uint32 offset_x , VX_uint32 offset_y ,
                                 VX_uint32 offset_z )
{
    int optimality = aoptimal_p( root );
    VX_uint32 color;
    VX_aoct_node * out = NULL;

    switch( optimality ){
    case 0: /* can't compile */
        out = root;
        break;
    case 1: /* empty case */
        free( root );
        *bytes_used = 0;
        /* out = NULL; */
        break;
    case 2: /* same color case */
        color = root->childs[0]->color;
        aoct_free_end( root );
        root = malloc( sizeof( VX_aoct_node_leaf ) );
        root->flags = 0;
        root->color = color;
        out = root;
        *bytes_used = sizeof( VX_aoct_node_leaf );
        break;
    default:
        out = root;
        break;
    }

    VX_uint64 N = (1 << lvl);
    VX_uint64 raw_sz = ((N*N*N * 4) + 4);

    if( *bytes_used > raw_sz ){
        VX_uint32 dim = 1 << lvl;
        VX_byte * raw_node = malloc( raw_sz );
        *((VX_uint32*)raw_node) = 1 << 31;
        VX_uint32 * it = (VX_uint32*)(raw_node + 4);
        VX_format * fmt = src->fmt;

        for( VX_uint32 z = 0 ; z < dim ; z++ ){
            for( VX_uint32 y = 0 ; y < dim ; y++ ){
                for( VX_uint32 x = 0 ; x < dim ; x++ ){
                    int lod = 0xff;
                    VX_byte * cptr = src->get_voxel( src ,
                                                     x + offset_x ,
                                                     y + offset_y ,
                                                     z + offset_z , &lod );
                    (*it) = fmt->ARGB32( fmt , cptr );
                    it++;
                }
            }
        }
        aoct_free_end( out );
        out = (VX_aoct_node*)raw_node;
        *bytes_used = raw_sz;
    }

    return out;
}

static VX_aoct_node * aoct_compile_impl( VX_model * self, 
                                         VX_model * m ,
                                         VX_block bbox ,
                                         VX_uint32 lvl ,
                                         VX_uint32 * sz )

{
    VX_aoct_node * out = NULL;
    VX_format * fmt = m->fmt;
    
    if( bbox.w <= 1 )
    {
        int lod = 0xff;
        VX_byte * color_ptr = m->get_voxel( m , bbox.x , bbox.y ,
                                            bbox.z , &lod );
        if( !color_ptr ){
            /* *sz += 0; */
            return NULL;
        }
        
        VX_uint32 color = fmt->ARGB32( fmt , color_ptr );
        
        /* empty */
        if( color == fmt->colorkey ){
            /* *sz += 0; */
            return NULL;
        }
        
        out = calloc( 1 , sizeof(VX_oct_node_leaf) );
        out->color = color;
        (*sz) += sizeof(VX_oct_node_leaf);

        return out;
    }

    VX_block c = bbox;

    c.w /= 2;
    c.h /= 2;
    c.d /= 2;

    out = malloc( sizeof(VX_aoct_node) );
    out->flags = 0;

    VX_uint32 mysz = sizeof(VX_aoct_node);

    for( int z = 0 ; z < 2 ; z++ )
    {
        for( int y = 0 ; y < 2 ; y++ )
        {
            for( int x = 0 ; x < 2 ; x++ )
            {
                VX_uint32 idx = (z*4)+(y*2)+x;
                VX_aoct_node * in = aoct_compile_impl( self , m ,
                                                     (VX_block){
                                                         bbox.x+(x*c.w) ,
                                                             bbox.y+(y*c.h) ,
                                                             bbox.z+(z*c.d) ,
                                                             c.w, c.w , c.w },
                                                     lvl-1,
                                                     &mysz );

                out->childs[idx] = in;
                out->flags |= ((in != NULL) << idx);
            }
        }
    }

    VX_aoct_node * root = aoptimize( self , out ,
                                     &mysz , lvl ,
                                     m , 
                                     bbox.x , bbox.y ,
                                     bbox.z );



    *sz += mysz;
    return root;    
}

int VX_aoct_compile( VX_model * self ,
                     VX_model * in )
{
    VX_uint32 w = MAX( in->dim_size.d , 
                       MAX( in->dim_size.w , in->dim_size.h ) );

    w = nearest_pow2( w );

    VX_block bbox = { 0,0,0, w , w, w };
    int lvl = ilog2( w );
    VX_uint32 sz = 0;

    printf( "Adaptive octree size: %d\n" , w );
    self->root = aoct_compile_impl( self , in , bbox , lvl , &sz );

    self->dim_size = bbox;

    printf("sz: %u\n" , sz );

    ((VX_oct_info*)self->data)->stack_len = lvl;    

    return (self->root ? 0 : -1);
}

static void aoct_inspect_impl( VX_aoct_node * n , int * nodes ,
                               int * leaf_nodes , int * raws ,
                               VX_uint32 * total_bytes , int lvl )
{
    if( !n )
        return;

    if( n->flags & (1 << 31) ){
        (*raws) += 1;
        VX_uint32 N = (1 << lvl);
        (*total_bytes) += ((N*N*N*4) + 4);
        return;
    }

    *nodes = (*nodes + 1);

    if( !n->flags )
    {
        (*total_bytes) += sizeof(VX_aoct_node_leaf);
        (*leaf_nodes)++;
        return;
    }

    (*total_bytes) += sizeof(VX_aoct_node);

    for( int i = 0; i < 8 ; i++ )
    {
        aoct_inspect_impl( n->childs[i] , nodes , leaf_nodes , raws ,
                           total_bytes , lvl-1 );
    }
}

static void VX_aoct_inspect( VX_model * self ){
    VX_uint32 w = MAX( self->dim_size.d , 
                       MAX( self->dim_size.w , self->dim_size.h ) );
    
    w = nearest_pow2( w );
    int lvl = ilog2( w );

    int voxels = 0;
    int leaf_vox = 0;
    int raws = 0;
    VX_uint32 total_bytes = 0;

    aoct_inspect_impl( self->root , &voxels , &leaf_vox , &raws ,
                      &total_bytes , lvl );

    VX_uint32 size = total_bytes;

    printf("Adaptive octree info: size %d x %d x %d \n" , 
           self->dim_size.w , self->dim_size.h , self->dim_size.d );
    printf("Adaptive octree stats: nodes %d total B used: %u\nleaf: %d raws: %d\nlvl: %d\n" ,
           voxels ,
           size ,
           leaf_vox , raws, lvl );


}

static void VX_aoct_free( VX_model * self ){
    aoct_free_end( self->root );
    free( self->data );
    free( self );
}

static VX_uint32 _oct_black = 0;

static inline VX_byte * aoct_get_vox_impl( VX_model * self ,
                                           VX_uint32 x,
                                           VX_uint32 y,
                                           VX_uint32 z,
                                           VX_uint32 * lod)
{
    VX_uint32 hw = self->dim_size.w;
    VX_uint32 b [4] = {0,0,0, hw};
    VX_oct_node * root = self->root;
    VX_uint32 z_order, y_order, x_order;
    VX_uint32 mask = 1 << ((VX_oct_info*)self->data)->stack_len-1;

    while( root && root->flags /*&& (*lod > 0)*/ )
    {
        if( (root->flags & (1 << 31) ) ){
            /* its a raw subnode */
            //printf("raw \n"  );

            VX_uint32 dpos[3] = { x - b[X] ,
                                  y - b[Y] , 
                                  z - b[Z] };
            VX_byte * raw = (VX_byte*)root;

            return raw + ((1 + (dpos[Z] * hw * hw) +
                           (dpos[Y] * hw) + dpos[X]) * 4);
        }

        hw >>= 1;

        z_order = (z & mask) > 0;
        y_order = (y & mask) > 0;
        x_order = (x & mask) > 0;

        /* 0 or 1 * hw will give hw or 0 => ifless benefit */
        /* recycle add variable - give the compiler chance */
        VX_uint32 add = (x_order) * hw;
        b[X] += add;
        add = (y_order) * hw;
        b[Y] += add;
        add = (z_order) * hw;
        b[Z] += add;

        /* | faster than + but in this case equivalent */
        root = root->childs[ (z_order<<2) | (y_order<<1) | (x_order) ];
        *lod = (*lod)-1;

        mask >>= 1;
    }

    if( root )
        return (VX_byte*)(&root->color);
    return (VX_byte*)(&_oct_black);
}

/* adressing methods - should be extremly efficient! */
static VX_byte * VX_aoct_get_voxel( VX_model * self ,
                                    VX_uint32 x ,
                                    VX_uint32 y ,
                                    VX_uint32 z ,
                                    int * lod )
{
    return aoct_get_vox_impl( self , x , y , z , lod );
}

static VX_uint32 toARGB32( VX_format * self , VX_byte * chunk ){
    VX_uint32 out = *((VX_uint32*)chunk);
    return out;
}
static VX_format FMT_ARGB32 = {32,4,8,8,8,8,0, toARGB32 };

typedef struct VX_AOCT_header{
    VX_uint32 size;
    VX_uint32 checksum;
    VX_uint32 entry_offset;
}VX_AOCT_header;

/* Due to x86 and AMD64 interportability */
typedef struct _aoct_file_node{
    VX_uint32 flags;
    VX_uint32 color;
    VX_uint32 childs_offsets[8];
}_aoct_file_node;

typedef struct _aoct_file_node_leaf{
    VX_uint32 flags;
    VX_uint32 color;
}_aoct_file_node_leaf;


static int aoct_dump_end( VX_aoct_node * root , FILE * descriptor ,
                          VX_uint32 * idx , VX_uint32 sz , VX_uint32 * myidx )
{
    if( root->flags & (1 << 31 ) ){
        VX_uint32 fsz = sizeof(VX_uint32)*((sz*sz*sz)+1);
        (*myidx) = *idx;
        (*idx) = (*idx) + fsz;
        printf("writing: %u raw bytes sz: %u\n" , fsz , sz);
        return fwrite( root , fsz , 1 , descriptor );
    }

    if( !root->flags ){
        (*myidx) = *idx;
        (*idx) += sizeof( _aoct_file_node_leaf );
        return fwrite( root , sizeof( _aoct_file_node_leaf ) , 1 , descriptor );
    }

    _aoct_file_node dump_node = {root->flags , root->color};
    int out = 1;
    

    for( int i = 0 ; i < 8 ; i++ )
    {
        if( root->childs[i] )
        {
            VX_uint32 chidx;
            out &= aoct_dump_end( root->childs[i] , descriptor , idx , sz/2 , &chidx );
            dump_node.childs_offsets[i] = chidx;
        }
        else{
            dump_node.childs_offsets[i] = 0;
        }
    }
    
    (*myidx) = *idx;
    (*idx) += sizeof(_aoct_file_node);

    out &= fwrite( &dump_node , sizeof(_aoct_file_node) , 1 , descriptor );
    return out;
}

int VX_aoct_dump( VX_model * self , const char * out_file )
{
    FILE * f = fopen( out_file , "w+" );

    if( !f )
        return -1;
    
    VX_AOCT_header header = { self->dim_size.w , 
                              (self->dim_size.w*self->dim_size.h*self->dim_size.d)+0xdead+0xbeaf };

    VX_aoct_node * r = self->root;
    if( !r->flags || (r->flags & (1 << 31)) ){
        header.entry_offset = 0;
    }
    else{
        header.entry_offset = 0xFFFFFFFF;
    }

    if( !fwrite(&header , sizeof(VX_AOCT_header) , 1 , f ) )
    {
        fclose( f );
        return -1;
    }
    VX_uint32 rix;

    VX_uint32 stack_top = sizeof(header);
    int ret = aoct_dump_end( self->root , f , &stack_top , header.size , &rix );
    fclose( f );

    printf("info: stack-top: %d\n" , stack_top );
    return (ret ? 0 : -1);
}

/* 
   octree loading - rebuild only tree from file - 
   faster will be a self allocation structure but complicated
*/
static VX_aoct_node * aoct_load_end( _aoct_file_node * pool_root ,
                                     VX_byte * pool , VX_uint32 sz )
{
    if( pool_root->flags == (1 << 31 ) ){
        VX_uint32 fsz = sizeof(VX_uint32) * (1+(sz*sz*sz));
        VX_uint32 * out = malloc(fsz);
        memcpy( out , pool_root , fsz );
        return (VX_aoct_node*)out;
    }

    VX_aoct_node * out = NULL;
    /* leaf FILE node -> allocate leaf RAM node */
    if( !(pool_root->flags) ){
        out = malloc( sizeof(VX_oct_node_leaf) );
        out->flags = 0;
        out->color = pool_root->color;
        return out;
    }

    out = malloc( sizeof(VX_aoct_node) );
    out->flags = pool_root->flags;
    out->color = pool_root->color;

    for( int i = 0 ; i < 8 ; i++ ){
        VX_byte * addr = pool + (pool_root->childs_offsets[i]);
        if( addr != pool ){
            out->childs[i] = aoct_load_end( (_aoct_file_node*)addr , pool , sz/2 );
        }
        else{
            out->childs[i] = NULL;
        }
    }

    return out;
}

int VX_aoct_load( VX_model * self , const char * path )
{
    FILE * f = fopen( path , "r" );

    if( !f )
        return -1;

    fseek( f , 0 , SEEK_END );
    size_t sz = ftell( f );
    rewind( f );

    /* read header */
    struct VX_AOCT_header header;
    if( !fread( &header , sizeof( struct VX_AOCT_header ) , 1 , f ) )
    {
        fclose( f );
        return -1;
    }

    /* compute header's checksum */
    if( ((header.size * header.size * header.size)+0xbeaf+0xdead) != header.checksum ){
        printf("err: not correct .aoct file!\n");
        fclose( f );
        return -1;
    }

    sz -= sizeof( struct VX_AOCT_header );
    void * voxel_pool = malloc(sz);

    if( !fread( voxel_pool , sizeof(VX_byte) , sz , f ) )
    {
        fclose( f );
        free( voxel_pool );
        printf("err: reading failed!\n");
        return -1;
    }

    self->dim_size.w = header.size;
    self->dim_size.h = header.size;
    self->dim_size.d = header.size;

    printf("aoct size: %u\n" , header.size );

    /* model with only one voxel */
    if( sz == sizeof( _aoct_file_node_leaf ) ) 
    {
        self->root = aoct_load_end( voxel_pool , voxel_pool - sizeof(header) , header.size );
    }
    else
    {
        if( !header.entry_offset ){ /* begin with raw */
            self->root = aoct_load_end( voxel_pool , voxel_pool - sizeof(header) , header.size );
        }
        else{
            _aoct_file_node * pr = (voxel_pool+sz-sizeof(_aoct_file_node));
            self->root = aoct_load_end( pr , voxel_pool - sizeof(header) , header.size );
        }
    }

    free( voxel_pool );
    fclose( f );

    oct_update_stack_len( self );
    ((VX_oct_info*)self->data)->buffer_size = 0;
    return 0;
}

static void aoct_sw_present( struct VX_model * self , VX_camera * c,
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

VX_model * VX_adaptive_octree_new()
{
	VX_model * out = malloc(sizeof(VX_model));
  VX_block b = {0,0,0,0,0,0};

  *out = (VX_model){ NULL , NULL , b , &FMT_ARGB32,
                     VX_aoct_compile , VX_aoct_dump , VX_aoct_inspect,
                     VX_aoct_get_voxel,
                     NULL,
                     VX_ao_ray/* ray_get_voxel */,
                     aoct_sw_present/* present */,
                     VX_aoct_load , VX_aoct_free , NULL
  };

  out->data = calloc( 1 , sizeof(VX_oct_info) );
  *((VX_oct_info*)out->data) = 
      (VX_oct_info){ 
      0 ,
      sizeof(VX_block) + sizeof( VX_aoct_node* ) };

	return out;
}
