 /**
 * @file globus_gass_copy.h
 *
 * @brief Globus GASS copy API 
 *
 * <h2>Introduction</h2>
 *
 * The globus GASS copy library is motivated by the desire to provide a
 * uniform interface to transfer files specified by different protocols.
 *
 * The goals in doing this are to:
 *
 * <ul>
 * <li>Provide a robust way to describe and apply file transfer properties
 * for a variety of protocols. These include the standard HTTP, FTP and 
 * GSIFTP options.  Some of the new file transfer capabilities in GSIFTP are
 * parallel, striping, authentication and TCP buffer sizing.</li>
 *
 * <li>Provide a service to support nonblocking file transfer and handle
 * asynchronous file and network events.</li>
 *
 * <li>Provide a simple and portable way to implement file transfers.</li>
 * </ul>
 *
 * Any program that uses Globus GASS copy functions must include
 * "globus_gass_copy.h".
 *
 */

#ifndef GLOBUS_INCLUDE_GLOBUS_GASS_COPY_H
#define GLOBUS_INCLUDE_GLOBUS_GASS_COPY_H

EXTERN_C_BEGIN

/** Module descriptor
 *
 * Globus GASS copy uses standard Globus module activation and deactivation.
 * Before any Globus GASS copy functions are called, the following function
 * must be called:
 *
 * @code
 *      globus_module_activate(GLOBUS_GASS_COPY_MODULE)
 * @endcode
 *
 *
 * This function returns GLOBUS_SUCCESS if Globus GASS copy was successfully
 * initialized, and you are therefore allowed to subsequently call
 * Globus GASS copy functions.  Otherwise, an error code is returned, and
 * Globus GASS copy functions should not be subsequently called. This
 * function may be called multiple times.
 *
 * To deactivate Globus GASS copy, the following function must be called:
 *
 * @code
 *    globus_module_deactivate(GLOBUS_GASS_COPY_MODULE)
 * @endcode
 *
 * This function should be called once for each time Globus GASS copy was
 * activated.
 *
 */
#define GLOBUS_GASS_COPY_MODULE (&globus_i_gass_copy_module)
extern
globus_module_descriptor_t        globus_i_gass_copy_module;


/** control tcpbuffer Types */
typedef enum
{
    /** Don't change the TCP buffer/window size from the system default */
    GLOBUS_GSIFTP_CONTROL_TCPBUFFER_DEFAULT,

    /** Set the TCP buffer/window size to a fixed value */
    GLOBUS_GSIFTP_CONTROL_TCPBUFFER_FIXED,

    /** Automatically set the TCP buffer/window size */
    GLOBUS_GSIFTP_CONTROL_TCPBUFFER_AUTOMATIC
} globus_gsiftp_control_tcpbuffer_mode_t;
    
/** control tcpbuffer attribute structure  */
typedef struct globus_gsiftp_control_tcpbuffer_s
{
    globus_gsiftp_control_tcpbuffer_mode_t mode;
    union
    {
	/*
	 * No data required for:
	 *     GLOBUS_GSIFTP_CONTROL_TCPBUFFER_DEFAULT
	 */
	struct /* GLOBUS_GSIFTP_CONTROL_TCPBUFFER_FIXED */
	{
	    unsigned long size;
	} fixed;
	struct /* GLOBUS_GSIFTP_CONTROL_TCPBUFFER_AUTOMATIC */
	{
	    unsigned long initial_size;
	    unsigned long minimum_size;
	    unsigned long maximum_size;
	} automatic;
    } data;
} globus_gsiftp_control_tcpbuffer_t;


/** control parallelism Types */
typedef enum {
    /** No parallelism */
    GLOBUS_GSIFTP_CONTROL_PARALLELISM_NONE,

    /** Partitioned parallelism */
    GLOBUS_GSIFTP_CONTROL_PARALLELISM_FIXED,

    /** Blocked parallelism with round-robin */
    GLOBUS_GSIFTP_CONTROL_PARALLELISM_AUTOMATIC,
} globus_gsiftp_control_parallelism_mode_t;
    
/** control parallelism attribute structure  */
typedef struct globus_gsiftp_control_parallelism_s
{
    globus_gsiftp_control_parallelism_mode_t mode;
    union
    {
	/*
	 * No data required for:
	 *     GLOBUS_GSIFTP_CONTROL_PARALLELISM_NONE
	 */
	struct /* GLOBUS_GSIFTP_CONTROL_PARALLELISM_FIXED */
	{
	    unsigned long size;
	} fixed;
	struct /* GLOBUS_GSIFTP_CONTROL_PARALLELISM_AUTOMATIC */
	{
	    unsigned long initial_size;
	    unsigned long minimum_size;
	    unsigned long maximum_size;
	} automatic;
    } data;
} globus_gsiftp_control_parallelism_t;



/** control striping Types */
typedef enum
{
    /** No striping */
    GLOBUS_GSIFTP_CONTROL_STRIPING_NONE,

    /** Partitioned striping */
    GLOBUS_GSIFTP_CONTROL_STRIPING_PARTITIONED,

    /** Blocked striping with round-robin */
    GLOBUS_GSIFTP_CONTROL_STRIPING_BLOCKED_ROUND_ROBIN,
} globus_gsiftp_control_striping_mode_t;

/** control striping attribute structure  */
typedef struct globus_gsiftp_control_striping_s
{
    globus_gsiftp_control_striping_mode_t mode;
    union
    {
	/*
	 * No data required for:
	 *     GLOBUS_GSIFTP_CONTROL_STRIPING_NONE
	 *     GLOBUS_GSIFTP_CONTROL_STRIPING_PARTITIONED
	 */
	struct /* GLOBUS_GSIFTP_CONTROL_STRIPING_BLOCKED_ROUND_ROBIN */
	{
	    unsigned long long block_size;
	} blocked_round_robin;
    } data;
} globus_gsiftp_control_striping_t;

struct globus_gass_copy_handle_s
{
   int       fd;
   void *    user_pointer;
   /* ... add other fields */
} globus_gass_copy_handle_t;

/**
 * Signature of a callback from globus_gass_copy_register_*() functions.
 * (asynchronous transfer functions)
 */
typedef void (*globus_gass_copy_callback_t)(
    void * callback_arg,
    globus_gass_copy_handle_t * handle,
    globus_object_t * result);

/** initialization and destruction of GASS copy handle */
globus_result_t
globus_gass_copy_init(
    globus_gass_copy_handle_t * handle);

globus_result_t
globus_gass_copy_destroy(
    globus_gass_copy_handle_t * handle);


/**
 * copy functions (blocking)
 */
globus_result_t
globus_gass_copy_url_to_url(
    globus_gass_copy_handle_t * handle,
    char * source_url,
    globus_gass_copy_attr_t * source_attr,
    char * dest_url,
    globus_gass_copy_attr_t * dest_attr);

globus_result_t
globus_gass_copy_url_to_handle(
    globus_gass_copy_handle_t * handle,
    char * source_url,
    globus_gass_copy_attr_t * source_attr,
    globus_io_handle_t * dest_handle);

globus_result_t
globus_gass_copy_handle_to_url(
    globus_gass_copy_handle_t * handle,
    globus_io_handle_t * source_handle,
    char * dest_url,
    globus_gass_copy_attr_t * dest_attr);

/**
 * copy functions (asyncronous)
 */
globus_result_t
globus_gass_copy_register_url_to_url(
    globus_gass_copy_handle_t * handle,
    char * source_url,
    globus_gass_copy_attr_t * dest_attr,
    char * dest_url,
    globus_gass_copy_attr_t * source_attr,
    globus_gass_copy_callback_t callback_func,
    void * callback_arg);

globus_result_t
globus_gass_copy_register_url_to_handle(
    globus_gass_copy_handle_t * handle,
    char * source_url,
    globus_gass_copy_attr_t * source_attr,
    globus_io_handle_t * dest_handle,
    globus_gass_copy_callback_t callback_func,
    void * callback_arg);

globus_result_t
globus_gass_copy_register_handle_to_url(
    globus_gass_copy_handle_t * handle,
    globus_io_handle_t * source_handle,
    char * dest_url,
    globus_gass_copy_attr_t * dest_attr,
    globus_gass_copy_callback_t callback_func,
    void * callback_arg);

/**
 * cache handles functions
 *
 * Use this when transferring mulitple files from or to the same host
 */
globus_result_t
globus_gass_copy_cache_url_state(
    globus_gass_copy_handle_t * handle,
    char * url);

globus_result_t
globus_gass_copy_flush_url_state(
    globus_gass_copy_handle_t * handle,
    char * url);

/**
 *  get/set user pointers from/to GASS copy handles
 */
globus_result_t
globus_gass_copy_set_user_pointer(
    globus_gass_copy_handle_t * handle,
    void * user_data);

void *
globus_gass_copy_get_user_pointer(
    globus_gass_copy_handle_t * handle);


/**
 * Set Attribute functions
 */

/* TCP buffer/window size */
globus_result_t
globus_gass_copy_attr_set_tcpbuffer(
    globus_gass_copy_attr_t * attr,
    globus_gsiftp_control_tcpbuffer_t * tcpbuffer_info);

/* parallel transfer options */
globus_result_t
globus_gass_copy_attr_set_parallelism(
    globus_gass_copy_attr_t * attr,
    globus_gsiftp_control_parallelism_t * parallelism_info);

/* striping options */
globus_result_t
globus_gass_copy_attr_set_striping(
    globus_gass_copy_attr_t * attr,
    globus_gsiftp_control_striping_t * striping_info);

/* authorization options */
globus_result_t
globus_gass_copy_attr_set_authorization(
    globus_gass_copy_attr_t * attr,
    globus_io_authorization_t * authorization_info);

/* secure channel options */
globus_result_t
globus_gass_copy_attr_set_secure_channel(
    globus_gass_copy_attr_t * attr,
    globus_io_secure_channel_t * secure_channel_info);

/**
 * Get Attribute functions
 */

EXTERN_C_END

#endif /* GLOBUS_INCLUDE_GLOBUS_GASS_COPY_H */
