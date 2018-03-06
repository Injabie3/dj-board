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
#include "platform.h"

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

int circular_buf_reset(circular_buf_t * cbuf);
int circular_buf_put(circular_buf_t * cbuf, uint32_t data);
//TODO: int circular_buf_put_range(circular_buf_t cbuf, uint8_t * data, size_t len);
int circular_buf_get(circular_buf_t * cbuf, uint32_t * data);
int circular_buf_getSummedTaps(circular_buf_t * cbuf, uint32_t * data, int distanceApart);
//TODO: int circular_buf_get_range(circular_buf_t cbuf, uint8_t *data, size_t len);
bool circular_buf_empty(circular_buf_t cbuf);
bool circular_buf_full(circular_buf_t cbuf);

// Resets the circular buffer.
// Arguments:
// cbuf:	The circular buffer struct.
//
// Returns:
// 0 - Successfully reset.
// 1 - Could not reset.  Check your cbuf object.
int circular_buf_reset(circular_buf_t * cbuf)
{
    int r = -1;

    if(cbuf)
    {
        cbuf->head = 0;
        cbuf->tail = 0;
        r = 0;
    }

    return r;
}

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
int circular_buf_put(circular_buf_t * cbuf, uint32_t data)
{
    int r = -1;

    if(cbuf)
    {
        cbuf->buffer[cbuf->head] = data;
        cbuf->head = (cbuf->head + 1) % cbuf->size;

        if(cbuf->head == cbuf->tail)
        {
            cbuf->tail = (cbuf->tail + 1) % cbuf->size;
        }

        r = 0;
    }

    return r;
}


// Grabs a value from "tail" of the circular buffer, and advances
// the tail pointer.
//
// Arguments:
// cbuf:	The circular buffer struct.
// data:	A pointer to store the data in.
//
// Return:
// 0 - Data successfully retreived.
// 1 - Invalid cbuf, or the buffer is empty.
int circular_buf_get(circular_buf_t * cbuf, uint32_t * data)
{
    int r = -1;

    if(cbuf && data && !circular_buf_empty(*cbuf))
    {
        *data = cbuf->buffer[cbuf->tail];
        cbuf->tail = (cbuf->tail + 1) % cbuf->size;

        r = 0;
    }

    return r;
}

int circular_buf_getSummedTaps(circular_buf_t * cbuf, uint32_t * data, int distanceApart)
{
    int r = -1;

    if(cbuf && data && !circular_buf_empty(*cbuf))
    {
    	int tap1Index = cbuf->tail;
    	int tap2Index = (cbuf->tail + distanceApart) % cbuf->size;
    	int tap3Index = (cbuf->tail + 2*distanceApart) % cbuf->size;
    	uint16_t leftChannel = cbuf->buffer[tap1Index];
    	uint16_t rightChannel = cbuf->buffer[tap1Index] >> 16;

    	leftChannel += (uint16_t)(cbuf->buffer[tap2Index]);// >> 2;
    	leftChannel += (uint16_t)(cbuf->buffer[tap3Index]);// >> 4;
    	rightChannel += (uint16_t)(cbuf->buffer[tap2Index] >> 16);// >> 2;
		rightChannel += (uint16_t)(cbuf->buffer[tap3Index] >> 16);// >> 4;
        //*data = cbuf->buffer[cbuf->tail];
		*data = (uint32_t)(rightChannel << 16) | leftChannel;
        cbuf->tail = (cbuf->tail + 1) % cbuf->size;

        r = 0;
    }

    return r;
}

// Checks to see if the buffer is empty.
//
// Arguments:
// cbuf:	The circular buffer struct.
//
// Returns:
// true  - Buffer is empty.
// false - Buffer is empty.
bool circular_buf_empty(circular_buf_t cbuf)
{
	// We define empty as head == tail
    return (cbuf.head == cbuf.tail);
}

// Checks to see if the buffer is full.
//
// Ar
bool circular_buf_full(circular_buf_t cbuf)
{
	// We determine "full" case by head being one position behind the tail
	// Note that this means we are wasting one space in the buffer!
	// Instead, you could have an "empty" flag and determine buffer full that way
    return ((cbuf.head + 1) % cbuf.size) == cbuf.tail;
}

//int main(void)
//{
//	circular_buf_t cbuf;
//	cbuf.size = 10;
//
//	printf("\n=== C Circular Buffer Check ===\n");
//
//	circular_buf_reset(&cbuf); //set head/tail to 0
//
//	cbuf.buffer = malloc(cbuf.size);
//
//	printf("Full: %d, empty: %d\n", circular_buf_full(cbuf), circular_buf_empty(cbuf));
//
//	printf("Adding 9 values\n");
//	for(uint8_t i = 0; i < cbuf.size - 1; i++)
//	{
//		circular_buf_put(&cbuf, i);
//	}
//
//	printf("Full: %d, empty: %d\n", circular_buf_full(cbuf), circular_buf_empty(cbuf));
//
//	printf("Reading back values: ");
//	while(!circular_buf_empty(cbuf))
//	{
//		uint8_t data;
//		circular_buf_get(&cbuf, &data);
//		printf("%u ", data);
//	}
//	printf("\n");
//
//	printf("Adding 15 values\n");
//	for(uint8_t i = 0; i < cbuf.size + 5; i++)
//	{
//		circular_buf_put(&cbuf, i);
//	}
//
//	printf("Full: %d, empty: %d\n", circular_buf_full(cbuf), circular_buf_empty(cbuf));
//
//	printf("Reading back values: ");
//	while(!circular_buf_empty(cbuf))
//	{
//		uint8_t data;
//		circular_buf_get(&cbuf, &data);
//		printf("%u ", data);
//	}
//	printf("\n");
//
//	free(cbuf.buffer);
//
//	return 0;
//}
