#include "VX_lib.h"
#include "stream.h"


VX_promise * VX_promise_new( void (*func) , void * user_args )
{
    VX_promise * out = malloc(sizeof(VX_promise));
    *out = (VX_promise){ func , user_args };
    return out;
}

static void s_call( VX_stream * self , void * shared_args , void * ret )
{
    int i = self->stack_top;

    while( i >= 0 )
    {
        VX_promise * p = self->stack_promises[i];
        p->func_call( shared_args , p->args , ret );
        i--;
    }
}

static void s_promise( VX_stream * self , VX_promise * p )
{
    self->stack_promises[ self->stack_top++ ] = p;

    if( self->stack_max_size >= self->stack_top )
    {
        self->stack_max_size *= 2;
        self->stack_promises = realloc( self->stack_promises ,
                                        sizeof(void*) * self->stack_max_size );
    }
}

VX_stream * VX_stream_new( VX_uint32 default_stack_size )
{
    VX_stream * out = calloc(sizeof(VX_stream) , 0);

    out->stack_promises = malloc( sizeof( void* ) * default_stack_size );
    out->stack_max_size = default_stack_size;
    out->stack_top = 0;
    out->call = s_call;
    out->promise = s_promise;

    return out;
}
