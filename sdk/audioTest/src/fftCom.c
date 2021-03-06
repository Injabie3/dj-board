#include "fftCom.h"
#include <stdio.h>
#include "xaxidma.h"
#include "luiMemoryLocations.h"
#include "xgpio.h" 					// GPIO drivers, PL side.
#include "xgpiops.h"				// GPIO drivers, PS side.

static XGpio gpioFftConfig;					// AXI GPIO object for the FFT configuration.

//#define FFT_1024
#define FFT_2048

const u64 scalingSchedule512 = 0b010101010100000000;
const u32 scalingSchedule1024 = 0b1011000000;
const u32 scalingSchedule2048 = 0b101010000000;

// This function prepares the sample data of the IFFT output data to be sent back to the audio codec.
// Parameters:
// - bufferToShift:		The buffer that contains the unmodified IFFT output data.
// - bufferToStoreIn:	The buffer to store the rearranged data.
void shiftBits(volatile u64* bufferToShift, volatile u64* bufferToStoreIn) {

	// Some easy constants to adjust.
	const uint BITS_TO_SHIFT = 0;

	u64 tempBuffer[LUI_FFT_SIZE];

	for (int i=0; i < LUI_FFT_SIZE; i++) {
		tempBuffer[i] = (bufferToShift[i] << BITS_TO_SHIFT) & 0xFFFF; // the LSB part
		tempBuffer[i] = (((bufferToShift[i] & 0xFFFF0000) << BITS_TO_SHIFT) & 0xFFFF0000) | tempBuffer[i]; // concatenating as we go
		tempBuffer[i] = (((bufferToShift[i] & 0xFFFF00000000) << BITS_TO_SHIFT) & 0xFFFF00000000) | tempBuffer[i];
		//for the last one do we need to & it again ? i don't think so lol
		tempBuffer[i] = ((bufferToShift[i] & 0xFFFF000000000000) << BITS_TO_SHIFT) | tempBuffer[i];

		if (i==0)
			bufferToStoreIn[i] = tempBuffer[i];
		else {
			bufferToStoreIn[LUI_FFT_SIZE-i] = tempBuffer[i];
		}
	}

}

// This function initializes the FFT DMA.
int setupDma(XAxiDma* axiDma, u16 DeviceId) {
	XAxiDma_Config *CfgPtr;
	int status;

	CfgPtr = XAxiDma_LookupConfig(DeviceId);
	if (!CfgPtr) {
		xil_printf("No config found for %d\r\n", DeviceId);
		return XST_FAILURE;
	}

	status = XAxiDma_CfgInitialize(axiDma, CfgPtr);
	if (status != XST_SUCCESS) {
		xil_printf("Initialization failed %d\r\n", status);
		return XST_FAILURE;
	}

	if (XAxiDma_HasSg(axiDma)) {
		xil_printf("Device configured as SG mode \r\n");
		return XST_FAILURE;
	}

	// Disable interrupts, we use polling mode
	XAxiDma_IntrDisable(axiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrDisable(axiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
	
	return status;
}

// This function initializes the FFT GPIO.
int setUpFftGpio(u16 DeviceId) {
	int status = XGpio_Initialize(&gpioFftConfig, DEVICE_ID_FFT_GPIO);
	if (status != XST_SUCCESS ){
		xil_printf("Initialization failed %d\r\n", status);
	}

	return status;
}

// This function does the following:
// - Does a data transfer to and from the FFT core via the DMA
//   to perform a forward FFT.
int XAxiDma_FftDataTransfer(u16 DeviceId, volatile u64* inputBuffer, volatile u64* outputBuffer, XAxiDma* axiDma) {
	int status;

	// flush the cache
	Xil_DCacheFlushRange((UINTPTR)inputBuffer, LUI_FFT_SIZE*8); // FFT_point size * 8 bytes since we have 64 bits.
	Xil_DCacheFlushRange((UINTPTR)outputBuffer, LUI_FFT_SIZE*8);

	/**********************Start data transfer with FFT***************************/
	status = XAxiDma_SimpleTransfer(axiDma, (UINTPTR)outputBuffer, LUI_FFT_SIZE*8, XAXIDMA_DEVICE_TO_DMA);

	if (status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	status = XAxiDma_SimpleTransfer(axiDma, (UINTPTR)inputBuffer, LUI_FFT_SIZE*8, XAXIDMA_DMA_TO_DEVICE);

	if (status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	// loop while DMA is busy
	while ((XAxiDma_Busy(axiDma,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(axiDma,XAXIDMA_DMA_TO_DEVICE))) {
			/* Wait */
	}

	return 0;
}

int XAxiDma_MixerDataTransfer(u16 DeviceId, volatile u32* inputBuffer, volatile u32* outputBuffer, XAxiDma* axiDma, int8_t bothDirection){

	//****************** Configure the DMA ***********************************/
	int status;

	// flush the cache
	Xil_DCacheFlushRange((UINTPTR)inputBuffer, LUI_FFT_SIZE/2*4); // Send half samples * 4 bytes since 32 bits.
	if (bothDirection == 1) {
		Xil_DCacheFlushRange((UINTPTR)outputBuffer, LUI_FFT_SIZE/2*4);
	}

	//
	if (bothDirection == 1){
		status = XAxiDma_SimpleTransfer(axiDma, (UINTPTR)outputBuffer, LUI_FFT_SIZE/2*4, XAXIDMA_DEVICE_TO_DMA);

		if (status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	}

	status = XAxiDma_SimpleTransfer(axiDma, (UINTPTR)inputBuffer, LUI_FFT_SIZE/2*4, XAXIDMA_DMA_TO_DEVICE);

	if (status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	// loop while DMA is busy
	if (bothDirection == 1){
		while ((XAxiDma_Busy(axiDma, XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(axiDma, XAXIDMA_DMA_TO_DEVICE))) {
			/* Wait */
		}
	}


	return 0;
}

// This function configures the FFT for time-to-frequency domain transformation.
int fftConfigForward() {

	/********************Configure the FFT***********************/
	// configure forward FFT for each channel
	// this is configuring for 010101...
	// top 2 channels are IFFT bottom 2 are forward FFT


#ifdef FFT_1024
	u32 configData = 0b11 | scalingSchedule1024 << 2 | scalingSchedule1024 << 12; // 256pt FFT
	// Configuration:
	// GPIO Channel 1:
	// Bits 0, 1: 	Forward/Inverse for channel 0 and 1 respectively
	// Bits 2-12: 	Scaling schedule for channel 0.
	// Bits 13-22:	Scaling schedule for channel 1.
	// GPIO Channel 2:
	// Bit 1:		Valid signal.

	XGpio_DiscreteWrite(&gpioFftConfig, 1, configData);
	XGpio_DiscreteWrite(&gpioFftConfig, 2, 0b1);
#endif // FFT_1024

#ifdef FFT_2048
	u32 configData = 0b11 | scalingSchedule2048 << 2 | scalingSchedule2048 << 14;
	// Configuration:
	// GPIO Channel 1:
	// Bits 0, 1: 	Forward/Inverse for channel 0 and 1 respectively
	// Bits 2-12: 	Scaling schedule for channel 0.
	// Bits 13-22:	Scaling schedule for channel 1.
	// GPIO Channel 2:
	// Bit 1:		Valid signal.

	XGpio_DiscreteWrite(&gpioFftConfig, 1, configData);
	XGpio_DiscreteWrite(&gpioFftConfig, 2, 0b1);
#endif // FFT_2048
	return 0;
}

// This function configures the FFT for frequency-to-time domain transformation.
int fftConfigInverse() {

	/********************Configure the FFT***********************/
	// configure forward FFT for each channel
	// this is configuring for 010101...
	// top 2 channels are IFFT bottom 2 are forward FFT

#ifdef FFT_1024
	u32 configData = 0b00 | scalingSchedule1024 << 2 | scalingSchedule1024 << 12; // 256pt FFT
	// Configuration:
	// GPIO Channel 1:
	// Bits 0, 1: 	Forward/Inverse for channel 0 and 1 respectively
	// Bits 2-12: 	Scaling schedule for channel 0.
	// Bits 13-22:	Scaling schedule for channel 1.
	// GPIO Channel 2:
	// Bit 1:		Valid signal.

	XGpio_DiscreteWrite(&gpioFftConfig, 1, configData);
	XGpio_DiscreteWrite(&gpioFftConfig, 2, 0b1);
#endif // FFT_1024

#ifdef FFT_2048
	u32 configData = 0b00 | scalingSchedule2048 << 2 | scalingSchedule2048 << 14;
	// Configuration:
	// GPIO Channel 1:
	// Bits 0, 1: 	Forward/Inverse for channel 0 and 1 respectively
	// Bits 2-12: 	Scaling schedule for channel 0.
	// Bits 13-22:	Scaling schedule for channel 1.
	// GPIO Channel 2:
	// Bit 1:		Valid signal.

	XGpio_DiscreteWrite(&gpioFftConfig, 1, configData);
	XGpio_DiscreteWrite(&gpioFftConfig, 2, 0b1);
#endif // FFT_2048
	return 0;
}
