////////////////////////////////////////////////////////////////////////////////
/**
*@file      ring_buffer.c
*@brief     Ring (circular) buffer for general use
*@author    Ziga Miklosic
*@date      03.02.2021
*@version   V1.0.0
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
 * 	Buffer data types
 */
typedef union
{
	uint32_t	u32;	/**<Unsigned 32-bit value */
	int32_t		i32;	/**<Signed 32-bit value */
	float32_t	f;		/**<32-bit floating value */
} ring_buffer_data_t;

/**
 * 	Ring buffer
 */
typedef struct ring_buffer_s
{
	ring_buffer_data_t *p_data;		/**<Data in buffer */
	uint32_t 			idx;		/**<Pointer to oldest data */
	uint32_t			size;		/**<Size of buffer */
} ring_buffer_t;

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////
static uint32_t ring_buffer_wrap_index		(const uint32_t idx, const uint32_t size);
static uint32_t ring_buffer_increment_index	(const uint32_t idx, const uint32_t size);
static uint32_t ring_buffer_parse_index		(const int32_t idx_req, const uint32_t idx_cur, const uint32_t size);
static bool 	ring_buffer_check_index		(const int32_t idx_req, const uint32_t size);

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////


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

	if ( NULL != buf_inst )
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

	if ( NULL != buf_inst )
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

	if ( NULL != buf_inst )
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

	if 	( NULL != buf_inst )
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

	if ( NULL != buf_inst )
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

	if ( NULL != buf_inst )
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

	return data;
}

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
