#include "fftCom.h"
#include <stdio.h>
#include "xaxidma.h"
#include "luiMemoryLocations.h"
#include "xgpio.h" 					// GPIO drivers, PL side.
#include "xgpiops.h"				// GPIO drivers, PS side.

static XGpio gpioFftConfig;					// AXI GPIO object for the FFT configuration.
static XAxiDma axiDma;						// AXI DMA object that is tied with the FFT core.


void shiftBits(volatile u64* RxBuf){

	const uint POINT_SIZE = 64;
	volatile u64 *RxShiftBufferPtr;
	u64 temp[POINT_SIZE];
	RxShiftBufferPtr = (u64 *)RX_SHIFT_BUFFER_BASE;

	for (int i=0;i<POINT_SIZE;i++){
		temp[i] = (RxBuf[i] << 6) & 0xFFFF; // the LSB part
		temp[i] = (((RxBuf[i] & 0xFFFF0000) << 6) & 0xFFFF0000) | temp[i]; // concatenating as we go
		temp[i] = (((RxBuf[i] & 0xFFFF00000000) << 6) & 0xFFFF00000000) | temp[i];
		//for the last one do we need to & it again ? i don't think so lol
		temp[i] = ((RxBuf[i] & 0xFFFF000000000000) << 6) | temp[i];

		if (i==0)
			RxShiftBufferPtr[i] = temp[i];
		else {
			RxShiftBufferPtr[POINT_SIZE-i] = temp[i];
		}
	}
	// now need to swap the order  of the bits



}

// This function does the following:
// - Initializes the DMA.
// - Populates DDR with a test vector.
// - Does a data transfer to and from the FFT core via the DMA
//   to perform a forward FFT.
int XAxiDma_FftDataTransfer(u16 DeviceId, volatile u64* TxBuf, volatile u64* RxBuf){


	// making these pointers global for the purpose of using same FFT core for forward and inverse


	//****************** Configure the DMA ***********************************/
	/********************Using Xilinx Sample code provided for simple polling example********************/
	XAxiDma_Config *CfgPtr;
	int Status;
	//int Tries = NUMBER_OF_TRANSFERS;
	/* Initialize the XAxiDma device.
	 */
	CfgPtr = XAxiDma_LookupConfig(DeviceId);
	if (!CfgPtr) {
		xil_printf("No config found for %d\r\n", DeviceId);
		return XST_FAILURE;
	}

	Status = XAxiDma_CfgInitialize(&axiDma, CfgPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("Initialization failed %d\r\n", Status);
		return XST_FAILURE;
	}

	if(XAxiDma_HasSg(&axiDma)){
		xil_printf("Device configured as SG mode \r\n");
		return XST_FAILURE;
	}

	/* Disable interrupts, we use polling mode
	 */
	XAxiDma_IntrDisable(&axiDma, XAXIDMA_IRQ_ALL_MASK,
						XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrDisable(&axiDma, XAXIDMA_IRQ_ALL_MASK,
						XAXIDMA_DMA_TO_DEVICE);

	//Value = TEST_START_VALUE;


	// flush the cache
	Xil_DCacheFlushRange((UINTPTR)TxBuf, 0x200);
	//#ifdef __aarch64__
		Xil_DCacheFlushRange((UINTPTR)RxBuf, 0x200);
	//#endif

		/**********************Start data transfer with FFT***************************/
		//
			Status = XAxiDma_SimpleTransfer(&axiDma,(UINTPTR) RxBuf,
						0x200, XAXIDMA_DEVICE_TO_DMA);

			if (Status != XST_SUCCESS) {
				return XST_FAILURE;
			}

			Status = XAxiDma_SimpleTransfer(&axiDma,(UINTPTR) TxBuf,
					0x200, XAXIDMA_DMA_TO_DEVICE);

			if (Status != XST_SUCCESS) {
				return XST_FAILURE;
			}

			// loop while DMA is busy
			while ((XAxiDma_Busy(&axiDma,XAXIDMA_DEVICE_TO_DMA)) ||
						(XAxiDma_Busy(&axiDma,XAXIDMA_DMA_TO_DEVICE))) {
							/* Wait */
					}

	return 0;
}


// This function sets up the FFT core

int XGpio_FftConfig() {

	/***********************Initialize GPIO**************************/
	int status = XGpio_Initialize(&gpioFftConfig, DEVICE_ID_FFT_GPIO);
	if (status != XST_SUCCESS ){
		xil_printf("Initialization failed %d\r\n", status);
		return XST_FAILURE;
	}

	/********************Configure the FFT***********************/
	// configure forward FFT for each channel
	// this is configuring for 010101...
	// top 2 channels are IFFT bottom 2 are forward FFT
	XGpio_DiscreteWrite(&gpioFftConfig, 1, 0x1555557);
	//XGpio_DiscreteWrite(&gpioFftConfig, 1, 0b11);
	// send valid signal
	// concatenated to the config
	XGpio_DiscreteWrite(&gpioFftConfig, 2, 0b1);


	return 0;
}


int XGpio_IFftConfig() {

	/***********************Initialize GPIO**************************/
	int status = XGpio_Initialize(&gpioFftConfig, DEVICE_ID_FFT_GPIO);
	if (status != XST_SUCCESS ){
		xil_printf("Initialization failed %d\r\n", status);
		return XST_FAILURE;
	}

	/********************Configure the FFT***********************/
	// configure forward FFT for each channel
	// this is configuring for 010101...
	// top 2 channels are IFFT bottom 2 are forward FFT
	XGpio_DiscreteWrite(&gpioFftConfig, 1, 0x1555554);
	//XGpio_DiscreteWrite(&gpioFftConfig, 1, 0b11);
	// send valid signal
	XGpio_DiscreteWrite(&gpioFftConfig, 2, 0b1);
	return 0;
}
