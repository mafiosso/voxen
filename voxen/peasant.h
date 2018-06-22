#ifndef PEASANT_H
#define PEASANT_H

/**
 * @file peasant.h
 */

/*! Undocumented structure for almost inner purposes */
typedef struct VX_peasant{
    sem_t sem_in;
    sem_t sem_out;
    void (*plant)(void*);
    void * param;
    int destroy_p;
}VX_peasant;

/*! Structure holding set of threads */
typedef struct VX_field{
    /** information about count of threads */
    int peasants_count;
    /** is private and should not be changed */
    int destroy_p;
    /** is array of threads and should not be changed */
    pthread_t *t;
    /** workers array of threads, should not be changed */
    VX_peasant * workers;
    /** Executes all threads, this call blocks calling thread until done
     *  \param self this pointer analogy
     *  \param params array of parameters, must be same the same size as peasants_count
     *  \param funcs array of functions, that returns void and takes one void* parameter
     */
    void (*enter)(struct VX_field* , void ** params , void (*funcs[])(void *) );
    /** Standard destructor of VX_field, stops all threads and frees them
     * \param self analogy to this pointer
     */
    void (*destroy)(struct VX_field*);
}VX_field;

/*!
 *  \brief Constructor that creates peasants_count-1 threads, one job runs in calling thread
 *  \param peasants_count is not 0 argument, specifying count of threads
 *  \return returns VX_field pointer, or NULL if error occured or peasants_count is 0
 */
VX_field * VX_field_new( VX_uint32 peasants_count );

#endif
