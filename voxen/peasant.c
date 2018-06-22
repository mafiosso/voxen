/*
  support for threaded main loop
  my barrier vision - use threads only if needed and blocking like
*/
#include "VX_lib.h"

static void field_destroy( VX_field * self ){
    for( int i = 0 ; i < self->peasants_count ; i++ ){
        self->workers[i].destroy_p = 1;

        if( i > 0 ){
            self->workers[i].plant = NULL;
            sem_post( &(self->workers[i].sem_in) );
            sem_wait( &(self->workers[i].sem_out) );
        }
    }

    for( int i = 0 ; i < self->peasants_count-1 ; i++ ){
        pthread_join( self->t[i] , NULL );
    }

    free( self->t );
    free( self->workers );
    free( self );
}

static void field_cultivate( VX_field * self , void ** params , void (*funcs[])(void*) )
{
    for( int i = 1 ; i < self->peasants_count ; i++ ){
        self->workers[i].param = params[i];
        self->workers[i].plant = funcs[i];
        sem_post( &(self->workers[i].sem_in) );
    }

    funcs[0]( params[0] );

    for( int i = 1 ; i < self->peasants_count ; i++ ){
        sem_wait( &(self->workers[i].sem_out) );
        // this have undefined behavior see man pages of pthread
        //sem_init( &(self->workers[i].sem_out) , 0 , 0 );
    }
}

static void work(VX_peasant * self ){
    while( 1 ){
        sem_wait( &(self->sem_in) );
        
        if( !self->plant ){
            break;
        }

        /* should be very power consumptive function */
        self->plant( self->param );
        
        /* do this twice because waiting in driving thread */
        sem_post( &(self->sem_out) );
    }
    
    sem_post( &(self->sem_out) );
}

static int field_make( VX_field * self ){
    /* start threads */
    int p_cnt = self->peasants_count-1;
    pthread_t * threads = malloc(sizeof(pthread_t)*p_cnt);
    int ret = 0;

    for( int i = 0 ; i < p_cnt ; i++ ){
        ret += pthread_create( &(threads[i]) , NULL ,
                               (void * (*)(void *))&work ,
                               &(self->workers[i+1]) );

    }

    self->t = threads;

    return ret;
}

VX_field * VX_field_new( VX_uint32 peasants_count )
{
    if( !peasants_count )
        return NULL;

    VX_field * out = calloc( 1 , sizeof(VX_field) );
    out->peasants_count = peasants_count;
    out->workers = calloc(peasants_count , sizeof(VX_peasant));

    for( int i = 0 ; i < peasants_count ; i++ ){
        sem_init( &(out->workers[i].sem_in) , 0 , 0 );
        sem_init( &(out->workers[i].sem_in) , 0 , 0 );
    }

    if( field_make( out ) ){
        printf("err: can not create threads!\n");
        for( int i = 0 ; i < peasants_count ; i++ ){
            free(out->workers);
        }

        free( out );
        return NULL;
    }

    out->destroy = field_destroy;
    out->enter = field_cultivate;
    return out;
}
