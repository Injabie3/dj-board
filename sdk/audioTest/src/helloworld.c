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
#include "xgpio.h" // GPIO drivers
#include "xil_printf.h"
//#include "xintc.h"
#include "xscugic.h" // Interrupt controller drivers.
#include "Xil_exception.h"
#include "luiInterrupts.h" // Contains interrupt-related setup
#include "luiMemoryLocations.h"


// Here we have some defines, pretty self-explanatory here.
volatile u32* AUDIOCHIP = ((volatile u32*)XPAR_AUDIOINOUT16_0_S00_AXI_BASEADDR);
#define MIDDLEC_ADDRESS 				XPAR_AXI_GPIO_MIDDLEC_BASEADDR
#define DEVICE_ID_MIDDLEC 				XPAR_AXI_GPIO_MIDDLEC_DEVICE_ID
#define DEVICE_ID_SWITCHES 				XPAR_AXI_GPIO_SWITCHES_DEVICE_ID
#define DEVICE_ID_PUSHBUTTONS			XPAR_AXI_GPIO_BUTTONS_DEVICE_ID
#define DEVICE_ID_INTERRUPTCONTROLLER	XPAR_INTC_0_DEVICE_ID
//#define INTERRUPTCONTROLLER_ADDRESS		XPAR_AXI_INTC_0_BASEADDR

static XGpio gpioMiddleC; 		// AXI GPIO object for the middle C note.
static XGpio gpioSwitches;		// AXI GPIO object for the switches
static XGpio gpioPushButtons; 	// AXI GPIO object for the push buttons
//XIntc interruptController; // AXI Interrupt Controller object.
static XScuGic_Config *interruptControllerConfig;
static XScuGic psInterruptController;

int switches;

// Initialize GPIO peripherals and the PS interrupt controller
int initializePeripherals();

// A test audio loop.
void audioLoop();

int main()
{
    init_platform();

    initializePeripherals();

    //setupInterruptSystemXIntc(&interruptController, &gpioSwitches, XPAR_INTC_0_GPIO_1_VEC_ID, gpioSwitchesInterruptHandler);
	// Set up interrupt handler for switches.
    setupInterruptSystemXScuGic(&psInterruptController, &gpioSwitches, XPAR_FABRIC_AXI_GPIO_SWITCHES_IP2INTC_IRPT_INTR, gpioSwitchesInterruptHandler);

    // Set up interrupt handler for buttons.
	setupInterruptSystemXScuGic(&psInterruptController, &gpioPushButtons, XPAR_FABRIC_AXI_GPIO_BUTTONS_IP2INTC_IRPT_INTR, gpioPushButtonsInterruptHandler);

	// Enable the interrupts, and away we go!
	registerInterruptHandler(&psInterruptController);

    print("Hello World\n\r");
    AUDIOCHIP[0] = 3; // Reset FIFOs.

    audioLoop();

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

	status = XScuGic_CfgInitialize(&psInterruptController, interruptControllerConfig, interruptControllerConfig->CpuBaseAddress);
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

	return status;
}

void audioLoop() {
	int tone = 0;
	u32* audioChannels = (u32 *) LUI_MEM_AUDIO_CHANNELS;
	while (1) {
		// WAIT FOR DAC FIFO TO BE EMPTY.

		if ((AUDIOCHIP[0] & 1<<3)!=0) {
			// Transmit Line-In to HP Out.
			if (*audioChannels == 0b001 || switches == 0xC1) {
				AUDIOCHIP[1] = AUDIOCHIP[2];
			}
			else if (switches == 0x3) {
				// Swap left and right channels.
				AUDIOCHIP[1] = AUDIOCHIP[2] >> 16 | AUDIOCHIP[2] << 16;
			}
			else if (switches == 0x7) {
				// Overlay middle C
				XGpio_SetDataDirection(&gpioMiddleC, 1, 0xFFFF); // Set as input
				tone =  XGpio_DiscreteRead(&gpioMiddleC, 1);
				AUDIOCHIP[1] = AUDIOCHIP[2] + (tone >> 6 | tone << 10);
			}
			else if (*audioChannels == 0b010) {
				// Output only right channel
				AUDIOCHIP[1] = AUDIOCHIP[2] & 0xFFFF0000;
			}
			else if (*audioChannels == 0b100) {
				// Output only left channel
				AUDIOCHIP[1] = AUDIOCHIP[2] & 0xFFFF;
			}
			else {
				XGpio_SetDataDirection(&gpioMiddleC, 1, 0xFFFF); // Set as input
				tone =  XGpio_DiscreteRead(&gpioMiddleC, 1);
				AUDIOCHIP[1] = tone >> 5  | tone << 11; // Send middle C note to headphone out.  We shift 16 bits to cover the right channel as well.

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
}
