/*
 * Copyright 1999-2006 University of Chicago
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if !defined GLOBUS_XIO_DRIVER_HTTP_H
#define GLOBUS_XIO_DRIVER_HTTP_H 1

/**
 * @file globus_xio_http.h Globus XIO HTTP Driver Header
 */
#include "globus_xio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup globus_xio_http_driver Globus XIO HTTP Driver
 * @ingroup globus_xio
 * This driver implements the HTTP/1.0 and HTTP/1.1 protocols within
 * the Globus XIO framework. It may be used with the tcp driver for
 * the standard HTTP protocol stack, or may be combined with the gsi
 * driver for a HTTPS implementation.
 *
 * This implementation supports user-defined HTTP headers, persistent
 * connections, and chunked transfer encoding.
 */

/**
 * @defgroup globus_xio_http_driver_instance Opening/Closing
 * @ingroup globus_xio_http_driver
 *
 * An XIO handle with the http driver can be created with either 
 * @ref globus_xio_handle_create() or
 * @ref globus_xio_server_register_accept().
 *
 * If the handle is created with
 * @ref globus_xio_server_register_accept(), then an HTTP service handle
 * will be created when @ref globus_xio_register_open() is called. The XIO
 * application must call one of the functions in the @ref globus_xio_read()
 * family to receive the HTTP request metadata. This metadata will be returned
 * in the data descriptor associated with that first read: the application
 * should use the GLOBUS_XIO_HTTP_GET_REQUEST descriptor cntl to extract
 * this metadata.
 *
 * If the handle is created with @ref globus_xio_handle_create(), then
 * an HTTP client handle will be created when
 * @ref globus_xio_register_open() is called. HTTP request headers, version and
 * method may be chosen by setting attributes.
 */

/**
 * @defgroup globus_xio_http_driver_io Reading/Writing
 * @ingroup globus_xio_http_driver
 *
 * The HTTP driver behaves similar to the underlying transport driver
 * with respect to reads and writes with the exception that metadata must
 * be passed to the handle via open attributes on the client side and will
 * be received as data descriptors as part of the first request read or
 * response read.
 */

/**
 * @defgroup globus_xio_http_driver_server Server
 * @ingroup globus_xio_http_driver
 *
 * The @ref globus_xio_server_create() causes a new transport-specific
 * listener socket to be created to handle new HTTP connections.
 * @ref globus_xio_server_register_accept() will accept a new
 * connection for processing. @ref globus_xio_server_register_close()
 * cleans up the internal resources associated with the http server
 * and calls close on the listener.
 *
 * Multiple HTTP requests may be read in sequence from an HTTP
 * server. After each request is processed and the response is sent (either by
 * writing the entire entity body as specified by the Content-Length header or 
 * by using the GLOBUS_XIO_HTTP_HANDLE_SET_END_OF_ENTITY handle cntl), the next
 * read will contain the metadata related to the next operation.
 * Only one request will be in process at once--the previous request must have
 * sent or received and EOF (whichever is applicable to the request type).
 */

/**
 * @defgroup globus_xio_http_driver_cntls Attributes and Cntls
 * @ingroup globus_xio_http_driver
 * 
 * HTTP driver specific attrs and cntls.
 * 
 * @see globus_xio_attr_cntl()
 * @see globus_xio_handle_cntl()
 */

/**
 * @defgroup globus_xio_http_driver_errors Error Types
 * @ingroup globus_xio_http_driver
 *
 * In addition to errors generated by underlying protocol drivers, the XIO
 * HTTP driver defines a few error conditions specific to the HTTP protocol.
 *
 * @see globus_xio_driver_error_match()
 */
/** doxygen varargs filter stuff
 * GlobusVarArgDefine(
 *      attr, globus_result_t, globus_xio_attr_cntl, attr, driver)
 * GlobusVarArgDefine(
 *      handle, globus_result_t, globus_xio_handle_cntl, handle, driver)
 * GlobusVarArgDefine(
 *      dd, globus_result_t, globus_xio_data_descriptor_cntl, dd, driver)
 */

/**
 * HTTP Header
 * @ingroup globus_xio_http_driver
 */
typedef struct
{
    /** Header Name */
    char *                              name;
    /** Header Value */
    char *                              value;
}
globus_xio_http_header_t;

/**
 * HTTP driver specific cntls
 * @ingroup globus_xio_http_driver_cntls
 */
typedef enum
{
    /**
     * GlobusVarArgEnum(handle)
     * Set the value of a response HTTP header. 
     * @ingroup globus_xio_http_driver_cntls
     *
     * @param header_name
     *        Name of the HTTP header to set.
     * @param header_value
     *        Value of the HTTP header
     *
     * Certain headers will cause changes in how the HTTP protocol will
     * be handled. These include:
     * - Transfer-Encoding: {identity|chunked}
     *   Override the default transfer encoding. If a server knows the
     *   exact length of the message body, or does not intend to support
     *   persistent connections, it may set this header to be
     *   "identity".<br><br>
     *   If this is set to "identity" and any of the following are true, then
     *   the connection will be closed after the end of the response is sent:
     *   <br><br>
     *   - A Content-Length header is not present
     *   - The HTTP version is set to "HTTP/1.0"
     *   - The Connection header is set to "close"
     *   Attempts to set this to "chunked" with an "HTTP/1.0" client will
     *   fail with a GLOBUS_XIO_ERROR_HTTP_INVALID_HEADER error.
     * - Content-Length: 1*Digit
     *   - Provide a content length for the response message. If the 
     *     "chunked" transfer encoding is being used, then this header
     *     will be silently ignored by the HTTP driver.
     * - Connection: close
     *   - The HTTP connection will be closed after the end of the data
     *     response is written.
     *
     * @return This handle control function can fail with
     * - GLOBUS_XIO_ERROR_MEMORY
     * - GLOBUS_XIO_ERROR_PARAMETER 
     * - GLOBUS_XIO_ERROR_HTTP_INVALID_HEADER
     */
    /* const char *                     header_name,
       const char *                     header_value */
    GLOBUS_XIO_HTTP_HANDLE_SET_RESPONSE_HEADER,
    /** GlobusVarArgEnum(handle)
     * Set the response status code.
     * @ingroup globus_xio_http_driver_cntls
     *
     * @param status
     * Value in the range 100-599 which will be used as the HTTP response
     * code, as per RFC 2616.
     *
     * If this cntl is not called by a server, then
     * the default value of 200 ("Ok") will be used. If this is called on the
     * client-side of an HTTP connection, the handle control will fail with a
     * GLOBUS_XIO_ERROR_PARAMETER error.
     *
     * @return This handle control function can fail with
     * - GLOBUS_XIO_ERROR_PARAMETER 
     */
    /* int status */
    GLOBUS_XIO_HTTP_HANDLE_SET_RESPONSE_STATUS_CODE,
    /** GlobusVarArgEnum(handle)
     * Set the response reason phrase.
     * @ingroup globus_xio_http_driver_cntls
     *
     * @param reason
     *        The value of the HTTP response string, as per RFC 2616.
     *
     * If this cntl is not called by a server, then a default value based on
     * the handle's response status code will be generated. If this is called
     * on the client-side of an HTTP connection, the handle control will fail
     * with a GLOBUS_XIO_ERROR_PARAMETER error.
     *
     * @return This handle control function can fail with
     * - GLOBUS_XIO_ERROR_MEMORY
     * - GLOBUS_XIO_ERROR_PARAMETER 
     */
    /* const char * reason */
    GLOBUS_XIO_HTTP_HANDLE_SET_RESPONSE_REASON_PHRASE,
    /** GlobusVarArgEnum(handle)
     * Set the response HTTP version.
     * @ingroup globus_xio_http_driver_cntls
     *
     * @param version
     *        The HTTP version to be used in the server response line.
     *
     * If this cntl is not called by a server, then the default of
     * GLOBUS_XIO_HTTP_VERSION_1_1 will be used, though no HTTP/1.1 features 
     * (chunking, persistent connections, etc) will be
     * assumed if the client request was an HTTP/1.0 request. If this is
     * called on the client-side of an HTTP connection, the handle control
     * will fail with GLOBUS_XIO_ERROR_PARAMETER.
     *
     * @return This handle control function can fail with
     * - GLOBUS_XIO_ERROR_MEMORY
     * - GLOBUS_XIO_ERROR_PARAMETER
     */
    /* globus_xio_http_version_t version */
    GLOBUS_XIO_HTTP_HANDLE_SET_RESPONSE_HTTP_VERSION,
    /** GlobusVarArgEnum(handle)
     * Indicate end-of-entity for an HTTP body.
     * @ingroup globus_xio_http_driver_cntls
     *
     * HTTP clients and servers must call this command to indicate to the
     * driver that the entity-body which is being sent is completed.
     * Subsequent attempts to write data on the handle will fail.
     *
     * This handle command MUST be called on the client side of an HTTP
     * connection when the HTTP method is OPTIONS, POST, or PUT, or when
     * the open attributes indicate that an entity will be sent. This handle
     * command MUST be called on the server side of an HTTP request connection
     * when the HTTP method was OPTIONS, GET, POST, or TRACE.
     */
    GLOBUS_XIO_HTTP_HANDLE_SET_END_OF_ENTITY,
    GLOBUS_XIO_HTTP_HANDLE_SET_REQUEST_HEADER
}
globus_xio_http_handle_cmd_t;

/**
 * HTTP driver specific attribute and data descriptor cntls
 * @ingroup globus_xio_http_driver_cntls
 */
typedef enum
{
    /** GlobusVarArgEnum(attr)
     * Set the HTTP method to use for a client request.
     * @ingroup globus_xio_http_driver_cntls
     *
     * @param method
     *        The request method string ("GET", "PUT", "POST", etc) that will
     *        be used in the HTTP request.
     *
     * If this is not set on the target before it is opened, it will default
     * to GET.
     *
     * This attribute is ignored when opening the server side of an HTTP
     * connection.
     *
     * Setting this attribute may fail with
     * - GLOBUS_XIO_ERROR_MEMORY
     * - GLOBUS_XIO_ERROR_PARAMETER
     */
    /* const char * method */
    GLOBUS_XIO_HTTP_ATTR_SET_REQUEST_METHOD,
    /** GlobusVarArgEnum(attr)
     * Set the HTTP version to use for a client request.
     * @ingroup globus_xio_http_driver_cntls
     *
     * @param version 
     * The HTTP version to use for the client request.
     *
     * If the client is using HTTP/1.0 in a request which will send a
     * request message body (such as a POST or PUT), then the client MUST set
     * the "Content-Length" HTTP header to be the length of the message. If
     * this attribute is not present, then the default of
     * GLOBUS_XIO_HTTP_VERSION_1_1 will be used.
     *
     * This attribute is ignored when opening the server side of an HTTP
     * connection.
     */
    /* globus_xio_http_version_t version */
    GLOBUS_XIO_HTTP_ATTR_SET_REQUEST_HTTP_VERSION,
    /** GlobusVarArgEnum(attr)
     * Set the value of an HTTP request header. 
     * @ingroup globus_xio_http_driver_cntls
     *
     * @param header_name
     *        Name of the HTTP header to set.
     * @param header_value
     *        Value of the HTTP header
     *
     * Certain headers will cause the HTTP driver to behave differently than
     * normal. This must be called before
     *
     * - Transfer-Encoding: {identity|chunked}
     *   Override the default transfer encoding. If a server knows the
     *   exact length of the message body, or does not intend to support
     *   persistent connections, it may set this header to be
     *   "identity".<br><br>
     *   If this is set to "identity" and any of the following are true, then
     *   the connection will be closed after the end of the message is sent:
     *   <br><br>
     *     - A Content-Length header is not present
     *     - The HTTP version is set to "HTTP/1.0"
     *     - The Connection header is set to "close"
     *   Attempts to set this to "chunked" with an "HTTP/1.0" client will
     *   fail with a GLOBUS_XIO_ERROR_HTTP_INVALID_HEADER error.
     * - Content-Length: 1*Digit
     *   - Provide a content length for the response message. If the 
     *     "chunked" transfer encoding is being used, then this header
     *     will be silently ignored by the HTTP driver.
     * - Connection: close
     *   - If present in the server response, the connection
     *     will be closed after the end of the data response is written.
     *     Otherwise, when persistent connections are enabled, the connection
     *     <em>may</em> be left open by the driver. Persistent connections
     *     are not yet implemented.
     */
    /* const char *                     header_name,
     * const char *                     header_value */
    GLOBUS_XIO_HTTP_ATTR_SET_REQUEST_HEADER,
    /** GlobusVarArgEnum(attr)
     * Delay writing HTTP request until first data write.
     *
     * If this attribute is present when opening an HTTP handle, the HTTP
     * request will not be sent immediately upon opening the handle. Instead,
     * it will be delayed until the first data write is done. This allows
     * other HTTP headers to be sent after the handle is opened.
     * 
     * This attribute cntl takes no arguments.
     */
    GLOBUS_XIO_HTTP_ATTR_DELAY_WRITE_HEADER,
    /** GlobusVarArgEnum(dd)
     * Get HTTP Request Information.
     *
     * Returns in the passed parameters values concerning the HTTP request. Any
     * of the parameters may be NULL if the application is not interested in
     * that part of the information.
     *
     * @param method
     *      Pointer to be set to the HTTP request method (typically
     *      GET, PUT, or POST). The caller must not access this value
     *      outside of the lifetime of the data descriptor nor free it.
     * @param uri
     *      Pointer to be set to the requested HTTP path. The caller must
     *      not access this value outside of the lifetime of the data
     *      descriptor nor free it.
     * @param http_version
     *      Pointer to be set to the HTTP version used for this request.
     * @param headers
     *      Pointer to be set to point to a hashtable of
     *      globus_xio_http_header_t values, keyed by the HTTP header names.
     *      The caller must not access this value outside of the lifetime of
     *      the data descriptor nor free it or any values in it.
     */
    /* char **                          method,
       char **                          uri,
       globus_xio_http_version_t *      http_version,
       globus_hashtable_t *             headers */
    GLOBUS_XIO_HTTP_GET_REQUEST,
    /** GlobusVarArgEnum(dd)
     * Get HTTP Response Information
     *
     * Returns in the passed parameters values concerning the HTTP response.
     * Any of the parameters may be NULL if the application is not interested
     * in that part of the information.
     *
     * @param status_code
     *      Pointer to be set to the HTTP response status code (such as 404),
     *      as per RFC 2616. The caller must not access this value outside of
     *      the lifetime of the data descriptor nor free it or any values in
     *      it.
     * @param reason_phrase
     *      Pointer to be set to the HTTP response reason phrase (such as Not
     *      Found). The caller must not access this value outside of
     *      the lifetime of the data descriptor nor free it or any values in
     *      it.
     * @param http_version
     *      Pointer to be set to the HTTP version used for this request.
     * @param headers
     *      Pointer to be set to point to a hashtable of
     *      globus_xio_http_header_t values, keyed by the HTTP header names.
     *      The caller must not access this value outside of the lifetime of
     *      the data descriptor nor free it or any values in it.
     */
    /* int *                            status_code,
       char **                          reason_phrase,
       globus_xio_http_version_t *      http_version,
       globus_hashtable_t *             headers */
    GLOBUS_XIO_HTTP_GET_RESPONSE

}
globus_xio_http_attr_cmd_t;

/**
 * Error types used to generate errors using the globus_error_generic module.
 * @ingroup globus_xio_http_driver_errors
 */
typedef enum
{
    /**
     * An attempt to set a header which is not compatible with the HTTP
     * version being used.
     * @hideinitializer
     */
    GLOBUS_XIO_HTTP_ERROR_INVALID_HEADER,
    /**
     * Error parsing HTTP protocol
     */
    GLOBUS_XIO_HTTP_ERROR_PARSE,
    /**
     * There is no entity body to read or write.
     */
    GLOBUS_XIO_HTTP_ERROR_NO_ENTITY,
    /**
     * Server side fake EOF
     */
    GLOBUS_XIO_HTTP_ERROR_EOF,
    /**
     * Persistent connection dropped by the server.
     */
    GLOBUS_XIO_HTTP_ERROR_PERSISTENT_CONNECTION_DROPPED
}
globus_xio_http_errors_t;

/**
 * @ingroup globus_xio_http_driver
 * Valid HTTP versions, used with the
 * #GLOBUS_XIO_HTTP_ATTR_SET_REQUEST_HTTP_VERSION attribute and the
 * #GLOBUS_XIO_HTTP_HANDLE_SET_RESPONSE_HTTP_VERSION handle control
 */
typedef enum
{
    GLOBUS_XIO_HTTP_VERSION_UNSET,
    /**
     * HTTP/1.0
     */
    GLOBUS_XIO_HTTP_VERSION_1_0,
    /**
     * HTTP/1.1
     */
    GLOBUS_XIO_HTTP_VERSION_1_1
}
globus_xio_http_version_t;

#ifdef __cplusplus
}
#endif

#endif
