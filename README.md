# Ring buffer
Ring buffer modules is implemented for general used in embedded C code. It support three data types: uint32_t, int32_t and float32_t. Data type is not importatn at initialization point as it is only how stored information are interpreted by C compiler. 

Ring buffer memory space is dynamically allocated and success of allocation is taken into consideration before using that instance. Deallocation on exsisting ring buffer instance is not supported as it's not good practice to free memory in C world. 

#### Dependencies

Definition of flaot32_t must be provided by user. In current implementation it is defined in "*project_config.h*". Just add following statement to your code where it suits the best.

```C
// Define float
typedef float float32_t;
```

#### API

API of ring buffer constist of three blocks, those are: initialization, add element to buffer and get element from buffer. Note that getting element from buffer intakes intiger parameter as support normal and invers access. Invers access meas that get function will return time related element from ring buffer. This feature is specially handy when comes to digital filter calculations.

#### List of functions:
 - ring_buffer_status_t 	**ring_buffer_init**	(p_ring_buffer_t * p_ring_buffer, const  uint32_t size);

 - ring_buffer_status_t 	**ring_buffer_add_u32**	(p_ring_buffer_t buf_inst, const uint32_t data);
 - ring_buffer_status_t 	**ring_buffer_add_i32**	(p_ring_buffer_t buf_inst, const int32_t data );
 - ring_buffer_status_t 	**ring_buffer_add_f**	(p_ring_buffer_t buf_inst, const float32_t data);
 - uint32_t 				**ring_buffer_get_u32**	(p_ring_buffer_t buf_inst, const int32_t idx);
 - int32_t 				**ring_buffer_get_i32**	(p_ring_buffer_t buf_inst, const int32_t idx);
 - float32_t 				**ring_buffer_get_f**	(p_ring_buffer_t buf_inst, const int32_t idx);

NOTE: Detailed description of fucntion can be found in doxygen!

