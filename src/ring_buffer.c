// Copyright (c) 2023 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      ring_buffer.c
*@brief     Ring (circular) buffer for general use
*@author    Ziga Miklosic
*@date      22.08.2023
*@version   V2.1.0
*
*@section Description
*
*    This module constains ring buffer implementation for general purpose usage.
*    It can work with simple byte size item or larger size items. Module is
*    written in such way that all details are hidden from user. Additionally
*    buffers are created as individual, separated instances so different
*     instances of buffer can be re-configured slitly different.
*
*    Override mode is supported where buffer is never full and new values are
*    always overriding old values regarding of reading rate. This functionality
*    is very usefull for filter sampling storage purposes.
*
*    Additionally buffers data storage can be allocated statically if dynamic
*    allocation is not perfered by application. Look at the example of
*    static allocation of memory.
*
*    There are two distinct get functions: "ring_buffer_get" and "ring_buffer_get_by_index".
*    First one returns oldest item in buffer and acts as a FIFO, meaning that tail increments
*    at every call of it. On the other side "ring_buffer_get_by_index" returns value relative
*    to input argument value and does not increment tail pointer! It is important not to
*    use those two get functionalities simultaniously.
*
*    Function "ring_buffer_get_by_index" supports two kind of access types:
*
*        1. NORMAL ACCESS:     classical aproach, where index is a positive
*                            number and simple represants buffer index. This approach
*                            has no information about time stamps of values inside buffer.
*                            Range: [0, size)
*
*        2. INVERS ACCESS:     chronologically aproach, where index is a negative number.
*                            Meaning that "-1" value will always returns latest value in
*                            buffer and "-size" index value will return oldest value
*                            in buffer. This feature becomes very handy when performing
*                            digital filtering where ring buffer can represants sample
*                            window and thus easy access from oldest to latest sample
*                            can be achieved with invers access.
*                            Range of index: [-size, -1]
*
*@section Code_example
*@code
*
*    // My ring buffer instance
*    p_ring_buffer_t         my_ringbuffer = NULL;
*
*    // Initialization as default buffer with size of 10 items + Dynamica allocation of memory
*    if ( eRING_BUFFER_OK != ring_buffer_init( &my_ringbuffer, 10, NULL ))
*    {
*        // Init failed...
*    }
*
*
*    // My ring buffer instance
*    p_ring_buffer_t         my_ringbuffer_2 = NULL;
*    ring_buffer_attr_t        my_ringbuffer_2_attr;
*
*    // Customize ring buffer:
*    my_ring_buffer_2_attr.name         = "Dynamic allocated buffer";
*    my_ring_buffer_2_attr.p_mem     = NULL;
*    my_ring_buffer_2_attr.item_size = sizeof(float32_t);
*    my_ring_buffer_2_attr.override     = true;
*
*    // Initialization as customized buffer with size of 32 items + Dynamic allocation of memory
*    if ( eRING_BUFFER_OK != ring_buffer_init( &my_ringbuffer_2, 32, &my_ring_buffer_2_attr ))
*    {
*        // Init failed...
*    }
*
*
*    // My ring buffer instance
*    p_ring_buffer_t         my_ringbuffer_3 = NULL;
*    ring_buffer_attr_t        my_ringbuffer_3_attr;
*    uint8_t buf_mem[128];
*
*    // Customize ring buffer:
*    my_ring_buffer_3_attr.name         = "Static allocated buffer";
*    my_ring_buffer_3_attr.p_mem        = &buf_mem;
*    my_ring_buffer_3_attr.item_size = sizeof(float32_t);
*    my_ring_buffer_3_attr.override     = true;
*
*    // Initialization as customised buffer with size of 32 items + Static allocation of memory
*    if ( eRING_BUFFER_OK != ring_buffer_init( &my_ringbuffer_2, 32, &my_ring_buffer_2_attr ))
*    {
*        // Init failed...
*    }
*
*
*
*    // Pump all items out of buffer
*    ring_buffer_get_taken( my_ring_buffer, &taken );
*
*    for ( i = 0; i < taken; i++ )
*    {
*        ring_buffer_get( my_ring_buffer, &item );
*    }
*
*    // OR equivalent
*
*    while( eRING_BUFFER_EMPTY != ring_buffer_get( my_ring_buffer, &item ));
*
*
*
*    // Get value at index 0 from ring buffer - classic access
*    ring_buffer_get_by_index( my_ringbuffer, 0 );
*
*    // Get latest value from ring buffer - inverted access
*    ring_buffer_get_by_index( my_ringbuffer, -1 );
*
*    // Get oldest value from ring buffer - inverted access
*    ring_buffer_get_by_index( my_ringbuffer, -10 );
*
*
*@endcode
*
*/
////////////////////////////////////////////////////////////////////////////////
/*!
* @addtogroup RING_BUFFER
* @{ <!-- BEGIN GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "ring_buffer.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 *     Ring buffer
 */
typedef struct ring_buffer_s
{
    uint8_t *    p_data;            /**<Data in buffer */
    uint32_t     head;              /**<Pointer to head of buffer */
    uint32_t     tail;              /**<Pointer to tail of buffer */
    uint32_t     size_of_buffer;    /**<Size of buffer in bytes */
    uint32_t     size_of_item;      /**<Size of item in bytes */
    const char * name;              /**<Name of buffer */
    bool         override;          /**<Override option */
    bool         is_init;           /**<Ring buffer initialization success flag */
    bool         is_full;           /**<Ring buffer completely full */
    bool         is_empty;          /**<Ring buffer completely empty */
} ring_buffer_t;

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////
static inline ring_buffer_status_t 	ring_buffer_default_setup    	(p_ring_buffer_t ring_buffer, const uint32_t size);
static inline ring_buffer_status_t 	ring_buffer_custom_setup     	(p_ring_buffer_t ring_buffer, const uint32_t size, const ring_buffer_attr_t * const p_attr);
static inline ring_buffer_status_t 	ring_buffer_clear_mem        	(p_ring_buffer_t buf_inst);
static inline uint32_t  			ring_buffer_wrap_index          (const uint32_t idx, const uint32_t size);
static inline uint32_t  			ring_buffer_increment_index     (const uint32_t idx, const uint32_t size, const uint32_t inc);
static inline uint32_t  			ring_buffer_parse_index         (const int32_t idx_req, const uint32_t idx_cur, const uint32_t size);
static inline bool      			ring_buffer_check_index         (const int32_t idx_req, const uint32_t size);
static inline void      			ring_buffer_add_single_to_buf   (p_ring_buffer_t buf_inst, const void * const p_item);
static inline void      			ring_buffer_add_many_to_buf     (p_ring_buffer_t buf_inst, const void * const p_item, const uint32_t size);
static inline void      			ring_buffer_get_many_from_buf   (p_ring_buffer_t buf_inst, void * const p_item, const uint32_t size);
static inline void      			ring_buffer_memcpy              (uint8_t * p_dst, const uint8_t * p_src, const uint32_t size);

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Default setup
*
* @note            Default initialization of ring buffer:
*                    - dynamicall alocation
*                    - size of element = 1
*                    - name = NULL
*
* @param[out]   ring_buffer - Pointer to ring buffer instance
* @param[in]    size        - Size of buffer
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static inline ring_buffer_status_t ring_buffer_default_setup(p_ring_buffer_t ring_buffer, const uint32_t size)
{
    ring_buffer_status_t status = eRING_BUFFER_OK;

    // Default item size
    ring_buffer->size_of_item = 1;

    // Allocate memory
    ring_buffer->p_data = malloc( size );

    // Allocation success
    if ( NULL != ring_buffer->p_data )
    {
        // Clear buffer data
        status = ring_buffer_clear_mem( ring_buffer );
    }
    else
    {
        status = eRING_BUFFER_ERROR_MEM;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Customised setup
*
* @note            User dependent initialization of ring buffer:
*                    - dynamicall or statuc alocation
*                    - size of element = custom
*                    - name = custom
*
* @param[out]   ring_buffer - Pointer to ring buffer instance
* @param[in]    size        - Size of buffer
* @param[in]    p_attr      - Pointer to buffer attributes
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static inline ring_buffer_status_t ring_buffer_custom_setup(p_ring_buffer_t ring_buffer, const uint32_t size, const ring_buffer_attr_t * const p_attr)
{
    ring_buffer_status_t status = eRING_BUFFER_OK;

    // Store attributes
    ring_buffer->name = p_attr->name;
    ring_buffer->size_of_item = p_attr->item_size;
    ring_buffer->override = p_attr->override;

    // Static allocation
    if ( NULL != p_attr->p_mem )
    {
        ring_buffer->p_data = p_attr->p_mem;
    }
    else
    {
        // Allocate memory
        ring_buffer->p_data = malloc( size * p_attr->item_size );

        // Allocation success
        if ( NULL != ring_buffer->p_data )
        {
            // Clear buffer data
            status = ring_buffer_clear_mem( ring_buffer );
        }
        else
        {
            status = eRING_BUFFER_ERROR_MEM;
        }
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Clear buffer memory space
*
*            Function will fill zeros to memory space of buffer
*
* @param[in]    buf_inst    - Pointer to ring buffer instance
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static inline ring_buffer_status_t ring_buffer_clear_mem(p_ring_buffer_t buf_inst)
{
    ring_buffer_status_t     status         = eRING_BUFFER_OK;
    uint32_t                 size_of_mem = 0UL;

    // Calculate memory size
    size_of_mem = ( buf_inst->size_of_buffer * buf_inst->size_of_item );

    // Clear memory
    memset( buf_inst->p_data, 0, size_of_mem );

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Wrap buffer index to [0, buffer_size)
*
* @param[in]    idx         - Index to wrap
* @param[in]    size        - Size of buffer
* @return       idx_wrap    - Wrapped index
*/
////////////////////////////////////////////////////////////////////////////////
static inline uint32_t ring_buffer_wrap_index(const uint32_t idx, const uint32_t size)
{
    uint32_t idx_wrap = 0;

    // Wrap to size of buffer
    if ( idx > ( size - 1U ))
    {
        idx_wrap = ( idx - size );
    }
    else
    {
        idx_wrap = idx;
    }

    return idx_wrap;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Increment buffer index and take care of wrapping.
*
* @param[in]    idx     - Current index
* @param[in]    size    - Size of buffer
* @param[in]    inc     - Increment value
* @return       new_idx - Incremented index
*/
////////////////////////////////////////////////////////////////////////////////
static inline uint32_t ring_buffer_increment_index(const uint32_t idx, const uint32_t size, const uint32_t inc)
{
    uint32_t new_idx = 0U;

    // Increment & wrap to size
    new_idx = idx + inc;
    new_idx = ring_buffer_wrap_index( new_idx, size );

    return new_idx;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Parse requested access index for ring buffer
*
* @note Two kind of access are supported with ring buffers:
*
*         1. Normal access (idx is positive number):
*             This access is classical, which return actual value of ring buffer
*             at requested index.
*
*         2. Invers access (idx is negative number):
*             This access logic takes into account time stamp of each value, so
*             it returns data chronologically. E.g. "-1" always return latest data
*             and "-size" index always returns oldest data.
*
* @param[in]    idx_req     - Requested index, can be negative
* @param[in]    idx_cur     - Current index pointer by buffer instance
* @param[in]    size        - Size of buffer
* @return       buf_idx     - Calculated buffer index
*/
////////////////////////////////////////////////////////////////////////////////
static inline uint32_t ring_buffer_parse_index(const int32_t idx_req, const uint32_t idx_cur, const uint32_t size)
{
    uint32_t buf_idx = 0;

    // Normal access
    if ( idx_req >= 0 )
    {
        buf_idx = idx_req;
    }

    // Invers access
    else
    {
        buf_idx = (( size + idx_req ) + idx_cur );
    }

    // Wrap
    buf_idx = ring_buffer_wrap_index( buf_idx, size );

    return buf_idx;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Check requested buffer index is within
*                 range of:
*
*                       [-buf_size, buf_size)
*
* @param[in]    idx_req - Requested index, can be negative
* @param[in]    size    - Size of buffer
* @return       valid   - Validation flag, true if within range
*/
////////////////////////////////////////////////////////////////////////////////
static inline bool ring_buffer_check_index(const int32_t idx_req, const uint32_t size)
{
    bool valid = false;

    //         Positive + less than size
    //    OR    Negative + less/equal as size
    if     (    (( idx_req >= 0 ) && ( idx_req < (int32_t) size ))
        ||    (( idx_req < 0 ) && ( abs(idx_req) <= size )))
    {
        valid = true;
    }

    return valid;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Add single item to buffer
*
* @param[in]    buf_inst    - Buffer instance
* @param[in]    p_item      - Pointer to item to put into buffer
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static inline void ring_buffer_add_single_to_buf(p_ring_buffer_t buf_inst, const void * const p_item)
{
    // Add new item to buffer
    ring_buffer_memcpy((uint8_t*) &buf_inst->p_data[ (buf_inst->head * buf_inst->size_of_item) ], (uint8_t*) p_item, buf_inst->size_of_item );

    // Increment head
    buf_inst->head = ring_buffer_increment_index( buf_inst->head, buf_inst->size_of_buffer, 1U );
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Add many items to buffer
*
* @param[in]    buf_inst    - Buffer instance
* @param[in]    p_item      - Pointer to item to put into buffer
* @param[in]    size        - Number of items to put into buffer
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static inline void ring_buffer_add_many_to_buf(p_ring_buffer_t buf_inst, const void * const p_item, const uint32_t size)
{
    // Calculate item size till end of buffer
    const uint32_t items_till_end = ( buf_inst->size_of_buffer - buf_inst->head );

    // Request to add more items that there is space till the end of buffer
    if ( size > items_till_end )
    {
        // Calculate size items till end of buffer in bytes
        const uint32_t sizeof_items_till_end = ( buf_inst->size_of_item * items_till_end );

        // Calculate size of items from start of buffer in bytes
        const uint32_t sizeof_items_from_start = (( size - items_till_end ) * buf_inst->size_of_item );

        // Add first items to end of buffer
        ring_buffer_memcpy((uint8_t*) &buf_inst->p_data[ (buf_inst->head * buf_inst->size_of_item) ], (uint8_t*) p_item, sizeof_items_till_end );

        // And then from start of buffer
        ring_buffer_memcpy((uint8_t*) &buf_inst->p_data[0], (uint8_t*) (p_item+sizeof_items_till_end), sizeof_items_from_start );
    }

    // Enough space till end of buffer, no need to wrap
    else
    {
        ring_buffer_memcpy((uint8_t*) &buf_inst->p_data[ (buf_inst->head * buf_inst->size_of_item) ], (uint8_t*) p_item, ( buf_inst->size_of_item * size ));
    }

    // Increment head
    buf_inst->head = ring_buffer_increment_index( buf_inst->head, buf_inst->size_of_buffer, size );
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Get many items from buffer
*
* @param[in]    buf_inst    - Buffer instance
* @param[in]    p_item      - Pointer to item to get from buffer
* @param[in]    size        - Number of items to get from buffer
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static inline void ring_buffer_get_many_from_buf(p_ring_buffer_t buf_inst, void * const p_item, const uint32_t size)
{
    // Calculate item size till end of buffer
    const uint32_t items_till_end = ( buf_inst->size_of_buffer - buf_inst->tail );

    // Request to add more items that there is space till the end of buffer
    if ( size > items_till_end )
    {
        // Calculate size items till end of buffer in bytes
        const uint32_t sizeof_items_till_end = ( buf_inst->size_of_item * items_till_end );

        // Calculate size of items from start of buffer in bytes
        const uint32_t sizeof_items_from_start = (( size - items_till_end ) * buf_inst->size_of_item );

        // Add first items to end of buffer
        ring_buffer_memcpy((uint8_t*) p_item, (uint8_t*) &buf_inst->p_data[ (buf_inst->tail * buf_inst->size_of_item) ], sizeof_items_till_end );

        // And then from start of buffer
        ring_buffer_memcpy((uint8_t*) (p_item + sizeof_items_till_end), (uint8_t*) &buf_inst->p_data[0], sizeof_items_from_start );
    }
    else
    {
        ring_buffer_memcpy((uint8_t*) p_item, (uint8_t*) &buf_inst->p_data[ (buf_inst->tail * buf_inst->size_of_item) ], ( buf_inst->size_of_item * size ));
    }

    // Increment tail due to lost of data
    buf_inst->tail = ring_buffer_increment_index( buf_inst->tail, buf_inst->size_of_buffer, size );
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Ring buffer custom "memcpy" implementation
*
* @note     Measurement on ARM Cortex-M4 with "Ofast" optimization using "arm-none-eabi-gcc"
*           yield 69% reduction of execution time when using custom "memcpy" implementation
*           rather than function from standard library!
*
* @param[in]    p_dst - Pointer to destination memory
* @param[in]    p_src - Pointer to source memory
* @param[in]    size  - Number of bytes to copy
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static inline void ring_buffer_memcpy(uint8_t * p_dst, const uint8_t * p_src, const uint32_t size)
{
    for (uint32_t offset = 0U; offset < size; offset++)
    {
        *( p_dst + offset ) = *( p_src + offset );
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup RING_BUFFER_API
* @{ <!-- BEGIN GROUP -->
*
*     Following function are part or ring buffer API.
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Initialize ring buffer instance
*
* @param[out]   p_ring_buffer   - Pointer to ring buffer instance
* @param[in]    size            - Size of ring buffer
* @param[in]    p_attr          - Pointer to buffer attributes
* @return       status          - Either OK or Error
*/
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_init(p_ring_buffer_t * p_ring_buffer, const uint32_t size, const ring_buffer_attr_t * const p_attr)
{
    ring_buffer_status_t status = eRING_BUFFER_OK;

    if ( NULL != p_ring_buffer )
    {
        // Check if that instance is already allocated
        // By meaning that this buffer instance was initialised before...
        if ( NULL == *p_ring_buffer )
        {
            // Allocate ring buffer instance space
            *p_ring_buffer = malloc( sizeof( ring_buffer_t ));

            // Allocation success
            if ( NULL != *p_ring_buffer )
            {
                (*p_ring_buffer)->size_of_buffer = size;
                (*p_ring_buffer)->head = 0;
                (*p_ring_buffer)->tail = 0;
                (*p_ring_buffer)->is_full = false;
                (*p_ring_buffer)->is_empty = true;

                // Default setup
                if ( NULL == p_attr )
                {
                    status = ring_buffer_default_setup( *p_ring_buffer, size );
                }

                // Customize setup
                else
                {
                    status = ring_buffer_custom_setup( *p_ring_buffer, size, p_attr );
                }

                // Setup success
                if ( eRING_BUFFER_OK == status )
                {
                    (*p_ring_buffer)->is_init = true;
                }
            }
            else
            {
                status = eRING_BUFFER_ERROR_MEM;
            }
        }

        // Already initialised
        else
        {
            status = eRING_BUFFER_ERROR_INIT;
        }
    }
    else
    {
        status = eRING_BUFFER_ERROR_INST;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Get initialization success flag
*
* @param[in]    buf_inst    - Buffer instance
* @param[out]   p_is_init   - Pointer to initialization flag
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_is_init(p_ring_buffer_t buf_inst, bool * const p_is_init)
{
    ring_buffer_status_t status = eRING_BUFFER_OK;

    if ( NULL != buf_inst )
    {
        if ( NULL != p_is_init )
        {
            *p_is_init = buf_inst->is_init;
        }
    }
    else
    {
        status = eRING_BUFFER_ERROR_INST;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Add item to ring buffer
*
* @pre        Buffer instance must be initialized before calling that function!
*
* @note        Function will return OK status if item can be put to buffer. In case
*            that buffer is full it will return "eRING_BUFFER_FULL" return code.
*
* @note        Based on buffer attribute settings "override" has direct impact on
*            function flow!
*
* @param[in]    buf_inst    - Buffer instance
* @param[in]    p_item      - Pointer to item to put into buffer
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_add(p_ring_buffer_t buf_inst, const void * const p_item)
{
    ring_buffer_status_t status = eRING_BUFFER_OK;

    if ( NULL != buf_inst )
    {
        if ( true == buf_inst->is_init )
        {
            if ( NULL != p_item )
            {
                // Buffer full
                if     (    ( buf_inst->head == buf_inst->tail )
                    &&    ( true == buf_inst->is_full ))
                {
                    // Override enabled - buffer never full
                    if ( true == buf_inst->override )
                    {
                        // Add single item to buffer
                        ring_buffer_add_single_to_buf( buf_inst, p_item );

                        // Push tail forward
                        buf_inst->tail = ring_buffer_increment_index( buf_inst->tail, buf_inst->size_of_buffer, 1U );
                    }

                    // Buffer full
                    else
                    {
                        status = eRING_BUFFER_FULL;
                    }
                }

                // Buffer not full
                else
                {
                    // Add single item to buffer
                    ring_buffer_add_single_to_buf( buf_inst, p_item );

                    // Buffer no longer empty
                    buf_inst->is_empty = false;

                    // Is buffer full
                    if ( buf_inst->head == buf_inst->tail )
                    {
                        buf_inst->is_full = true;
                    }
                    else
                    {
                        buf_inst->is_full = false;
                    }
                }
            }
            else
            {
                status = eRING_BUFFER_ERROR;
            }
        }
        else
        {
            status = eRING_BUFFER_ERROR_INIT;
        }
    }
    else
    {
        status = eRING_BUFFER_ERROR_INST;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Add multiple items to ring buffer
*
* @pre      Buffer instance must be initialized before calling that function!
*
* @note     Function will return OK status if items can be put to buffer. In case
*           that buffer is full it will return "eRING_BUFFER_FULL" return code.
*
* @note     In case there is no space for all items to put into buffer it will
*           ignore request and return "eRING_BUFFER_ERROR"!
*
* @note     Based on buffer attribute settings "override" has direct impact on
*           function flow!
*
* @param[in]    buf_inst    - Buffer instance
* @param[in]    p_item      - Pointer to item to put into buffer
* @param[in]    size        - Number of items to get from buffer
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_add_multi(p_ring_buffer_t buf_inst, const void * const p_item, const uint32_t size)
{
    ring_buffer_status_t    status      = eRING_BUFFER_OK;
    uint32_t                free_slots  = 0U;

    if ( NULL != buf_inst )
    {
        if ( true == buf_inst->is_init )
        {
            if ( NULL != p_item )
            {
                // Get number free slots
                (void) ring_buffer_get_free( buf_inst, &free_slots );

                // There is space in buffer
                if  ( size <= free_slots )
                {
                    // Add data to buffer
                    ring_buffer_add_many_to_buf( buf_inst, p_item, size );

                    // Buffer no longer empty
                    buf_inst->is_empty = false;

                    // Is buffer full
                    if ( buf_inst->head == buf_inst->tail )
                    {
                        buf_inst->is_full = true;
                    }
                    else
                    {
                        buf_inst->is_full = false;
                    }
                }

                // No space for all items in buffer
                else
                {
                    // Override enabled - buffer never full
                    if ( true == buf_inst->override )
                    {
                        // Add data to buffer
                        ring_buffer_add_many_to_buf( buf_inst, p_item, size );

                        // Push tail forward
                        buf_inst->tail = ring_buffer_increment_index( buf_inst->tail, buf_inst->size_of_buffer, size );
                    }

                    // Buffer full
                    else
                    {
                        status = eRING_BUFFER_ERROR;
                    }
                }
            }
            else
            {
                status = eRING_BUFFER_ERROR;
            }
        }
        else
        {
            status = eRING_BUFFER_ERROR_INIT;
        }
    }
    else
    {
        status = eRING_BUFFER_ERROR_INST;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Get first item from ring buffer
*
* @pre        Buffer instance must be initialized before calling that function!
*
* @note        Function will return "eRING_BUFFER_OK" status if item can be acquired from buffer. In case
*            that buffer is empty it will return "eRING_BUFFER_EMPTY" code.
*
*        !!! If function do not return "eRING_BUFFER_OK" ignore returned data !!!
*
*            This function gets last item from buffer and increment tail
*            pointer.
*
* @param[in]    buf_inst    - Buffer instance
* @param[out]   p_item      - Pointer to item to put into buffer
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_get(p_ring_buffer_t buf_inst, void * const p_item)
{
    ring_buffer_status_t status = eRING_BUFFER_OK;

    if ( NULL != buf_inst )
    {
        if ( true == buf_inst->is_init )
        {
            if ( NULL != p_item )
            {
                if     (    ( buf_inst->tail == buf_inst->head )
                    &&    ( false == buf_inst->is_full ))
                {
                    status = eRING_BUFFER_EMPTY;
                }
                else
                {
                    // Get item
                    ring_buffer_memcpy((uint8_t*) p_item, (uint8_t*) &buf_inst->p_data[ (buf_inst->tail * buf_inst->size_of_item) ], buf_inst->size_of_item );

                    // Increment tail due to lost of data
                    buf_inst->tail = ring_buffer_increment_index( buf_inst->tail, buf_inst->size_of_buffer, 1U );

                    // Buffer no longer full
                    buf_inst->is_full = false;

                    // Is buffer empty
                    if ( buf_inst->tail == buf_inst->head )
                    {
                        buf_inst->is_empty = true;
                    }
                    else
                    {
                        buf_inst->is_empty = false;
                    }
                }
            }
            else
            {
                status = eRING_BUFFER_ERROR;
            }
        }
        else
        {
            status = eRING_BUFFER_ERROR_INIT;
        }
    }
    else
    {
        status = eRING_BUFFER_ERROR_INST;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Get multiple items from ring buffer
*
* @pre      Buffer instance must be initialized before calling that function!
*
* @note     Function will return "eRING_BUFFER_OK" status if all items can be acquired
*           from buffer. In case that buffer is empty it will return "eRING_BUFFER_EMPTY" code.
*
*           !!! If function do not return "eRING_BUFFER_OK" ignore returned data !!!
*
*           This function gets last items from buffer and increment tail
*           pointer.
*
* @param[in]    buf_inst    - Buffer instance
* @param[out]   p_item      - Pointer to item to put into buffer
* @param[in]    size        - Number of items to get from buffer
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_get_multi(p_ring_buffer_t buf_inst, void * const p_item, const uint32_t size)
{
    ring_buffer_status_t status         = eRING_BUFFER_OK;
    uint32_t             taken_slots    = 0U;

    if ( NULL != buf_inst )
    {
        if ( true == buf_inst->is_init )
        {
            if ( NULL != p_item )
            {
                if  (   ( buf_inst->tail == buf_inst->head )
                     &&  ( false == buf_inst->is_full ))
                 {
                     status = eRING_BUFFER_EMPTY;
                 }
                 else
                 {
                     // Get number taken slots
                     (void) ring_buffer_get_taken( buf_inst, &taken_slots );

                     // Request to take out of buffer valid
                     if ( size <= taken_slots )
                     {
                         // Get data from buffer
                         ring_buffer_get_many_from_buf( buf_inst, p_item, size );

                         // Buffer no longer full
                         buf_inst->is_full = false;

                         // Is buffer empty
                         if ( buf_inst->tail == buf_inst->head )
                         {
                             buf_inst->is_empty = true;
                         }
                         else
                         {
                             buf_inst->is_empty = false;
                         }
                     }
                     else
                     {
                         status = eRING_BUFFER_ERROR;
                     }
                 }
            }
            else
            {
                status = eRING_BUFFER_ERROR;
            }
        }
        else
        {
            status = eRING_BUFFER_ERROR_INIT;
        }
    }
    else
    {
        status = eRING_BUFFER_ERROR_INST;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Get item from ring buffer at the requested index
*
* @note     Index of aquired data must be within range of:
*
*                     -size_of_buffer < idx < ( size_of_buffer - 1 )
*
*
*             This function does not increment buffer tail!
*
* @code
*
*            // EXAMPLE OF RING BUFFER ACCESS
*            // NOTE: Size of buffer in that example is 4
*
*             // LATEST DATA IN BUFFER
*             // Example of getting latest data from ring buffer,
*             // nevertheless of buffer size
*             ring_buffer_get_by_index( buf_inst, -1 );
*
*             // OR equivalent
*             ring_buffer_get_by_index( buf_inst, 3 );
*
*            // OLDEST DATA IN BUFFER
*            ring_buffer_get_by_index( buf_inst, 0 );
*
*            // OR equivivalent
*            ring_buffer_get_by_index( buf_inst, -4 );
*
*
* @endcode
*
* @param[in]    buf_inst    - Buffer instance
* @param[out]   p_item      - Pointer to item to put into buffer
* @param[in]    idx         - Index of wanted data
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_get_by_index(p_ring_buffer_t buf_inst, void * const p_item, const int32_t idx)
{
    ring_buffer_status_t     status     = eRING_BUFFER_OK;
    uint32_t                buf_idx = 0UL;

    if ( NULL != buf_inst )
    {
        if ( true == buf_inst->is_init )
        {
            // Check validy of requestd idx
            if     (    ( NULL != p_item )
                &&    ( true == ring_buffer_check_index( idx, buf_inst->size_of_buffer )))
            {
                // Get parsed buffer index
                buf_idx = ring_buffer_parse_index( idx, buf_inst->tail, buf_inst->size_of_buffer );

                // Get data
                ring_buffer_memcpy((uint8_t*) p_item, (uint8_t*) &buf_inst->p_data[ (buf_idx * buf_inst->size_of_item) ], buf_inst->size_of_item );
            }
            else
            {
                status = eRING_BUFFER_ERROR;
            }
        }
        else
        {
            status = eRING_BUFFER_ERROR_INIT;
        }
    }
    else
    {
        status = eRING_BUFFER_ERROR_INST;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Reset ring buffer
*
* @pre        Buffer instance must be initialized before calling that function!
*
* @note        This will resets also head & tail pointers!
*
* @param[in]    buf_inst    - Buffer instance
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_reset(p_ring_buffer_t buf_inst)
{
    ring_buffer_status_t status = eRING_BUFFER_OK;

    if ( NULL != buf_inst )
    {
        if ( true == buf_inst->is_init )
        {
            buf_inst->head = 0;
            buf_inst->tail = 0;
            buf_inst->is_full = false;
            buf_inst->is_empty = true;
            status = ring_buffer_clear_mem( buf_inst );
        }
        else
        {
            status = eRING_BUFFER_ERROR_INIT;
        }
    }
    else
    {
        status = eRING_BUFFER_ERROR_INST;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Get ring buffer name
*
* @pre        Buffer instance must be initialized before calling that function!
*
* @param[in]    buf_inst    - Buffer instance
* @param[out]   p_name      - Pointer to buffer name
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_get_name(p_ring_buffer_t buf_inst, char * const p_name)
{
    ring_buffer_status_t status = eRING_BUFFER_OK;

    if ( NULL != buf_inst )
    {
        if ( true == buf_inst->is_init )
        {
            if ( NULL != p_name )
            {
                strcpy( p_name, buf_inst->name );
            }
        }
        else
        {
            status = eRING_BUFFER_ERROR_INIT;
        }
    }
    else
    {
        status = eRING_BUFFER_ERROR_INST;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Get number of taken space for items in buffer
*
* @pre        Buffer instance must be initialized before calling that function!
*
* @param[in]    buf_inst    - Buffer instance
* @param[out]   p_taken     - Pointer to number of taken space
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_get_taken(p_ring_buffer_t buf_inst, uint32_t * const p_taken)
{
    ring_buffer_status_t status = eRING_BUFFER_OK;

    if ( NULL != buf_inst )
    {
        if ( true == buf_inst->is_init )
        {
            if ( NULL != p_taken )
            {
                if ( true == buf_inst->is_empty )
                {
                    *p_taken = 0;
                }
                else if ( true == buf_inst->is_full )
                {
                    *p_taken = buf_inst->size_of_buffer;
                }
                else
                {
                    if ( buf_inst->head > buf_inst->tail )
                    {
                        *p_taken = ( buf_inst->head - buf_inst->tail );
                    }
                    else
                    {
                        *p_taken = ( buf_inst->size_of_buffer - ( buf_inst->tail - buf_inst->head ));
                    }
                }
            }
        }
        else
        {
            status = eRING_BUFFER_ERROR_INIT;
        }
    }
    else
    {
        status = eRING_BUFFER_ERROR_INST;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Get number of free space for items in buffer
*
* @pre        Buffer instance must be initialized before calling that function!
*
* @param[in]    buf_inst    - Buffer instance
* @param[out]   p_free      - Pointer to number of free space
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_get_free(p_ring_buffer_t buf_inst, uint32_t * const p_free)
{
    ring_buffer_status_t     status     = eRING_BUFFER_OK;
    uint32_t                taken     = 0UL;

    if ( NULL != buf_inst )
    {
        if ( true == buf_inst->is_init )
        {
            if ( NULL != p_free )
            {
                // Get free items
                ring_buffer_get_taken( buf_inst, &taken );

                // Calculate taken
                *p_free = ( buf_inst->size_of_buffer - taken );
            }
        }
        else
        {
            status = eRING_BUFFER_ERROR_INIT;
        }
    }
    else
    {
        status = eRING_BUFFER_ERROR_INST;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Get size of all items inside ring buffer 
*
* @pre        Buffer instance must be initialized before calling that function!
*
* @note        Item can be multiple bytes as it can be also large strucure data
*            therefore item size and buffer size differs.
*
* @param[in]    buf_inst    - Buffer instance
* @param[out]   p_size      - Pointer to buffer size
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_get_size(p_ring_buffer_t buf_inst, uint32_t * const p_size)
{
    ring_buffer_status_t status = eRING_BUFFER_OK;

    if ( NULL != buf_inst )
    {
        if ( true == buf_inst->is_init )
        {
            if ( NULL != p_size )
            {
                *p_size = buf_inst->size_of_buffer;
            }
        }
        else
        {
            status = eRING_BUFFER_ERROR_INIT;
        }
    }
    else
    {
        status = eRING_BUFFER_ERROR_INST;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Get ring buffer single item size
*
* @pre        Buffer instance must be initialized before calling that function!
*
* @note        Item can be multiple bytes as it can be also large strucure data
*            therefore item size and buffer size differs.
*
* @param[in]    buf_inst    - Buffer instance
* @param[out]   p_item_size - Pointer buffer item size in bytes
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_get_item_size(p_ring_buffer_t buf_inst, uint32_t * const p_item_size)
{
    ring_buffer_status_t status = eRING_BUFFER_OK;

    if ( NULL != buf_inst )
    {
        if ( true == buf_inst->is_init )
        {
            if ( NULL != p_item_size )
            {
                *p_item_size = buf_inst->size_of_item;
            }
        }
        else
        {
            status = eRING_BUFFER_ERROR_INIT;
        }
    }
    else
    {
        status = eRING_BUFFER_ERROR_INST;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
