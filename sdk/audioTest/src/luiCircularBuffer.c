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
	size_t head;	// Index of the array where we will insert the next entry.
	size_t tail;	// Index of the array where we will "pop"
	size_t size; //of the buffer
	size_t startingIndex;	// The offset in which we will start fetching.  Used for echo.
} circular_buf_t;

/**
* Important Usage Note: This library reserves one spare byte for queue-full detection
* Otherwise, corner cases and detecting difference between full/empty is hard.
* You are not seeing an accidental off-by-one.
*/

int circular_buf_reset(circular_buf_t * cbuf);
int circular_buf_put(circular_buf_t * cbuf, uint32_t data);
int circular_buf_get(circular_buf_t * cbuf, uint32_t * data);
int circular_buf_getSummedTaps(circular_buf_t * cbuf, uint32_t * data, int distanceApart);
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
        cbuf->startingIndex = 12000;
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


// Grabs a value from the tail + startingIndex of the circular buffer,
// and advances the tail pointer.
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
    	int index = (cbuf->tail + cbuf->startingIndex) % cbuf->size;
        *data = cbuf->buffer[index];
        cbuf->tail = (cbuf->tail + 1) % cbuf->size;

        r = 0;
    }

    return r;
}

// Grabs values from multiple taps, and sums them together.
// In this case, we are using 3 taps, with the first tap weighted
// 100%, the second weighted 50%, and the third one weighted 25%.
// This requires your buffer size to be >= 12000.
// The first tap will be at index startingIndex, and every other tap will
// be at -distanceApart from the previous tap.
//
// Arguments:
// cbuf:			The circular buffer struct.
// data:			A pointer where the result will be stored.
// distanceApart:	The spacing between taps.
//
// Returns:
// 0 - Data successfully retrieved.
// 1 - Invalid cbuf, buffer is empty, or size not large enough.
int circular_buf_getSummedTaps(circular_buf_t * cbuf, uint32_t * data, int distanceApart)
{
	// Do a check for distanceApart wrt startingIndex
	if (4*distanceApart >= cbuf->startingIndex) {
		return 1;
	}
    int r = 1;

    if(cbuf && data && !circular_buf_empty(*cbuf) && (cbuf->size >= cbuf->startingIndex))
    {

    	int tap1Index = (cbuf->tail + cbuf->startingIndex) % cbuf->size;
    	int tap2Index = (cbuf->tail + cbuf->startingIndex - distanceApart) % cbuf->size;
    	int tap3Index = (cbuf->tail + cbuf->startingIndex - 2*distanceApart) % cbuf->size;
    	int tap4Index = (cbuf->tail + cbuf->startingIndex - 3*distanceApart) % cbuf->size;
    	int tap5Index = (cbuf->tail + cbuf->startingIndex - 4*distanceApart) % cbuf->size;
    	int16_t leftChannel = cbuf->buffer[tap1Index];
    	int16_t rightChannel = cbuf->buffer[tap1Index] >> 16;

    	// Progressively, for each tap, reduce the amplitude.
    	leftChannel += (int16_t)(cbuf->buffer[tap2Index]) >> 1;
    	leftChannel += (int16_t)(cbuf->buffer[tap3Index]) >> 2;
    	leftChannel += (int16_t)(cbuf->buffer[tap4Index]) >> 3;
    	leftChannel += (int16_t)(cbuf->buffer[tap5Index]) >> 4;

    	rightChannel += (int16_t)(cbuf->buffer[tap2Index] >> 16) >> 1;
		rightChannel += (int16_t)(cbuf->buffer[tap3Index] >> 16) >> 2;
		rightChannel += (int16_t)(cbuf->buffer[tap4Index] >> 16) >> 3;
		rightChannel += (int16_t)(cbuf->buffer[tap5Index] >> 16) >> 4;

        //*data = cbuf->buffer[cbuf->tail];
		*data = ((uint16_t)(rightChannel) << 16) | (uint16_t)leftChannel;
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


// ###############
// # SAMPLE CODE #
// ###############
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
