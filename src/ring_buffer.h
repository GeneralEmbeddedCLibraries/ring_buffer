// Copyright (c) 2025 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      ring_buffer.h
*@brief     Ring (circular) buffer for general use
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@author    Matej Otic
*@email     otic.matej@dancing-bits.com
*@date      28.08.2025
*@version   V3.0.0
*/
////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup RING_BUFFER_API
* @{ <!-- BEGIN GROUP -->
*
*/
////////////////////////////////////////////////////////////////////////////////

#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 *     Module version
 */
#define RING_BUFFER_VER_MAJOR       ( 3 )
#define RING_BUFFER_VER_MINOR       ( 0 )
#define RING_BUFFER_VER_DEVELOP     ( 0 )

/**
 *     Status
 */
typedef enum
{
    eRING_BUFFER_OK         = 0x00U,    /**<Normal operation */

    eRING_BUFFER_ERROR      = 0x01U,    /**<General error */
    eRING_BUFFER_ERROR_INIT = 0x02U,    /**<Initialization error */
    eRING_BUFFER_ERROR_MEM  = 0x04U,    /**<Memory allocation error */
    eRING_BUFFER_ERROR_INST = 0x08U,    /**<Buffer instance missing */
    
    eRING_BUFFER_FULL       = 0x10U,    /**<Buffer full */
    eRING_BUFFER_EMPTY      = 0x20U,    /**<Buffer empty */
} ring_buffer_status_t;

/**
 *    Attributes 
 */
typedef struct 
{
    const char * name;      /**<Name of ring buffer for debugging purposes. Default: NULL */
    void *       p_mem;     /**<Used buffer memory for static allocation, NULL for dynamic allocation. Default: NULL */
    uint32_t     item_size; /**<Size in bytes of individual item in buffer. Default: 1 */
    bool         override;  /**<Override buffer content when full. Default: false */
} ring_buffer_attr_t;

/**
 *     Ring buffer
 */
typedef struct
{
    uint8_t *     p_data;            /**<Data containter (buffer) */
    uint32_t      head;              /**<Pointer to head of buffer */
    uint32_t      tail;              /**<Pointer to tail of buffer */
    uint32_t      size_of_buffer;    /**<Size of buffer in items */
    uint32_t      size_of_item;      /**<Size of item in bytes */
    const char *  name;              /**<Name of buffer */
    bool          override;          /**<Override option */
    bool          is_init;           /**<Ring buffer initialization success flag */
    atomic_size_t count;             /**<Number of elements in buffer */
} ring_buffer_t;
typedef ring_buffer_t * p_ring_buffer_t;

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_init        (p_ring_buffer_t * p_ring_buffer, const uint32_t size, const ring_buffer_attr_t * const p_attr);
ring_buffer_status_t ring_buffer_init_static (p_ring_buffer_t ring_buffer, const uint32_t size, const ring_buffer_attr_t * const p_attr);
bool                 ring_buffer_is_init     (p_ring_buffer_t buf_inst);

ring_buffer_status_t ring_buffer_add         (p_ring_buffer_t buf_inst, const void * const p_item);
ring_buffer_status_t ring_buffer_add_multi   (p_ring_buffer_t buf_inst, const void * const p_item, const uint32_t size);
ring_buffer_status_t ring_buffer_get         (p_ring_buffer_t buf_inst, void * const p_item);
ring_buffer_status_t ring_buffer_get_multi   (p_ring_buffer_t buf_inst, void * const p_item, const uint32_t size);
ring_buffer_status_t ring_buffer_get_by_index(p_ring_buffer_t buf_inst, void * const p_item, const int32_t target_idx);
ring_buffer_status_t ring_buffer_reset       (p_ring_buffer_t buf_inst);

const char * ring_buffer_get_name (p_ring_buffer_t buf_inst);
uint32_t ring_buffer_get_taken    (p_ring_buffer_t buf_inst);
uint32_t ring_buffer_get_free     (p_ring_buffer_t buf_inst);
uint32_t ring_buffer_get_size     (p_ring_buffer_t buf_inst);
uint32_t ring_buffer_get_item_size(p_ring_buffer_t buf_inst);
bool     ring_buffer_is_full      (p_ring_buffer_t buf_inst);
bool     ring_buffer_is_empty     (p_ring_buffer_t buf_inst);

#endif // __RING_BUFFER_H

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
