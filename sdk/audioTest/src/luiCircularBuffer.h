// Note: This file is originally from
// https://github.com/embeddedartistry/embedded-resources/blob/master/examples/c/circular_buffer.c
//
// Modified by Ryan Lui (rclui@sfu.ca)
// - Added function comments.
// - Changed uint8_t to uint32_t to support our use case.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifndef SRC_LUICIRCULARBUFFER_H_
#define SRC_LUICIRCULARBUFFER_H_
typedef struct {
	uint32_t * buffer;
	size_t head;
	size_t tail;
	size_t size; //of the buffer
} circular_buf_t;

/**
* Important Usage Note: This library reserves one spare byte for queue-full detection
* Otherwise, corner cases and detecting difference between full/empty is hard.
* You are not seeing an accidental off-by-one.
*/

// Resets the circular buffer.
// Arguments:
// cbuf:	The circular buffer struct.
//
// Returns:
// 0 - Successfully reset.
// 1 - Could not reset.  Check your cbuf object.
int circular_buf_reset(circular_buf_t * cbuf);

// Insert data into the "head" of the circular buffer, and advances
// the head pointer. Must be careful to see if the buffer is full or
// not before inserting!
//
// Arguments:
// cbuf:	The circular buffer struct.
// data:	The u64 data you want to insert.
//
// Returns:
// 0 - Data inserted.
// 1 - Invalid cbuf struct passed in.
int circular_buf_put(circular_buf_t * cbuf, uint32_t data);

//TODO: int circular_buf_put_range(circular_buf_t cbuf, uint8_t * data, size_t len);

// Grabs a value from "tail" of the circular buffer, and advances
// the tail pointer.
//
// Arguments:
// cbuf:	The circular buffer struct.
// data:	A pointer to store the data in.
//
// Returns:
// 0 - Data successfully retreived.
// 1 - Invalid cbuf, or the buffer is empty.
int circular_buf_get(circular_buf_t * cbuf, uint32_t * data);

// Grabs values from multiple taps, and sums them together.
// In this case, we are using 3 taps, with the first tap weighted
// 100%, the second weighted 40%, and the last one weighted 10%
//
// Arguments:
// cbuf:			The circular buffer struct.
// data:			A pointer where the result will be stored.
// distanceApart:	The spacing between taps.
//
// Returns:
// 0 - Data successfully retrieved.
// 1 - Invalid cbuf, or buffer is empty.
int circular_buf_getSummedTaps(circular_buf_t * cbuf, uint32_t * data, int distanceApart);

//TODO: int circular_buf_get_range(circular_buf_t cbuf, uint8_t *data, size_t len);

// Checks to see if the buffer is empty.
//
// Arguments:
// cbuf:	The circular buffer struct.
//
// Returns:
// true  - Buffer is empty.
// false - Buffer is empty.
bool circular_buf_empty(circular_buf_t cbuf);


bool circular_buf_full(circular_buf_t cbuf);

#endif // SRC_LUICIRCULARBUFFER_H_
