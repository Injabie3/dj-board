/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "xparameters.h"
#include "platform.h"
#include "xgpio.h" 					// GPIO drivers, PL side.
#include "xgpiops.h"				// GPIO drivers, PS side.
#include "xscutimer.h"				// Timer drivers.
#include "xil_printf.h"
//#include "xintc.h"
#include "xscugic.h" 				// Interrupt controller drivers.
#include "xaxidma.h" 				// DMA drivers.
#include "xil_exception.h"
#include "luiInterrupts.h" 			// Contains interrupt-related setup.
#include "luiMemoryLocations.h"		// Custom memory location mappings.
#include "luiCircularBuffer.h"		// Circular buffer.
#include "audioCodecCom.h"


// Here we have some defines, pretty self-explanatory here.

#define MIDDLEC_ADDRESS 				XPAR_AXI_GPIO_MIDDLEC_BASEADDR
#define DEVICE_ID_MIDDLEC 				XPAR_AXI_GPIO_MIDDLEC_DEVICE_ID
#define DEVICE_ID_SWITCHES 				XPAR_AXI_GPIO_SWITCHES_DEVICE_ID
#define DEVICE_ID_PUSHBUTTONS			XPAR_AXI_GPIO_BUTTONS_DEVICE_ID
#define DEVICE_ID_BEATDETECTOR01		XPAR_AXI_GPIO_BEATDETECTOR01_CONFIG_DEVICE_ID
#define DEVICE_ID_BEATDETECTOR02		XPAR_AXI_GPIO_BEATDETECTOR02_CONFIG_DEVICE_ID
#define DEVICE_ID_PS_PUSHBUTTONS		XPAR_XGPIOPS_0_DEVICE_ID
#define DEVICE_ID_FFT_GPIO				XPAR_AXI_GPIO_FFT_CONFIG_DEVICE_ID
#define DEVICE_ID_INTERRUPTCONTROLLER	XPAR_INTC_0_DEVICE_ID
#define DEVICE_ID_DMA					XPAR_AXIDMA_0_DEVICE_ID
#define DEVICE_ID_TIMER					XPAR_PS7_SCUTIMER_0_DEVICE_ID


//#define INTERRUPTCONTROLLER_ADDRESS		XPAR_AXI_INTC_0_BASEADDR

static XGpio gpioMiddleC; 					// AXI GPIO object for the middle C note.
static XGpio gpioSwitches;					// AXI GPIO object for the switches
static XGpio gpioPushButtons; 				// AXI GPIO object for the push buttons
static XGpio gpioBeatDetector01;			// AXI GPIO object for beat detector 1.
static XGpio gpioBeatDetector02;			// AXI GPIO object for beat detector 2.

//XIntc interruptController; 				// AXI Interrupt Controller object.
static XScuGic_Config *interruptControllerConfig;
static XScuGic * psInterruptController = (XScuGic *)PS_INTERRUPT_CONTROLLER;
static XGpioPs gpioPSPushButtons; 			// PS GPIO object for the push buttons.
static XGpioPs_Config *gpioPSPushButtonsConfig;
static XScuTimer psTimer;					// PS Timer.
static XScuTimer_Config *psTimerConfig;		// PS Timer config.
int switches;

// This function is responsible for getting data from audio FIFO 64 samples at a time
// stores 64 samples in memory then sends it to FFT, then to IFFT

// Initialize GPIO peripherals and the PS interrupt controller
int initializePeripherals();

// This function does the following:
// - Initializes the DMA.
// - Populates DDR with a test vector.
// - Does a data transfer to and from the FFT core via the DMA
//   to perform a forward FFT.
int XAxiDma_FftDataTransfer(u16 DeviceId, volatile u64* TxBuf, volatile u64* RxBuf);

//mirrors image
int XAxiDma_IFftDataTransfer(u16 DeviceId);

// This function sets up the FFT core with forward FFT.
int XGpio_FftConfig();

// This function configures the FFT core with inverse FFT
//To get back the original data
int XGpio_IFftConfig();

//This function is to shift the IFFT output back
void shiftBits(volatile u64* RxBuf);

// This function is a loop to test the audio with GPIO switches.
void audioLoop();

int main()
{
	int status;
    init_platform();
    XScuGic test;
    * psInterruptController = test;
    initializePeripherals();

    // Set up the beat detector to detect certain bins
    XGpio_DiscreteWrite(&gpioBeatDetector01, 1, 0x80000); // 2^18
    XGpio_DiscreteWrite(&gpioBeatDetector01, 2, 0b1000000000 | 0xA); // Valid and bin 20.

    XGpio_DiscreteWrite(&gpioBeatDetector02, 1, 0x800000); // 2^18
    XGpio_DiscreteWrite(&gpioBeatDetector02, 2, 0b1000000000 | 0x0); // Valid and bin 20.

    //setupInterruptSystemXIntc(&interruptController, &gpioSwitches, XPAR_INTC_0_GPIO_1_VEC_ID, gpioSwitchesInterruptHandler);
	// Set up interrupt handler for switches.
    setupInterruptSystemGpio(psInterruptController, &gpioSwitches, XPAR_FABRIC_AXI_GPIO_SWITCHES_IP2INTC_IRPT_INTR, gpioSwitchesInterruptHandler);

    // Set up interrupt handler for buttons.
	setupInterruptSystemGpio(psInterruptController, &gpioPushButtons, XPAR_FABRIC_AXI_GPIO_BUTTONS_IP2INTC_IRPT_INTR, gpioPushButtonsInterruptHandler);

	// Set up interrupt handler for PS buttons.
	setupInterruptSystemGpioPs(psInterruptController, &gpioPSPushButtons, XPAR_XGPIOPS_0_INTR, 50, gpioPushButtonsPSInterruptHandler);
	setupInterruptSystemGpioPs(psInterruptController, &gpioPSPushButtons, XPAR_XGPIOPS_0_INTR, 51, gpioPushButtonsPSInterruptHandler);

	// Set up interrupt handler for PS Timer
//	setupInterruptSystemTimerPs(psInterruptController, &psTimer, XPAR_SCUTIMER_INTR, timerInterruptHandler);
//	interruptSetTimer(&psTimer);
	interruptSetGpioPsPushButtons(&gpioPSPushButtons);

	// Enable the interrupts, and away we go!
	registerInterruptHandler(psInterruptController);
	setUpInterruptCounters();

	XScuTimer_LoadTimer(&psTimer, 5000);
	XScuTimer_Start(&psTimer);

    print("Hello World from CPU 0\n\r");

    // test data
	/*TxBufferPtr[0] = 0x0000000000004000;
	TxBufferPtr[1] = 0x00000000c6fae2f2;
	TxBufferPtr[2] = 0x0000000033c7da62;
	TxBufferPtr[3] = 0x000000000a033f36;
	TxBufferPtr[4] = 0x00000000c322ec39;
	TxBufferPtr[5] = 0x000000002d41d2bf;
	TxBufferPtr[6] = 0x0000000013c73cde;
	TxBufferPtr[7] = 0x00000000c0caf5fd;*/

	audioDriver(); // all the ADC/DAC read/write and FFT stuff taken out of here


	xil_printf("Successfully ran XAxiDma_SimplePoll Example\r\n");

//	audioLoop();

	cleanup_platform();
	return 0;
}

// Initialize GPIO peripherals and the PS interrupt controller
int initializePeripherals() {
	int status;
	// Initialize the Interrupt Controller
//	status = XIntc_Initialize(&interruptController, DEVICE_ID_INTERRUPTCONTROLLER);
//	if (status != XST_SUCCESS) {
//		xil_printf("Error: Interrupt controller initialization failed!\r\n");
//		return XST_FAILURE;
//	}

	interruptControllerConfig = XScuGic_LookupConfig(XPAR_SCUGIC_0_DEVICE_ID);
	if (NULL == interruptControllerConfig) {
		return XST_FAILURE;
	}

	status = XScuGic_CfgInitialize(psInterruptController, interruptControllerConfig, interruptControllerConfig->CpuBaseAddress);
	if (status != XST_SUCCESS) {
		xil_printf("Error: Interrupt controller initialization failed!\r\n");
		return XST_FAILURE;
	}

	//status = XIntc_SelfTest(&interruptController);
	if (status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	// Initialize AXI GPIO slave that is connected to my custom middle C note.
	status = XGpio_Initialize(&gpioMiddleC, DEVICE_ID_MIDDLEC);
	if (status != XST_SUCCESS) {
		xil_printf("Error: GPIO Middle C initialization failed!\r\n");
		return XST_FAILURE;
	}

	// Initialize AXI GPIO slave that is connected to the switches.
	status = XGpio_Initialize(&gpioSwitches, DEVICE_ID_SWITCHES);
	if (status != XST_SUCCESS) {
		xil_printf("Gpio Switches Initialization Failed\r\n");
		return XST_FAILURE;
	}

	// Initialize AXI GPIO slave that is connected to the push buttons.
	status = XGpio_Initialize(&gpioPushButtons, DEVICE_ID_PUSHBUTTONS);
	if (status != XST_SUCCESS) {
		xil_printf("Gpio Push Button Initialization Failed\r\n");
		return XST_FAILURE;
	}

	// Initialize AXI GPIO slave that is connected to custom beat detector 01.
	status = XGpio_Initialize(&gpioBeatDetector01, DEVICE_ID_BEATDETECTOR01);
	if (status != XST_SUCCESS) {
		xil_printf("Gpio Beat Detector Initialization Failed\r\n");
		return XST_FAILURE;
	}

	// Initialize AXI GPIO slave that is connected to custom beat detector 02.
	status = XGpio_Initialize(&gpioBeatDetector02, DEVICE_ID_BEATDETECTOR02);
	if (status != XST_SUCCESS) {
		xil_printf("Gpio Beat Detector Initialization Failed\r\n");
		return XST_FAILURE;
	}

	// Initialize PS GPIO.
	gpioPSPushButtonsConfig = XGpioPs_LookupConfig(DEVICE_ID_PS_PUSHBUTTONS);
	if (gpioPSPushButtonsConfig == NULL) {
		xil_printf("Could not find config for PS GPIO!\r\n");
		return XST_FAILURE;
	}

	status = XGpioPs_CfgInitialize(&gpioPSPushButtons, gpioPSPushButtonsConfig, gpioPSPushButtonsConfig->BaseAddr);
	if (status != XST_SUCCESS) {
		xil_printf("Gpio PS Push Button Initialization Failed\r\n");
		return XST_FAILURE;
	}

	// Initialize PS Timer
	psTimerConfig = XScuTimer_LookupConfig(DEVICE_ID_TIMER);
	if (psTimerConfig == NULL) {
		xil_printf("Could not find config for PS Timer!\r\n");
		return XST_FAILURE;
	}

	status = XScuTimer_CfgInitialize(&psTimer, psTimerConfig, psTimerConfig->BaseAddr);
	if (status != XST_SUCCESS) {
		xil_printf("PS Timer Initialization Failed\r\n");
		return XST_FAILURE;
	}

	return status;
}

// This function is a loop to test the audio with GPIO switches.
void audioLoop() {
	int tone = 0;
	u32* plPushButtonEnabled = (u32 *) LUI_MEM_PL_PUSHBUTTONS;
	circular_buf_t circularBuffer;
	circularBuffer.size = 48000*3;
	circularBuffer.buffer = (uint32_t*) CIRCULAR_BUFFER_BASE;
	circular_buf_reset(&circularBuffer);
	circularBuffer.startingIndex = 24000;

	volatile u32* AUDIOCHIP = ((volatile u32*)XPAR_AUDIOINOUT16_0_S00_AXI_BASEADDR);
	for (int index = 0; index < 48000; ) {

		// Wait for ADC FIFO to not be empty.
		if ((AUDIOCHIP[0] & 1 << 2) == 0) {
			circular_buf_put(&circularBuffer, AUDIOCHIP[2]);
			index++;
		}
	}

	while (1) {
		// WAIT FOR DAC FIFO TO BE EMPTY.
		int* echoCounter = (int *) ECHO_CNTR_LOCATION;
		int echoAmount = *echoCounter;
		if ((AUDIOCHIP[0] & 1<<3)!=0) {
			if (echoAmount == 0)
				circular_buf_get(&circularBuffer, &AUDIOCHIP[1]);
			else
				circular_buf_getSummedTaps(&circularBuffer, &AUDIOCHIP[1], echoAmount*120);
			circular_buf_put(&circularBuffer, AUDIOCHIP[2]);
			//AUDIOCHIP[1] = AUDIOCHIP[2];

		}

		XGpio_SetDataDirection(&gpioMiddleC, 1, 0x0); // Set as output
		// Set increment bit of middle C core.
		XGpio_DiscreteWrite(&gpioMiddleC, 1, 0x1);
		// Channel 2 controls the "clock", I toggle it here to "increment" it.
		// This is for demonstration purposes, since I highly doubt we want the PS to toggle "clocks" for us.
		XGpio_DiscreteWrite(&gpioMiddleC, 2, 0x0);
		XGpio_DiscreteWrite(&gpioMiddleC, 2, 0x1);

	}
}
