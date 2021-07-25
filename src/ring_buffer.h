// Copyright (c) 2021 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      ring_buffer.h
*@brief     Ring (circular) buffer for general use
*@author    Ziga Miklosic
*@date      03.02.2021
*@version   V1.0.1
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
#include <math.h>

#include "project_config.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 * 	Module version
 */
#define RING_BUFFER_VER_MAJOR		( 1 )
#define RING_BUFFER_VER_MINOR		( 0 )
#define RING_BUFFER_VER_DEVELOP		( 1 )

/**
 * 	Status
 */
typedef enum
{
	eRING_BUFFER_OK = 0,	/**<Normal operation */
	eRING_BUFFER_ERROR,		/**<General error */
} ring_buffer_status_t;

/**
 * 	Pointer to ring buffer instance
 */
typedef struct ring_buffer_s * p_ring_buffer_t;

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t 	ring_buffer_init	(p_ring_buffer_t * p_ring_buffer, const uint32_t size);
bool					ring_buffer_is_init	(p_ring_buffer_t buf_inst);
ring_buffer_status_t 	ring_buffer_add_u32	(p_ring_buffer_t buf_inst, const uint32_t data);
ring_buffer_status_t 	ring_buffer_add_i32	(p_ring_buffer_t buf_inst, const int32_t data );
ring_buffer_status_t 	ring_buffer_add_f	(p_ring_buffer_t buf_inst, const float32_t data);
uint32_t 				ring_buffer_get_u32	(p_ring_buffer_t buf_inst, const int32_t idx);
int32_t 				ring_buffer_get_i32	(p_ring_buffer_t buf_inst, const int32_t idx);
float32_t 				ring_buffer_get_f	(p_ring_buffer_t buf_inst, const int32_t idx);

#endif // __RING_BUFFER_H

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
