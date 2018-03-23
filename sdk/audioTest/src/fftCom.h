#ifndef FFTCOM_H
#define FFTCOM_H

#include <stdio.h>
#include "xil_exception.h"
#include "xaxidma.h"



int XGpio_FftConfig();
int XGpio_IFftConfig();


// function to shift the bits of the IFFT output data before sending back to Codec
void shiftBits(volatile u64* bufferToShift, volatile u64* bufferToStoreIn);

void shiftBitsRight(volatile u64* bufferToShift, volatile u64* bufferToStoreIn);


// This function does the following:
// - Initializes the DMA.
// - Populates DDR with a test vector.
// - Does a data transfer to and from the FFT core via the DMA
//   to perform a forward FFT.
int XAxiDma_FFTDataTransfer(u16 DeviceId, volatile u64* inputBuffer, volatile u64* outputBuffer);

int XAxiDma_MixerDataTransfer(u16 DeviceId, volatile u32* inputBuffer, volatile u32* outputBuffer, XAxiDma axiDma, int8_t bothDirection);
#endif
