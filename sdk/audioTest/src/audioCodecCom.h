#ifndef AUDIOCODECCOM_H
#define AUDIOCODECCOM_H

#include <stdio.h>
#include "xil_exception.h"
#include "luiMemoryLocations.h"


void audioDriver();

void adjustPitch();

// Receives data from line in.
// Parameters:
// samplesToRead:	The number of samples to read.
// toBuffer:		A pointer to the buffer in which the read data will be stored.
// offset:			Start storing samples from the specified offset of the buffer above.
void dataIn(int samplesToRead, volatile u64* toBuffer, int offset);

// Sends data out to headphone out.
// Parameters:
// samplesToSend:	The number of samples to send.
// fromBuffer:		A pointer to the buffer containing the samples to send.
// offset:			Start sending samples from the specified offset of the buffer above.
void dataOut(int samplesToSend, volatile u32* fromBuffer, int offset, bool circularBufferOnly);

void equalize();

#endif // AUDIOCODECCOM_H
