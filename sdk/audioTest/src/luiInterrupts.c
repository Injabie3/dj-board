// Interrupt Handlers
// Description:
//
// Ryan Lui - rclui@sfu.ca
// 301251951
//
// Changelog:
// 2018-02-16 - Initial Version.

#include <stdio.h>
#include "xparameters.h"
#include "platform.h"
#include "xgpio.h" 		// GPIO drivers
#include "xil_printf.h"
//#include "xintc.h"	// AXI interrupt controller, if we use it.
#include "xscugic.h"	// PS interrupt controller.
#include "Xil_exception.h"
#include "luiMemoryLocations.h"

// The function below does not work!
// This sets up the interrupt controller that is on the PS for the GPIO supplied.
// Parameters:
// - interruptController: 	A pointer to the interrupt controller.
// - gpio:					A pointer to the GPIO instance to associate with the interrupt controller.
// - interruptID			The interrupt ID.
// - interruptHandler		The function you want to execute when an interrupt from the GPIO occurs.
// Returns:
// - XST_SUCCESS, if the interrupt system was set up successfully!
// - XST_FAILURE, if the interrupt system could not be set up.
//int setupInterruptSystemXIntc(XIntc* interruptController, XGpio* gpio, u32 interruptID, Xil_ExceptionHandler interruptHandler) {
//	int status;
//	// Hook up the GPIO Switch interrupt to the interrupt controller.
//	status = XIntc_Connect(interruptController, interruptID, (Xil_ExceptionHandler)interruptHandler, gpio);
//	//status = XScuGic_Connect(&interruptController, 1, (Xil_ExceptionHandler)gpioSwitchesInterruptHandler, &gpioSwitches);
//	if (status != XST_SUCCESS) {
//		xil_printf("Error: Could not connect GPIO interrupt to interrupt controller!\r\n");
//		return XST_FAILURE;
//	}
//
//	// Enable the GPIO Switch interrupt
//	XIntc_Enable(interruptController, interruptID);
//	//XScuGic_Enable(&interruptController, 1);
//
//	// Start the AXI Interrupt Controller slave.
//	status = XIntc_Start(interruptController, XIN_REAL_MODE);
//	if (status != XST_SUCCESS) {
//		return status;
//	}
//
//
//	Xil_ExceptionInit();
//
//	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XIntc_InterruptHandler, interruptController);
//
//	/* Enable non-critical exceptions */
//	Xil_ExceptionEnable();
//
//
//
//	XGpio_InterruptEnable(gpio, 0xFFFF);
//	XGpio_InterruptGlobalEnable(gpio);
//
//	return status;
//}

// This sets up the interrupt controller that is on the PS for the GPIO supplied.
// Parameters:
// - interruptController: 	A pointer to the interrupt controller.
// - gpio:					A pointer to the GPIO instance to associate with the interrupt controller.
// - interruptID			The interrupt ID.
// - interruptHandler		The function you want to execute when an interrupt from the GPIO occurs.
// Returns:
// - XST_SUCCESS, if the interrupt system was set up successfully!
// - XST_FAILURE, if the interrupt system could not be set up.
int setupInterruptSystemXScuGic(XScuGic* interruptController, XGpio* gpio, u32 interruptID, Xil_ExceptionHandler interruptHandler) {
	int status;

	XScuGic_SetPriorityTriggerType(interruptController, interruptID, 0xA0, 0b01);
	// Hook up the GPIO Switch interrupt to the interrupt controller.
	status = XScuGic_Connect(interruptController, interruptID, interruptHandler, gpio);
	if (status != XST_SUCCESS) {
		xil_printf("Error: Could not connect GPIO Switch interrupt to interrupt controller!\r\n");
		return XST_FAILURE;
	}

	// Enable the GPIO Switch interrupt
	XScuGic_Enable(interruptController, interruptID);

	// This enables the interrupts to be detected in the GPIO.  The mask tells it which interrupts to enable.
	XGpio_InterruptEnable(gpio, 0xFFFF);

	// This enables the GPIO to output the interrupts to the interrupt controller.
	XGpio_InterruptGlobalEnable(gpio);

	return status;

}

// Registers the interrupt handler in the vector table, and enables IRQ interrupts in the ARM processor.
void registerInterruptHandler(XScuGic* interruptController) {
	// Initialize the exception table in the PS
	Xil_ExceptionInit();

	// Register the interrupt handler in the vector table.
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, interruptController);

	// Enable the IRQ
	Xil_ExceptionEnable();
}

// This function is what will get called when an interrupt occurs from the switches
void gpioSwitchesInterruptHandler(void *CallbackRef) {
	XGpio *gpioPointer = (XGpio *) CallbackRef;
	int switches = XGpio_DiscreteRead(gpioPointer, 1);
	int switchVar = switches;
	u32* audioChannels = (u32 *) LUI_MEM_AUDIO_CHANNELS;

	*audioChannels = (switches >> 5) & 0b111;

	int switchNum = 0;
	while(switchVar != 0) {

		if ((0x1 & switchVar) == 1) {
			xil_printf("Switch %d is up!\n\r", switchNum);
		}
		switchNum++;
		switchVar >>= 1;
	}

	// Clear the interrupt!!
	XGpio_InterruptClear(gpioPointer, 1);
	//XIntc_Acknowledge(&interruptController, 0);
	print("Success! Switch interrupts work!\r\n");
}


// This function is what will get called when an interrupt occurs from the 5 push buttons
void gpioPushButtonsInterruptHandler(void *CallbackRef) {
	XGpio *gpioPointer = (XGpio *) CallbackRef;

	// Get the buttons that are pushed down.
	u32 buttonStatus = XGpio_DiscreteRead(gpioPointer, 1);

	// From the constraints XDC in Vivado, we have set it as follows:
	// Bit 0: BTNC
	// Bit 1: BTND
	// Bit 2: BTNL
	// Bit 3: BTNR
	// Bit 4: BTNU
	if ((buttonStatus & 0b11111) != 0x0) {
		print("Button pressed!\n\r");
	}
	else {
		print("Button released!\n\r");
	}
	// Clear the interrupt!
	XGpio_InterruptClear(gpioPointer, 1);
	return;
}
