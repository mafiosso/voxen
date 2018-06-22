#ifndef STREAM_H
#define STREAM_H

typedef struct VX_promise{
    void (*func_call)( void * shared_args , void * user_args , void * ret );
    void * args;
}VX_promise;

typedef struct VX_stream{
    VX_promise ** stack_promises;

    int stack_top;
    int stack_max_size;

    void (*call)( struct VX_stream * self , void * shared_args , void * ret );
    void (*promise)( struct VX_stream * self , VX_promise * p );
}VX_stream;

#endif
