// Copyright (c) 2021 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      ring_buffer.c
*@brief     Ring (circular) buffer for general use
*@author    Ziga Miklosic
*@date      03.02.2021
*@version   V1.0.1
*
*@section Description
*
*	This module contations ring buffer for general use, where three data types
*	are supported: signed 32-bit integer, unsigned 32-bit integer and
*	floating value. Therefore three kinds of add/get functions are
*	provided.
*
*
*	API of ring buffer simply consist of three functions:
*		1. initialization: 	ring_buffer_init()
*		2. add value:		ring_buffer_add_x()
*		3. get value:		ring_buffer_get_x()
*
*
*	Ring buffer get function support two kind of access types:
*
*		1. NORMAL ACCESS: 	classical aproach, where index is positive a positive
*							number and simple represants buffer index. This approach
*							has no information about time stamps of values inside buffer.
*							Range: [0, size)
*
*		2. INVERS ACCESS: 	chronologically aproach, where index is a negative number.
*							Meaning that "-1" value will always returns latest value in
*							buffer and "-size" index value will return oldest value
*							in buffer. This feature becomes very handy when performing
*							digital filtering where ring buffer can represants sample
*							windown and thus easy access from oldest to latest sample
*							can be achieved with invers access.
*							Range of index: [-size, -1]
*
*@section Code_example
*@code
*
*	// My ring buffer instance
*	p_ring_buffer_t my_ringbuffer = NULL;
*
*	// Initialization
*	if ( eRING_BUFFER_OK != ring_buffer_init( &my_ringbuffer, 10 ))
*	{
*		// Init failed...
*	}
*
*	// Add value to ring buffer
*	ring_buffer_add_u32( my_ringbuffer, u32_value );
*	ring_buffer_add_i32( my_ringbuffer, i32_value );
*	ring_buffer_add_f( my_ringbuffer, f_value );
*
*	// Get value at index 0 from ring buffer - classic access
*	ring_buffer_get( my_ringbuffer, 0 );
*
*	// Get latest value from ring buffer - inverted access
*	ring_buffer_get( my_ringbuffer, -1 );
*
*	// Get oldest value from ring buffer - inverted access
*	ring_buffer_get( my_ringbuffer, -10 );
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
#include "ring_buffer.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 * 	Ring buffer
 */
typedef struct ring_buffer_s
{
	uint8_t * 		p_data;				/**<Data in buffer */
	uint32_t 		head;				/**<Pointer to head of buffer */
	uint32_t 		tail;				/**<Pointer to tail of buffer */
	uint32_t		size_of_buffer;		/**<Size of buffer in bytes */
	uint32_t		size_of_item;		/**<Size of item in bytes */
	const char * 	name;				/**<Name of buffer */
	bool			override;			/**<Override option */
	bool			is_init;			/**<Ring buffer initialization success flag */
	bool			is_full;			/**<Ring buffer completely full */
	bool			is_empty;			/**<Ring buffer completely empty */
} ring_buffer_t;

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////
static ring_buffer_status_t ring_buffer_default_setup	(p_ring_buffer_t ring_buffer, const uint32_t size);
static ring_buffer_status_t ring_buffer_custom_setup	(p_ring_buffer_t ring_buffer, const uint32_t size, const ring_buffer_attr_t * const p_attr);
static ring_buffer_status_t ring_buffer_clear_mem		(p_ring_buffer_t buf_inst);

static uint32_t ring_buffer_wrap_index		(const uint32_t idx, const uint32_t size);
static uint32_t ring_buffer_increment_index	(const uint32_t idx, const uint32_t size);
static uint32_t ring_buffer_decrement_index	(const uint32_t idx, const uint32_t size);
static uint32_t ring_buffer_parse_index		(const int32_t idx_req, const uint32_t idx_cur, const uint32_t size);
static bool 	ring_buffer_check_index		(const int32_t idx_req, const uint32_t size);

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    	Default setup
*
* @note			Default initialization of ring buffer:
*					- dynamicall alocation
*					- size of element = 1
*					- name = NULL
*
* @param[out]  	ring_buffer	- Pointer to ring buffer instance
* @param[in]	size		- Size of buffer
* @return       status		- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static ring_buffer_status_t ring_buffer_default_setup(p_ring_buffer_t ring_buffer, const uint32_t size)
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
* @brief    	Customised setup
*
* @note			User dependent initialization of ring buffer:
*					- dynamicall or statuc alocation  
*					- size of element = custom
*					- name = custom
*
* @param[out]  	ring_buffer	- Pointer to ring buffer instance
* @param[in]	size		- Size of buffer
* @param[in]	p_attr		- Pointer to buffer attributes
* @return       status		- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static ring_buffer_status_t ring_buffer_custom_setup(p_ring_buffer_t ring_buffer, const uint32_t size, const ring_buffer_attr_t * const p_attr)
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
*			Function will fill zeros to memory space of buffer
*
* @param[in]  	buf_inst	- Pointer to ring buffer instance
* @param[out]  	p_item		- Pointer to item to put into buffer
* @return       status 		- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static ring_buffer_status_t ring_buffer_clear_mem(p_ring_buffer_t buf_inst)
{
	ring_buffer_status_t 	status 		= eRING_BUFFER_OK;
	uint32_t 				size_of_mem = 0UL;

	// Calculate memory size
	size_of_mem = ( buf_inst->size_of_buffer * buf_inst->size_of_item );

	// Clear memory
	memset( buf_inst->p_data, 0, size_of_mem );

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    	Wrap buffer index to [0, buffer_size)
*
*
* @param[in]	idx			- Index to wrap
* @param[in]	size		- Size of buffer
* @return       idx_wrap	- Wrapped index
*/
////////////////////////////////////////////////////////////////////////////////
static uint32_t ring_buffer_wrap_index(const uint32_t idx, const uint32_t size)
{
	uint32_t idx_wrap = 0;

	// Wrap to size of buffer
	if ( idx > ( size - 1UL ))
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
* @brief    	Increment buffer index and take care of wrapping.
*
*
* @param[in]	idx			- Current index
* @param[in]	size		- Size of buffer
* @return       new_idx		- Incremented index
*/
////////////////////////////////////////////////////////////////////////////////
static uint32_t ring_buffer_increment_index(const uint32_t idx, const uint32_t size)
{
	uint32_t new_idx = 0UL;

	// Increment & wrap to size
	new_idx = idx + 1UL;
	new_idx = ring_buffer_wrap_index( new_idx, size );

	return new_idx;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    	Increment buffer index and take care of wrapping.
*
*
* @param[in]	idx			- Current index
* @param[in]	size		- Size of buffer
* @return       new_idx		- Incremented index
*/
////////////////////////////////////////////////////////////////////////////////
static uint32_t ring_buffer_decrement_index(const uint32_t idx, const uint32_t size)
{
	uint32_t new_idx = 0UL;

	// Increment & wrap to size
	new_idx = idx - 1UL;
	new_idx = ring_buffer_wrap_index( new_idx, size );

	return new_idx;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    	Parse requested access index for ring buffer
*
* @note Two kind of access are supported with ring buffers:
*
* 		1. Normal access (idx is positive number):
* 			This access is classical, which return actual value of ring buffer
* 			at requested index.
*
* 		2. Invers access (idx is negative number):
* 			This access logic takes into account time stamp of each value, so
* 			it returns data chronologically. E.g. "-1" always return latest data
* 			and "-size" index always returns oldest data.
*
* @param[in]	idx_req		- Requested index, can be negative
* @param[in]	idx_cur		- Current index pointer by buffer instance
* @param[in]	size		- Size of buffer
* @return       buf_idx		- Calculated buffer index
*/
////////////////////////////////////////////////////////////////////////////////
static uint32_t ring_buffer_parse_index(const int32_t idx_req, const uint32_t idx_cur, const uint32_t size)
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
* @brief    	Check requested buffer index is within
* 				range of:
* 								[-buf_size, buf_size)
*
* @param[in]	idx_req		- Requested index, can be negative
* @param[in]	size		- Size of buffer
* @return       valid		- Validation flag, true if within range
*/
////////////////////////////////////////////////////////////////////////////////
static bool ring_buffer_check_index(const int32_t idx_req, const uint32_t size)
{
	bool valid = false;

	// Positive + less than size
	if (( idx_req >= 0 ) && ( idx_req < size ))
	{
		valid = true;
	}

	// Negative + less/equal as size
	else if (( idx_req < 0 ) && ( abs(idx_req) <= size ))
	{
		valid = true;
	}

	// None of the above
	else
	{
		valid = false;
	}

	return valid;
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
* 	Following function are part or ring buffer API.
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Initialize ring buffer instance
*
* @param[out]  	p_ring_buffer	- Pointer to ring buffer instance
* @param[in]  	size			- Size of ring buffer
* @param[in]  	p_attr			- Pointer to buffer attributes
* @return       status			- Either OK or Error
*/
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_init(p_ring_buffer_t * p_ring_buffer, const uint32_t size, const ring_buffer_attr_t * const p_attr)
{
	ring_buffer_status_t status = eRING_BUFFER_OK;

	if ( NULL != p_ring_buffer )
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
			(*p_ring_buffer)->is_empty = false;

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
* @param[in]  	buf_inst	- Pointer to ring buffer instance
* @param[out]  	p_is_init	- Pointer to initialization flag
* @return       status 		- Status of operation
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
* @pre		Buffer instance must be initialized before calling that function!
*
* @note		Function will return OK status if item can be put to buffer. In case
*			that buffer is full it will return BUFFER_FULL return code.
*		
* @note		Based on buffer attribute settings "overide" has direct impact on
*			function flow!
*
* @note		This implementation of buffer will use size-1 space of buffer as
*			head==tail is empty state!
*
* @param[in]  	buf_inst	- Pointer to ring buffer instance
* @param[in]  	p_item		- Pointer to item to put into buffer
* @return       status 		- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_add(p_ring_buffer_t buf_inst, const void * const p_item)
{
	ring_buffer_status_t 	status 		= eRING_BUFFER_OK;
	uint32_t				inter_head	= 0;

	if ( NULL != buf_inst )
	{
		if ( true == buf_inst->is_init )
		{
			if ( NULL != p_item )
			{
				// Buffer full
				if 	(	( buf_inst->head == buf_inst->tail )
					&&	( true == buf_inst->is_full ))
				{
					status = eRING_BUFFER_FULL;
				}

				// Buffer empty
				else
				{
					// Add new item to buffer
					memcpy((void*) &buf_inst->p_data[ (buf_inst->head * buf_inst->size_of_item) ], p_item, buf_inst->size_of_item );
					
					// Increment head
					buf_inst->head = ring_buffer_increment_index( buf_inst->head, buf_inst->size_of_buffer );

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

				/*
				// Any space in buffer
				if ( ring_buffer_wrap_index( buf_inst->head + 1, buf_inst->size_of_buffer ) != buf_inst->tail )
				{
					// Add new item to buffer
					memcpy((void*) &buf_inst->p_data[ (buf_inst->head * buf_inst->size_of_item) ], p_item, buf_inst->size_of_item );
					
					// Increment head
					buf_inst->head = ring_buffer_increment_index( buf_inst->head, buf_inst->size_of_buffer );
				}

				// No more space or at least one
				else
				{
					// Is allow to override
					if ( true == buf_inst->override )
					{
						// Add new item to buffer
						memcpy((void*) &buf_inst->p_data[ (buf_inst->head * buf_inst->size_of_item) ], p_item, buf_inst->size_of_item );

						// Increment head
						buf_inst->head = ring_buffer_increment_index( buf_inst->head, buf_inst->size_of_buffer );

						// Push tail forward due to lost of data
						buf_inst->tail = ring_buffer_increment_index( buf_inst->tail, buf_inst->size_of_buffer );
					}
					else
					{
						status = eRING_BUFFER_FULL;
					}
				}
				*/
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
* @pre		Buffer instance must be initialized before calling that function!
*		
* @note		Function will return OK status if item can be acquired from buffer. In case
*			that buffer is empty it will return BUFFER_EMPTY code.
*		
*			This function gets last item from buffer and increment tail
*			pointer. 
*
* @note		Funtion "get_by_index" does not increment tail pointer as this 
*			function does that!
*
* @param[in]  	buf_inst	- Pointer to ring buffer instance
* @param[out]  	p_item		- Pointer to item to put into buffer
* @return       status 		- Status of operation
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
				if 	(	( buf_inst->tail == buf_inst->head )
					//&&	( true == buf_inst->is_empty ))
					&&	( false == buf_inst->is_full ))
				{
					status = eRING_BUFFER_EMPTY;
				}
				else
				{
					// Get item
					memcpy( p_item, (void*) &buf_inst->p_data[ (buf_inst->tail * buf_inst->size_of_item) ], buf_inst->size_of_item );

					// Increment tail due to lost of data
					buf_inst->tail = ring_buffer_increment_index( buf_inst->tail, buf_inst->size_of_buffer );

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



				/*
				// Buffer empty
				if ( buf_inst->tail == buf_inst->head )
				{
					status = eRING_BUFFER_EMPTY;
				}
				else
				{
					// Get item
					memcpy( p_item, (void*) &buf_inst->p_data[ (buf_inst->tail * buf_inst->size_of_item) ], buf_inst->size_of_item );

					// Increment tail due to lost of data
					buf_inst->tail = ring_buffer_increment_index( buf_inst->tail, buf_inst->size_of_buffer );
				}
				*/
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


// NOTE: Does not increment tail
ring_buffer_status_t ring_buffer_get_by_index(p_ring_buffer_t buf_inst, void * const p_item, const int32_t idx)
{
	ring_buffer_status_t status = eRING_BUFFER_OK;

	if ( NULL != buf_inst )
	{
		if ( true == buf_inst->is_init )
		{
			// TODO:...
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

ring_buffer_status_t ring_buffer_reset(p_ring_buffer_t buf_inst)
{
	ring_buffer_status_t status = eRING_BUFFER_OK;

	if ( NULL != buf_inst )
	{
		if ( true == buf_inst->is_init )
		{
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

ring_buffer_status_t ring_buffer_get_name(p_ring_buffer_t buf_inst, char * const p_name)
{
	ring_buffer_status_t status = eRING_BUFFER_OK;

	if ( NULL != buf_inst )
	{
		if ( true == buf_inst->is_init )
		{
			if ( NULL != p_name )
			{
				strncpy( p_name, buf_inst->name, strlen( buf_inst->name ));
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

ring_buffer_status_t ring_buffer_get_taken(p_ring_buffer_t buf_inst, uint32_t * const p_taken)
{
	ring_buffer_status_t status = eRING_BUFFER_OK;

	if ( NULL != buf_inst )
	{
		if ( true == buf_inst->is_init )
		{
			if ( NULL != p_taken )
			{
				// TODO:
				*p_taken = 0;
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

ring_buffer_status_t ring_buffer_get_free(p_ring_buffer_t buf_inst, uint32_t * const p_free)
{
	ring_buffer_status_t status = eRING_BUFFER_OK;

	if ( NULL != buf_inst )
	{
		if ( true == buf_inst->is_init )
		{
			if ( NULL != p_free )
			{
				// TODO: 
				*p_free = 0;
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


#if 0 // OBSOLETE


////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Initialize ring buffer instance
*
* @param[out]  	p_ring_buffer	- Pointer to ring buffer instance
* @param[in]  	size			- Size of ring buffer
* @return       status			- Either OK or Error
*/
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_init(p_ring_buffer_t * p_ring_buffer, const uint32_t size)
{
	ring_buffer_status_t status = eRING_BUFFER_OK;

	if ( NULL != p_ring_buffer )
	{
		// Allocate ring buffer instance space
		*p_ring_buffer = malloc( sizeof( ring_buffer_t ));

		// Allocation success
		if ( NULL != *p_ring_buffer )
		{
			// Allocate ring buffer data space
			(*p_ring_buffer)->p_data = malloc( size * sizeof( ring_buffer_data_t ));

			// Allocation success
			if ( NULL != ( (*p_ring_buffer)->p_data ))
			{
				// Clear buffer data
				for (uint32_t i = 0; i < size; i++)
				{
					(*p_ring_buffer)->p_data[i].u32 = 0UL;
				}

				// Init index and size
				(*p_ring_buffer)->idx = 0;
				(*p_ring_buffer)->size = size;

				// Init success
				(*p_ring_buffer)->is_init = true;
			}
			else
			{
				status = eRING_BUFFER_ERROR;
			}
		}
		else
		{
			status = eRING_BUFFER_ERROR;
		}
	}
	else
	{
		status = eRING_BUFFER_ERROR;
	}

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Get initialization success flag
*
* @param[in]  	buf_inst	- Pointer to ring buffer instance
* @param[out]  	p_is_init	- Pointer to initialization flag
* @return       status 		- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_is_init(p_ring_buffer_t buf_inst, bool * const p_is_init)
{
	ring_buffer_status_t status = eRING_BUFFER_OK;

	if 	(	( NULL != buf_inst )
		&& 	( NULL != p_is_init ))
	{
		*p_is_init = buf_inst->is_init;
	}

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Add unsigned 32-bit value to ring buffer
*
* @param[out]  	buf_inst	- Pointer to ring buffer instance
* @param[in]  	data		- Unsigned 32-bit data
* @return       status		- Either OK or Error
*/
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_add_u32(p_ring_buffer_t buf_inst, const uint32_t data)
{
	ring_buffer_status_t status = eRING_BUFFER_OK;

	// Check for instance and initialization
	if ( NULL != buf_inst )
	{
		if ( true == buf_inst->is_init )
		{
			// Add new data to buffer
			buf_inst->p_data[ buf_inst->idx ].u32 = data;

			// Increment index
			buf_inst->idx = ring_buffer_increment_index( buf_inst->idx, buf_inst->size );
		}
		else
		{
			status = eRING_BUFFER_ERROR;
		}
	}
	else
	{
		status = eRING_BUFFER_ERROR;
	}

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Add signed 32-bit value to ring buffer
*
* @param[out]  	buf_inst	- Pointer to ring buffer instance
* @param[in]  	data		- Unsigned 32-bit data
* @return       status		- Either OK or Error
*/
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_add_i32(p_ring_buffer_t buf_inst, const int32_t data )
{
	ring_buffer_status_t status = eRING_BUFFER_OK;

	// Check for instance and initialization
	if ( NULL != buf_inst )
	{
		if ( true == buf_inst->is_init )
		{
			// Add new data to buffer
			buf_inst->p_data[ buf_inst->idx ].i32 = data;

			// Increment index
			buf_inst->idx = ring_buffer_increment_index( buf_inst->idx, buf_inst->size );
		}
		else
		{
			status = eRING_BUFFER_ERROR;
		}
	}
	else
	{
		status = eRING_BUFFER_ERROR;
	}

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Add floating value to ring buffer
*
* @param[out]  	buf_inst	- Pointer to ring buffer instance
* @param[in]  	data		- Unsigned 32-bit data
* @return       status		- Either OK or Error
*/
////////////////////////////////////////////////////////////////////////////////
ring_buffer_status_t ring_buffer_add_f(p_ring_buffer_t buf_inst, const float32_t data)
{
	ring_buffer_status_t status = eRING_BUFFER_OK;

	// Check for instance and initialization
	if ( NULL != buf_inst )
	{
		if ( true == buf_inst->is_init )
		{
			// Add new data to buffer
			buf_inst->p_data[ buf_inst->idx ].f = data;

			// Increment index
			buf_inst->idx = ring_buffer_increment_index( buf_inst->idx, buf_inst->size );
		}
		else
		{
			status = eRING_BUFFER_ERROR;
		}
	}
	else
	{
		status = eRING_BUFFER_ERROR;
	}

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    	Get unsigned 32-bit value from ring buffer at requesed index.
*
* @note 	Index of aquired data must be within range of:
*
* 					-size < idx < ( size - 1 )
*
* @code
*
*			// EXAMPLE OF RING BUFFER ACCESS
*			// NOTE: Size of buffer in that example is 4
*
* 			// LATEST DATA IN BUFFER
* 			// Example of getting latest data from ring buffer,
* 			// nevertheless of buffer size
* 			ring_buffer_get_u32( buf_inst, -1 );
*
* 			// OR equivalent
* 			ring_buffer_get_u32( buf_inst, 3 );
*
*			// OLDEST DATA IN BUFFER
*			ring_buffer_get_u32( buf_inst, 0 );
*
*			// OR equivivalent
*			ring_buffer_get_u32( buf_inst, -4 );
*
*
* @endcode
*
* @param[in]  	buf_inst	- Ring buffer instance
* @param[in]	idx			- Index of wanted data
* @return       data		- Ring buffer data at index idx
*/
////////////////////////////////////////////////////////////////////////////////
uint32_t ring_buffer_get_u32(p_ring_buffer_t buf_inst, const int32_t idx)
{
	uint32_t data = 0;
	uint32_t buf_idx = 0;

	// Check for instance and initialization
	if 	( NULL != buf_inst )
	{
		if ( true == buf_inst->is_init )
		{
			// Check validy of requestd idx
			if ( true == ring_buffer_check_index( idx, buf_inst->size ))
			{
				// Get parsed buffer index
				buf_idx = ring_buffer_parse_index( idx, buf_inst->idx, buf_inst->size );

				// Get data
				data = buf_inst->p_data[ buf_idx ].u32;
			}
		}
	}

	return data;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    	Get signed 32-bit value from ring buffer at requesed index.
*
* @note 	Index of aquired data must be within range of:
*
* 					-size < idx < ( size - 1 )
*
* @code
*
*			// EXAMPLE OF RING BUFFER ACCESS
*			// NOTE: Size of buffer in that example is 4
*
* 			// LATEST DATA IN BUFFER
* 			// Example of getting latest data from ring buffer,
* 			// nevertheless of buffer size
* 			ring_buffer_get_i32( buf_inst, -1 );
*
* 			// OR equivalent
* 			ring_buffer_get_i32( buf_inst, 3 );
*
*			// OLDEST DATA IN BUFFER
*			ring_buffer_get_i32( buf_inst, 0 );
*
*			// OR equivivalent
*			ring_buffer_get_i32( buf_inst, -4 );
*
*
* @endcode
*
* @param[in]  	buf_inst	- Ring buffer instance
* @param[in]	idx			- Index of wanted data
* @return       data		- Ring buffer data at index idx
*/
////////////////////////////////////////////////////////////////////////////////
int32_t ring_buffer_get_i32(p_ring_buffer_t buf_inst, const int32_t idx)
{
	int32_t data = 0;
	uint32_t buf_idx = 0;

	// Check for instance and initialization
	if ( NULL != buf_inst )
	{
		if ( true == buf_inst->is_init )
		{
			// Check validy of requestd idx
			if ( true == ring_buffer_check_index( idx, buf_inst->size ))
			{
				// Get parsed buffer index
				buf_idx = ring_buffer_parse_index( idx, buf_inst->idx, buf_inst->size );

				// Get data
				data = buf_inst->p_data[ buf_idx ].i32;
			}
		}
	}

	return data;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    	Get floating value from ring buffer at requesed index.
*
* @note 	Index of aquired data must be within range of:
*
* 					-size < idx < ( size - 1 )
* @section Example
* @code
*
*			// EXAMPLE OF RING BUFFER ACCESS
*			// NOTE: Size of buffer in that example is 4
*
* 			// LATEST DATA IN BUFFER
* 			// Example of getting latest data from ring buffer,
* 			// nevertheless of buffer size
* 			ring_buffer_get_f( buf_inst, -1 );
*
* 			// OR equivalent
* 			ring_buffer_get_f( buf_inst, 3 );
*
*			// OLDEST DATA IN BUFFER
*			ring_buffer_get_f( buf_inst, 0 );
*
*			// OR equivivalent
*			ring_buffer_get_f( buf_inst, -4 );
*
*
* @endcode
*
* @param[in]  	buf_inst	- Ring buffer instance
* @param[in]	idx			- Index of wanted data
* @return       data		- Ring buffer data at index idx
*/
////////////////////////////////////////////////////////////////////////////////
float32_t ring_buffer_get_f(p_ring_buffer_t buf_inst, const int32_t idx)
{
	float32_t data = 0;
	uint32_t buf_idx = 0;

	// Check for instance and initialization
	if ( NULL != buf_inst )
	{
		if ( true == buf_inst->is_init )
		{
			// Check validy of requestd idx
			if ( true == ring_buffer_check_index( idx, buf_inst->size ))
			{
				// Get parsed buffer index
				buf_idx = ring_buffer_parse_index( idx, buf_inst->idx, buf_inst->size );

				// Get data
				data = buf_inst->p_data[ buf_idx ].f;
			}
		}
	}

	return data;
}

#endif // OBSOLETE

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////







/**
 *  Test buffer 1 
 */
p_ring_buffer_t buf_1 = NULL;
ring_buffer_attr_t buf_1_attr = 
{
    .name       = "Buffer 1",
    .p_mem      = NULL,
    .item_size  = sizeof(uint8_t),
    .override   = false
};


static uint16_t buf_2_mem[5]; 
p_ring_buffer_t buf_2 = NULL;
ring_buffer_attr_t buf_2_attr = 
{
    .name       = "Buffer 2",
    .p_mem      = &buf_2_mem,
    .item_size  = 2,
    .override   = false
};

p_ring_buffer_t buf_3 = NULL;


void print_buf_info(p_ring_buffer_t p_buf);
void dump_buffer(p_ring_buffer_t p_buf);



int main(void * args)
{   
    ring_buffer_status_t status;

    status = ring_buffer_init( &buf_1, 4, &buf_1_attr );

    //print_buf_info( buf_1 );
	//dump_buffer( buf_1 );


    status = ring_buffer_init( &buf_2, 5, &buf_2_attr );

    //print_buf_info( buf_2 );
	//dump_buffer( buf_2 );


	char 		cmd[16];
	float32_t	val;
	const char * gs_status_str[10] =
	{
		"eRING_BUFFER_OK",

		"eRING_BUFFER_ERROR",	
		"eRING_BUFFER_ERROR_INIT",		
		"eRING_BUFFER_ERROR_MEM",		
		"eRING_BUFFER_ERROR_INST",

		"eRING_BUFFER_FULL",			
		"eRING_BUFFER_EMPTY",			
	};

	printf("-----------------------------------------------------\n");
	printf(" READY TO DEBUG... \n");
	printf("-----------------------------------------------------\n");
	
	while(1)
	{
		// Get input
		scanf("%s %g", cmd, &val );

		// Commands actions
		if ( 0 == strncmp( "add", cmd, 3 ))
		{
			printf("Adding to buffer...\n" );


			//uint32_t u32_val = (uint32_t) val;
			uint8_t u8_val = (uint32_t) val;
			status = ring_buffer_add( buf_1, &u8_val );

			printf("Status: %s\n", gs_status_str[status] );
			dump_buffer( buf_1 );
			printf("\n\n");
		}
		else if ( 0 == strncmp( "get", cmd, 3 ))
		{
			printf("Getting from buffer...\n" );

			uint8_t u8_val_rnt = 0;
			status = ring_buffer_get( buf_1, &u8_val_rnt );

			printf("Status: %s, rtn_val: %d\n", gs_status_str[status], u8_val_rnt );
			dump_buffer( buf_1 );
			printf("\n\n");
		}
		else if ( 0 == strncmp( "get_index", cmd, 6 ))
		{
			printf("Geting from buffer by index...\n" );
		}
		else if ( 0 == strncmp( "exit", cmd, 4 ))
		{
			break;
		}
		else
		{
			printf("Unknown command!\n");
		}


	}

    return 0;
}



void print_buf_info(p_ring_buffer_t p_buf)
{
    static char name[32];
	uint32_t size;
	uint32_t item_size;
	uint32_t free;
	uint32_t taken;

    ring_buffer_get_name( p_buf, (char*const) &name );
	ring_buffer_get_size( p_buf, &size );
	ring_buffer_get_item_size( p_buf, &item_size );
	ring_buffer_get_free( p_buf, &free );
	ring_buffer_get_taken( p_buf, &taken );

	printf( "----------------------------------------\n" );
	printf( "    General informations \n" );
	printf( "----------------------------------------\n" );
    printf( " Name:\t\t%s\n", name );
    printf( " Size:\t\t%d\n", size );
    printf( " Item size:\t%d\n", item_size );
    printf( " Free:\t\t%d\n", free );
    printf( " Taken:\t\t%d\n", taken );
	printf( "----------------------------------------\n" );
}

void dump_buffer(p_ring_buffer_t p_buf)
{
	static char name[32];
	uint32_t size;
	uint32_t item_size;
	uint32_t i;
	uint32_t j;

	static uint8_t dump_mem[256];

    ring_buffer_get_name( p_buf, (char*const) &name );
	ring_buffer_get_size( p_buf, &size );
	ring_buffer_get_item_size( p_buf, &item_size );

	memcpy( &dump_mem, p_buf->p_data, 256 );

	printf( "----------------------------------------\n" );
	printf( "    %s dump\n", name );
	printf( "----------------------------------------\n" );

	for ( i = 0; i < size; i++ )
	{
		printf( " location: %d\titems: ", i );

		for ( j = 0; j < item_size; j++ )
		{
			printf( "0x%.2x, ", dump_mem[ item_size * i + j ] );
		} 

		if ( i == p_buf->head )
		{
			printf( "  <--HEAD" );

			if ( p_buf->is_full )
			{
				printf( " (full) ");
			}
		}

		if ( i == p_buf->tail )
		{
			printf( "  <--TAIL" );

			if ( p_buf->is_empty )
			{
				printf( " (empty) ");
			}
		}

		printf("\n");
	}

	printf( "----------------------------------------\n" );
}
