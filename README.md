# **Ring buffer**
Ring buffer implementation offers an efficient and memory-constrained C module for managing a continuous, circular data storage structure within resource-limited environments. Designed to optimize memory utilization while enabling seamless data manipulation, this implementation is particularly well-suited for embedded systems, real-time applications, and scenarios where memory efficiency is important. Ring buffer module is very flexible, it can work with byte size items or larger data structures. Additionally each ring buffers are created as individual, separated instances providing freedom of configuring each buffer instance by its own.

Override mode is supported where buffer is never full and new values are always overriding old values regarding of reading rate. This functionality is very usefull for filter sampling storage purposes.

Additionally buffers data storage can be allocated statically if dynamic allocation is not perfered by application. Look at the example of static allocation of memory.

There are two distinct get functions: *"ring_buffer_get"* and *"ring_buffer_get_by_index"*.
First one returns oldest item in buffer and acts as a FIFO, meaning that tail increments
at every call of it. On the other side *"ring_buffer_get_by_index"* returns value relative
to input argument value and does not increment tail pointer! It is important not to
use those two get functionalities simultaniously. 

Function *"ring_buffer_get_by_index"* supports two kind of access types:

1. **NORMAL ACCESS: 	classical aproach**, where index is a positive
						number and simple represants buffer index. This approach
						has no information about time stamps of values inside buffer.
						Range: [0, size)

2. **INVERS ACCESS: 	chronologically aproach**, where index is a negative number.
						Meaning that "-1" value will always returns latest value in
						buffer and "-size" index value will return oldest value
						in buffer. ***This feature becomes very handy when performing
						digital filtering (convolution) where ring buffer can represants sample
						window and thus easy access from oldest to latest sample
						can be achieved with invers access.***
						Range of index: [-size, -1]



## **Dependencies**

This module needs only ANSI C standard libraries. 

## **General Embedded C Libraries Ecosystem**
In order to be part of *General Embedded C Libraries Ecosystem* this module must be placed in following path: 
```
root/middleware/ring_buffer/"module_space"
```

## **Limitations**

### **1. Multientry**
Guidance for multi-entry usage: 
 - **It is recomented to use ring_buffer between two task/interrupts/cores in provider/consumer manner.** Meaning one task/interrupt/core is writing to ring_buffer and other task/interrupt/core is reading from it.
 - **It is not recommended for two or more task/interrupt/core to read/write to same ring_buffer instance!** Meaning that data provider and consumer are two separate tasks/interrupts/cores.


## **API**

| API Functions | Description | Prototype |
| --- | ----------- | ----- |
| **ring_buffer_init** 				| Initialization of ring buffer | ring_buffer_status_t ring_buffer_init(p_ring_buffer_t * p_ring_buffer, const uint32_t size, const ring_buffer_attr_t * const p_attr) |
| **ring_buffer_is_init** 			| Get initialization flag | ring_buffer_status_t ring_buffer_is_init(p_ring_buffer_t buf_inst, bool * const p_is_init) |
| **ring_buffer_add** 				| Add element to ring buffer in FIFO form | ring_buffer_status_t ring_buffer_add(p_ring_buffer_t buf_inst, const void * const p_item) |
| **ring_buffer_add_multi** 		| Add multiple elements to ring buffer in FIFO form | ring_buffer_status_t ring_buffer_add_many(p_ring_buffer_t buf_inst, const void * const p_item, const uint32_t size) |
| **ring_buffer_get** 				| Get element from ring buffer in FIFO form | ring_buffer_status_t ring_buffer_get(p_ring_buffer_t buf_inst, void * const p_item) |
| **ring_buffer_get_multi** 		| Get multiple elements from ring buffer in FIFO form | ring_buffer_status_t ring_buffer_get_multi(p_ring_buffer_t buf_inst, void * const p_item, const uint32_t size) |
| **ring_buffer_get_by_index** 		| Get element from ring buffer without any side effects. Access by index. | ring_buffer_status_t	ring_buffer_get_by_index(p_ring_buffer_t buf_inst, void * const p_item, const int32_t idx) |
| **ring_buffer_reset** 			| Reset ring buffer | ring_buffer_status_t	ring_buffer_reset(p_ring_buffer_t buf_inst) |
| **ring_buffer_get_name** 			| Get ring buffer name | ring_buffer_status_t ring_buffer_get_name(p_ring_buffer_t buf_inst, char * const p_name)|
| **ring_buffer_get_taken** 		| Get number of taken space of elements inside a buffer | ring_buffer_status_t	ring_buffer_get_taken(p_ring_buffer_t buf_inst, uint32_t * const p_taken)|
| **ring_buffer_get_free** 			| Get number of free space of elements inside a buffer | ring_buffer_status_t ring_buffer_get_free(p_ring_buffer_t buf_inst, uint32_t * const p_free)|
| **ring_buffer_get_size** 			| Get size of all items inside ring buffer | ring_buffer_status_t ring_buffer_get_size(p_ring_buffer_t buf_inst, uint32_t * const p_size)|
| **ring_buffer_get_item_size** 	| Get item size in bytes | ring_buffer_status_t ring_buffer_get_item_size(p_ring_buffer_t buf_inst, uint32_t * const p_item_size)|
| **ring_buffer_is_full** 			| Is buffer full | ring_buffer_status_t ring_buffer_is_full(p_ring_buffer_t buf_inst, bool * const p_full)|
| **ring_buffer_is_empty** 			| Is buffer empty | ring_buffer_status_t ring_buffer_is_empty(p_ring_buffer_t buf_inst, bool * const p_empty)|

## **Usage**

### **Initialization examples**

```C
// My ring buffer instance
p_ring_buffer_t 		my_ringbuffer = NULL;

// Initialization as default buffer with size of 10 items + Dynamica allocation of memory
if ( eRING_BUFFER_OK != ring_buffer_init( &my_ringbuffer, 10, NULL ))
{
	// Init failed...
}


// My second ring buffer instance
p_ring_buffer_t 		my_ringbuffer_2 = NULL;
ring_buffer_attr_t		my_ringbuffer_2_attr;

// Customize ring buffer:
my_ring_buffer_2_attr.name 		= "Dynamic allocated buffer";
my_ring_buffer_2_attr.p_mem 	= NULL;	// NULL -> Dynamical allocation
my_ring_buffer_2_attr.item_size = sizeof(float32_t);
my_ring_buffer_2_attr.override 	= true;

// Initialization as customized buffer with size of 32 items + Dynamic allocation of memory
if ( eRING_BUFFER_OK != ring_buffer_init( &my_ringbuffer_2, 32, &my_ring_buffer_2_attr ))
{
	// Init failed...
}


// My third ring buffer instance
p_ring_buffer_t 		my_ringbuffer_3 = NULL;
ring_buffer_attr_t		my_ringbuffer_3_attr;
uint8_t buf_mem[128];

// Customize ring buffer:
my_ring_buffer_3_attr.name 		= "Static allocated buffer";
my_ring_buffer_3_attr.p_mem		= &buf_mem;
my_ring_buffer_3_attr.item_size = sizeof(float32_t);
my_ring_buffer_3_attr.override 	= true;

// Initialization as customised buffer with size of 32 items + Static allocation of memory
if ( eRING_BUFFER_OK != ring_buffer_init( &my_ringbuffer_2, 32, &my_ring_buffer_2_attr ))
{
	// Init failed...
}
```

### **Get items out of buffer examples**

```C
// My ring buffer is initialized for byte items
uint8_t item;

// =============================================================
//  GETTING VALUE 
// =============================================================

// Pump all items out of buffer
ring_buffer_get_taken( my_ring_buffer, &taken );

for ( uint32_t i = 0; i < taken; i++ )
{
	ring_buffer_get( my_ring_buffer, &item );

    // Do something with "item" value here...
}

// OR equivalent

while( eRING_BUFFER_EMPTY != ring_buffer_get( my_ring_buffer, &item ))
{
    // Do something with "item" value here...
}

// =============================================================
//  GETTING VALUE BY INDEX 
// =============================================================

// Get value at index 0 from ring buffer - classic access
ring_buffer_get_by_index( my_ringbuffer, &item, 0 );

// Get latest value from ring buffer - inverted access
ring_buffer_get_by_index( my_ringbuffer, &item, -1 );

// Get oldest value from ring buffer - inverted access
ring_buffer_get_by_index( my_ringbuffer, &item, -10 );


// ***** Example of filter usage of getting value by index ****

float32_t sample;

// Make convolution
for ( uint32_t i = 0; i < filter_inst -> order; i++ )
{
    // Get sample
    ring_buffer_get_by_index( filter_inst->p_x, &sample, (( -i ) - 1 ));

    // Convolve
    y += ( filter_inst->p_a[i] * sample );
}
```

### **Add item to buffer examples**

```C
// My ring buffer is initialized for byte items
uint8_t item = 42;

// Add value to buffer
ring_buffer_add( my_ringbuffer, &item );
```

### **Add multiple items to buffer examples**

```C
// My ring buffer is initialized for byte items
uint8_t items[3] = {1,2,3};

// Add items to buffer
ring_buffer_get_multi( my_ringbuffer, (uint8_t*) &items, 3 );
```
