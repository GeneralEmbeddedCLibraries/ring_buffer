// Copyright (c) 2022 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      ring_buffer.h
*@brief     Ring (circular) buffer for general use
*@author    Ziga Miklosic
*@date      03.02.2021
*@version   V2.1.0
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
#include <string.h>

/**
 * 	@note	For float32_t definition!
 */
#include "project_config.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 * 	Module version
 */
#define RING_BUFFER_VER_MAJOR		( 2 )
#define RING_BUFFER_VER_MINOR		( 1 )
#define RING_BUFFER_VER_DEVELOP		( 0 )

/**
 * 	Status
 */
typedef enum
{
	eRING_BUFFER_OK 		= 0,	/**<Normal operation */

	eRING_BUFFER_ERROR		= 0x01,	/**<General error */
	eRING_BUFFER_ERROR_INIT = 0x02,	/**<Initialization error */
	eRING_BUFFER_ERROR_MEM	= 0x04,	/**<Memory allocation error */
	eRING_BUFFER_ERROR_INST	= 0x08,	/**<Buffer instance missing */
	
	eRING_BUFFER_FULL		= 0x10,	/**<Buffer full */
	eRING_BUFFER_EMPTY		= 0x20,	/**<Buffer empty */
} ring_buffer_status_t;

/**
 *	Attributes 
 */
typedef struct 
{
	const char * 	name;		/**<Name of ring buffer for debugging purposes. Default: NULL */
	void * 			p_mem;		/**<Used buffer memory for static allocation, NULL for dynamic allocation. Default: NULL */	
	uint32_t		item_size;	/**<Size in bytes of individual item in buffer. Default: 1 */
	bool			override;	/**<Override buffer content when full. Default: false */
} ring_buffer_attr_t;

/**
 * 	Pointer to ring buffer instance
 */
typedef struct ring_buffer_s * p_ring_buffer_t;

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t 	ring_buffer_init			(p_ring_buffer_t * p_ring_buffer, const uint32_t size, const ring_buffer_attr_t * const p_attr);
ring_buffer_status_t	ring_buffer_is_init			(p_ring_buffer_t buf_inst, bool * const p_is_init);

ring_buffer_status_t	ring_buffer_add 			(p_ring_buffer_t buf_inst, const void * const p_item);
ring_buffer_status_t	ring_buffer_get 			(p_ring_buffer_t buf_inst, void * const p_item);
ring_buffer_status_t	ring_buffer_get_by_index	(p_ring_buffer_t buf_inst, void * const p_item, const int32_t idx);
ring_buffer_status_t	ring_buffer_reset			(p_ring_buffer_t buf_inst);

ring_buffer_status_t	ring_buffer_get_name		(p_ring_buffer_t buf_inst, char * const p_name);
ring_buffer_status_t	ring_buffer_get_taken		(p_ring_buffer_t buf_inst, uint32_t * const p_taken);
ring_buffer_status_t	ring_buffer_get_free		(p_ring_buffer_t buf_inst, uint32_t * const p_free);
ring_buffer_status_t	ring_buffer_get_size		(p_ring_buffer_t buf_inst, uint32_t * const p_size);
ring_buffer_status_t	ring_buffer_get_item_size	(p_ring_buffer_t buf_inst, uint32_t * const p_item_size);

#endif // __RING_BUFFER_H

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
