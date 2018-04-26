#ifndef FFTCOM_H
#define FFTCOM_H

#include <stdio.h>
#include "xil_exception.h"
#include "xaxidma.h"


// This function initializes the FFT DMA and GPIO respectively.
int setupDma(XAxiDma* axiDma, u16 DeviceId);
int setUpFftGpio(u16 DeviceId);

// Configuration functions to adjust the AXI-S FFT core.

// This function configures the FFT for time-to-frequency domain transformation.
int fftConfigForward();

// This function configures the FFT for frequency-to-time domain transformation.
int fftConfigInverse();


// This function prepares the sample data of the IFFT output data to be sent back to the audio codec.
// Parameters:
// - bufferToShift:		The buffer that contains the unmodified IFFT output data.
// - bufferToStoreIn:	The buffer to store the rearranged data.
void shiftBits(volatile u64* bufferToShift, volatile u64* bufferToStoreIn);

// This function does the following:
// - Does a data transfer to and from the FFT core via the DMA
//   to perform a forward FFT.
int XAxiDma_FftDataTransfer(u16 DeviceId, volatile u64* inputBuffer, volatile u64* outputBuffer, XAxiDma* axiDma);

int XAxiDma_MixerDataTransfer(u16 DeviceId, volatile u32* inputBuffer, volatile u32* outputBuffer, XAxiDma* axiDma, int8_t bothDirection);

#endif
