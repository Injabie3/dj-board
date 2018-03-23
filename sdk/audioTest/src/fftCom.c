#include "fftCom.h"
#include <stdio.h>
#include "xaxidma.h"
#include "luiMemoryLocations.h"
#include "xgpio.h" 					// GPIO drivers, PL side.
#include "xgpiops.h"				// GPIO drivers, PS side.

static XGpio gpioFftConfig;					// AXI GPIO object for the FFT configuration.
static XAxiDma axiDmaFFT;						// AXI DMA object that is tied with the FFT core.

//
//#define FFT_256		// 256pt FFT bitstream.
#define FFT_512	// 512pt FFT bitstream.

void shiftBits(volatile u64* bufferToShift, volatile u64* bufferToStoreIn) {

	// Some easy constants to adjust.
	const uint POINT_SIZE = 512;
	const uint BITS_TO_SHIFT = 0;


	u64 temp[POINT_SIZE];

	for (int i=0; i < POINT_SIZE; i++) {
		temp[i] = (bufferToShift[i] << BITS_TO_SHIFT) & 0xFFFF; // the LSB part
		temp[i] = (((bufferToShift[i] & 0xFFFF0000) << BITS_TO_SHIFT) & 0xFFFF0000) | temp[i]; // concatenating as we go
		temp[i] = (((bufferToShift[i] & 0xFFFF00000000) << BITS_TO_SHIFT) & 0xFFFF00000000) | temp[i];
		//for the last one do we need to & it again ? i don't think so lol
		temp[i] = ((bufferToShift[i] & 0xFFFF000000000000) << BITS_TO_SHIFT) | temp[i];

		if (i==0)
			bufferToStoreIn[i] = temp[i];
		else {
			bufferToStoreIn[POINT_SIZE-i] = temp[i];
		}
	}
	// now need to swap the order  of the bits



}

void shiftBitsRight(volatile u64* bufferToShift, volatile u64* bufferToStoreIn) {

	// Some easy constants to adjust.
	const uint POINT_SIZE = 64;
	const uint BITS_TO_SHIFT = 5;


	u64 temp[POINT_SIZE];

	for (int i=0; i < POINT_SIZE; i++) {
		temp[i] = (bufferToShift[i] >> BITS_TO_SHIFT) & 0xFFFF000000000000;
		temp[i] = (((bufferToShift[i] & 0xFFFF00000000) >> BITS_TO_SHIFT) & 0xFFFF00000000) | temp[i];
		temp[i] = (((bufferToShift[i] & 0xFFFF0000) >> BITS_TO_SHIFT) & 0xFFFF0000) | temp[i]; // concatenating as we go
		temp[i] = ((bufferToShift[i] & 0xFFFF ) >> BITS_TO_SHIFT) | temp[i]; // the LSB part

		if (i==0)
			bufferToStoreIn[i] = temp[i];
		else {
			bufferToStoreIn[POINT_SIZE-i] = temp[i];
		}
	}
	// now need to swap the order  of the bits



}

// This function does the following:
// - Initializes the DMA.
// - Populates DDR with a test vector.
// - Does a data transfer to and from the FFT core via the DMA
//   to perform a forward FFT.
int XAxiDma_FFTDataTransfer(u16 DeviceId, volatile u64* inputBuffer, volatile u64* outputBuffer) {


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

	Status = XAxiDma_CfgInitialize(&axiDmaFFT, CfgPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("Initialization failed %d\r\n", Status);
		return XST_FAILURE;
	}

	if(XAxiDma_HasSg(&axiDmaFFT)){
		xil_printf("Device configured as SG mode \r\n");
		return XST_FAILURE;
	}

	/* Disable interrupts, we use polling mode
	 */
	XAxiDma_IntrDisable(&axiDmaFFT, XAXIDMA_IRQ_ALL_MASK,
						XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrDisable(&axiDmaFFT, XAXIDMA_IRQ_ALL_MASK,
						XAXIDMA_DMA_TO_DEVICE);

	//Value = TEST_START_VALUE;


	// flush the cache
	Xil_DCacheFlushRange((UINTPTR)inputBuffer, 0x1000);
	//#ifdef __aarch64__
	Xil_DCacheFlushRange((UINTPTR)outputBuffer, 0x1000);
	//#endif

	/**********************Start data transfer with FFT***************************/
	//
	Status = XAxiDma_SimpleTransfer(&axiDmaFFT,(UINTPTR) outputBuffer,
				0x1000, XAXIDMA_DEVICE_TO_DMA);

	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	Status = XAxiDma_SimpleTransfer(&axiDmaFFT,(UINTPTR) inputBuffer,
			0x1000, XAXIDMA_DMA_TO_DEVICE);

	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	// loop while DMA is busy
	while ((XAxiDma_Busy(&axiDmaFFT,XAXIDMA_DEVICE_TO_DMA)) ||
		(XAxiDma_Busy(&axiDmaFFT,XAXIDMA_DMA_TO_DEVICE))) {
			/* Wait */
	}

	return 0;
}

int XAxiDma_MixerDataTransfer(u16 DeviceId, volatile u32* inputBuffer, volatile u32* outputBuffer, XAxiDma axiDma, int8_t bothDirection){

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

	if (bothDirection == 1)
		XAxiDma_IntrDisable(&axiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);

	//Value = TEST_START_VALUE;


	// flush the cache
	Xil_DCacheFlushRange((UINTPTR)inputBuffer, 0x400);
	//#ifdef __aarch64__
	if (bothDirection == 1)
		Xil_DCacheFlushRange((UINTPTR)outputBuffer, 0x400);
	//#endif

	/**********************Start data transfer with FFT***************************/
	//
	if (bothDirection == 1){
		Status = XAxiDma_SimpleTransfer(&axiDma,(UINTPTR) outputBuffer,
					0x400, XAXIDMA_DEVICE_TO_DMA);

		if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	}
	Status = XAxiDma_SimpleTransfer(&axiDma,(UINTPTR) inputBuffer,
			0x400, XAXIDMA_DMA_TO_DEVICE);

	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	// loop while DMA is busy
	if (bothDirection == 1)
		while ((XAxiDma_Busy(&axiDma,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&axiDma,XAXIDMA_DMA_TO_DEVICE))) {
				/* Wait */
		}
	else
		while (XAxiDma_Busy(&axiDma,XAXIDMA_DEVICE_TO_DMA)){
			/* Wait */
		}

	return 0;
}


// This function sets up the FFT core


u64 scalingSchedule256 = 0b0101010100000000;
u64 scalingSchedule512 = 0b010101010000000000;

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
	// 64 point FFT:
	// shift 1 bit at each stage: 0x1555557


	//u32 configData = 0b11 | scalingSchedule << 2 | scalingSchedule << 16; // 128pt FFT
	//XGpio_DiscreteWrite(&gpioFftConfig, 1, 0b11);
	// send valid signal
#ifdef FFT_256
	u32 configData = 0b11 | scalingSchedule256 << 2 | scalingSchedule256 << 18; // 256pt FFT
	XGpio_DiscreteWrite(&gpioFftConfig, 1, configData);
	// concatenated to the config
	XGpio_DiscreteWrite(&gpioFftConfig, 2, 0b100000001);
#endif // FFT_256

#ifdef FFT_512
	u32 configData = 0b11 | scalingSchedule512 << 2 | scalingSchedule512 << 20; // 256pt FFT
	XGpio_DiscreteWrite(&gpioFftConfig, 1, configData);
	// concatenated to the config
	XGpio_DiscreteWrite(&gpioFftConfig, 2, 0b100010101);
#endif // FFT_512

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
	// 64 point FFT:
	// shift 1 bit at each stage: 0x1555554

	//u32 configData = 0b00 | scalingSchedule << 2 | scalingSchedule << 16; // 128pt FFT
	// send valid signal
	//XGpio_DiscreteWrite(&gpioFftConfig, 1, 0b11);
#ifdef FFT_256
	u32 configData = 0b00 | scalingSchedule256 << 2 | scalingSchedule256 << 18; // 256pt FFT
	XGpio_DiscreteWrite(&gpioFftConfig, 1, configData);

	XGpio_DiscreteWrite(&gpioFftConfig, 2, 0b100000001);
#endif // FFT_256

#ifdef FFT_512
	u32 configData = 0b00 | scalingSchedule512 << 2 | scalingSchedule512 << 20; // 256pt FFT
	XGpio_DiscreteWrite(&gpioFftConfig, 1, configData);

	XGpio_DiscreteWrite(&gpioFftConfig, 2, 0b100010101);
#endif // FFT_512
	return 0;
}
