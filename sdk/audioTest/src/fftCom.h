#ifndef FFTCOM_H
#define FFTCOM_H

#include <stdio.h>
#include "xil_exception.h"



int XGpio_FftConfig();
int XGpio_IFftConfig();


// function to shift the bits of the IFFT output data before sending back to Codec
void shiftBits(volatile u64* RxBuf);


// This function does the following:
// - Initializes the DMA.
// - Populates DDR with a test vector.
// - Does a data transfer to and from the FFT core via the DMA
//   to perform a forward FFT.
int XAxiDma_FftDataTransfer(u16 DeviceId, volatile u64* TxBuf, volatile u64* RxBuf);

#endif
