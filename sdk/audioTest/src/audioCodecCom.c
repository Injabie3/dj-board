
#include <stdio.h>
#include "xparameters.h"
#include "xstatus.h"
#include "audioCodecCom.h"
#include "xil_printf.h"
#include "luiMemoryLocations.h"
#include "xil_exception.h"
#include "fftCom.h"

volatile u64 *TxBufferPtr = (u64*)TX_BUFFER_BASE;
volatile u64 *MxBufferPtr = (u64*)MX_BUFFER_BASE;
volatile u64 *RxBufferPtr = (u64*)RX_BUFFER_BASE;
volatile u64 *RxShiftBufferPtr = (u64*)RX_SHIFT_BUFFER_BASE;
volatile u32* AUDIOCHIP = ((volatile u32*)XPAR_AUDIOINOUT16_0_S00_AXI_BASEADDR);

void audioDriver(){
	int configStatus, status;

	// loop on audio
	while (1) {

			// read in audio data from ADC FIFO
			dataIn();

		configStatus = XGpio_FftConfig();
		status = XAxiDma_FftDataTransfer(DEVICE_ID_DMA, TxBufferPtr, MxBufferPtr);

		if (status != XST_SUCCESS) {
			xil_printf("XAxiDma_SimplePoll Example Failed\r\n");
		}
		// want to convert data back so we send it through IFFT

		configStatus = XGpio_IFftConfig();
		status = XAxiDma_FftDataTransfer(DEVICE_ID_DMA, MxBufferPtr, RxBufferPtr);

		if (status != XST_SUCCESS) {
				xil_printf("XAxiDma_SimplePoll Example Failed\r\n");
			}
		//need to convert output because it is shifted by 3 bits
		shiftBits(RxBufferPtr);

		dataOut();
	}
}

void dataIn(){
	//keep looping until we've read in 64 data samples
			u32 dataIn = 0;
			u32 temp = 0;
			u32 tempLeft = 0;
			u32 tempRight = 0;
			while (dataIn < 64){
				// check if the ADC FIFO is not empty
				if ((AUDIOCHIP[0] & 1<<2)==0){
					// get right and left channel
					// pad with 0s for 64bit input to FFT
					//0000RRRR0000LLLL
					temp = AUDIOCHIP[2];
					tempRight = temp & 0xFFFF0000;
					tempLeft = temp & 0xFFFF;
					tempRight>>=16;
					TxBufferPtr[dataIn] = (((u64)tempRight << 32) | tempLeft);
					dataIn++;
				}
			}
}

void dataOut(){
	int dataOut = 0;
	u32 temp = 0;
	u32 tempLeft = 0;
	u32 tempRight = 0;
	//now we want to check the DAC
	while(dataOut < 64){
		// if DAC FIFO is not FULL we can write data to it
		if ((AUDIOCHIP[0] & 1<<5)==0) {
			tempRight = RxShiftBufferPtr[dataOut]>>16;
			tempLeft = RxShiftBufferPtr[dataOut];
			temp = (tempRight & 0xFFFF0000)| (tempLeft & 0xFFFF);
			AUDIOCHIP[1] = temp;
			dataOut++;
		}
	}
}
