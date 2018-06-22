#ifndef DISPLAY_H
#define DISPLAY_H

/**
 * @file display.h 
*/

/* flag structure 
   |0| .... |FULLSCREEN|
*/

/** Flag specifying if initialize with fullscreen */
#define VX_FULLSCREEN 0x1

typedef struct VX_machine{
    /** Initialize VX_machine self with resolution given by resolution and with flags.
     * \param self
     * \param resoulution rectangle where is important only information about width and height, x and y values are ignored
     * \param flags is set of possible VoXen flags (commonly VX_FULLSCREEN)
     * \return returns 0 on success, not 0 on fail, some implementations may exit with -1
     */
		int          (*init)( struct VX_machine * self , VX_rect resolution , VX_uint32 flags );

    /** Quits machine self and does all needed frees and deallocations. 
     * \param self
     * \return 0 on success, not 0 on fail
     */
    int          (*quit)( struct VX_machine * self );
    /** Allocates new device-independent surface with size w,h and format ARGB32.
        \param w width of image
        \param h height of image
        \return valid VX_surface pointer on success, NULL when failed.
     */
    VX_surface * (*make_surface)( VX_uint32 w , VX_uint32 h );
    /** Redraws framebuffer of the machine.
     * \param self
     */
    void         (*redraw)( struct VX_machine * self );

    /** \var resolution x and y are arbitrary, w and h is set by size of image (read only)*/
		VX_rect      resolution;
    /** \var flags stores flags (read only) */
		VX_uint32    flags;
    /** \var native_surface pointer to VX_surface */
		VX_surface * native_surface;
    /** \var cores_count number of CPU cores in your system */
    int          cores_count;

    /** Custom data pointer, may be accessed only by programmer of driver */
    void * custom_dta;
}VX_machine;

/** Predefined 2D driver using SDL (Simple direct layer) as its backend. */
extern VX_machine VX_DRV_sdl;
/** Predefined 2D driver using raw VX_uint32 arrays as its backend. */
extern VX_machine VX_DRV_null;
/** Global main object of library, which operates with machine. */
extern VX_machine VX_lib;

/** Initialize driver and set it as main object (VX_lib).
 * \param driver may be arbitrary structure of type VX_machine, common use is VX_DRV_sdl or VX_DRV_null or any other self written driver.
 * \param w width of framebuffer
 * \param h height of framebuffer
 * \param flags flags for machine, commonly 0 or VX_FULLSCREEN
*/
int VX_init( VX_machine driver , VX_uint32 w , VX_uint32 h , VX_uint32 flags );

/** Undocumented inner function */
void VX_machine_default_init( VX_machine * m );

#endif
