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
#include "xgpio.h" 		// GPIO drivers, PL side
#include "xgpiops.h"	// GPIO drivers, PS side.
#include "xscutimer.h"	// Timer drivers, PS side.
#include "xil_printf.h"
//#include "xintc.h"	// AXI interrupt controller, if we use it.
#include "xscugic.h"	// PS interrupt controller.
#include "Xil_exception.h"
#include "luiMemoryLocations.h"

// Global variable
u8 ignoreButtonPress;

// ##############################
// # INTERRUPT SET UP FUNCTIONS #
// ##############################

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
int setupInterruptSystemGpio(XScuGic* interruptController, XGpio* gpio, u32 interruptID, Xil_ExceptionHandler interruptHandler) {
	int status;

	// This enables the interrupts to be detected in the GPIO.  The mask tells it which interrupts to enable.
	XGpio_InterruptEnable(gpio, 0xFFFF);

	// This enables the GPIO to output the interrupts to the interrupt controller.
	XGpio_InterruptGlobalEnable(gpio);


	// Now that the GPIO interrupt is configured, connect it to the interrupt controller, and the handler.
	XScuGic_Disable(interruptController, interruptID);

	XScuGic_SetPriorityTriggerType(interruptController, interruptID, 0xA0, 0b01);

	// Hook up the GPIO Switch interrupt to the interrupt controller.
	status = XScuGic_Connect(interruptController, interruptID, interruptHandler, gpio);
	if (status != XST_SUCCESS) {
		xil_printf("Error: Could not connect GPIO Switch interrupt to interrupt controller!\r\n");
		return XST_FAILURE;
	}

	// Enable the GPIO Switch interrupt
	XScuGic_Enable(interruptController, interruptID);

	return status;

}

// This sets up the interrupt controller that is on the PS for the GPIO supplied (for GPIO on the PS).
// Parameters:
// - interruptController: 	A pointer to the interrupt controller.
// - gpio:					A pointer to the PS GPIO instance to associate with the interrupt controller.
// - interruptID			The interrupt ID.
// - pin					The pin that you want to enable interrupts for.
// - interruptHandler		The function you want to execute when an interrupt from the GPIO occurs.
// Returns:
// - XST_SUCCESS, if the interrupt system was set up successfully!
// - XST_FAILURE, if the interrupt system could not be set up.
int setupInterruptSystemGpioPs(XScuGic* interruptController, XGpioPs* gpio, int interruptID, int pin, Xil_ExceptionHandler interruptHandler) {
	int status;

	// This enables the interrupts to be detected in the GPIO.  The mask tells it which interrupts to enable.
	/* Set the handler for gpio interrupts. */
	XGpioPs_SetCallbackHandler(gpio, (void *)gpio,(XGpioPs_Handler) interruptHandler);

	// Set pin to be input.
	XGpioPs_SetOutputEnablePin(gpio, pin, 0x0);

	// Enable the GPIO interrupt for the pin to be on rising edge.
	XGpioPs_SetIntrTypePin(gpio, pin, XGPIOPS_IRQ_TYPE_EDGE_RISING);
	XGpioPs_IntrEnablePin(gpio, pin);


	// Now that the GPIO interrupt is configured, connect it to the interrupt controller, and the handler.
	XScuGic_Disable(interruptController, interruptID);

	XScuGic_SetPriorityTriggerType(interruptController, interruptID, 0xA0, 0b01);

	// Hook up the GPIO Switch interrupt to the interrupt controller.
	status = XScuGic_Connect(interruptController, interruptID, interruptHandler, gpio);
	if (status != XST_SUCCESS) {
		xil_printf("Error: Could not connect GPIO Switch interrupt to interrupt controller!\r\n");
		return XST_FAILURE;
	}

	// Enable the GPIO Switch interrupt
	XScuGic_Enable(interruptController, interruptID);

	// This enables the GPIO to output the interrupts to the interrupt controller.

	return status;
}

// This sets up the interrupt controller that is on the PS for the timer supplied (for timer on the PS).
// Parameters:
// - interruptController: 	A pointer to the interrupt controller.
// - timer:					A pointer to the PS Timer instance to associate with the interrupt controller.
// - interruptID			The interrupt ID.
// - interruptHandler		The function you want to execute when an interrupt from the GPIO occurs.
// Returns:
// - XST_SUCCESS, if the interrupt system was set up successfully!
// - XST_FAILURE, if the interrupt system could not be set up.
int setupInterruptSystemTimerPs(XScuGic* interruptController, XScuTimer* timer, int interruptID, Xil_ExceptionHandler interruptHandler) {
	int status;

	XScuTimer_EnableInterrupt(timer);

	// Now that the Timer interrupt is configured, connect it to the interrupt controller, and the handler.
	XScuGic_Disable(interruptController, interruptID);

	XScuGic_SetPriorityTriggerType(interruptController, interruptID, 0xA0, 0b01);

	// Hook up the GPIO Switch interrupt to the interrupt controller.
	status = XScuGic_Connect(interruptController, interruptID, interruptHandler, timer);
	if (status != XST_SUCCESS) {
		xil_printf("Error: Could not connect GPIO Switch interrupt to interrupt controller!\r\n");
		return XST_FAILURE;
	}

	// Enable the GPIO Switch interrupt
	XScuGic_Enable(interruptController, interruptID);
	return 0;
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


// #######################################
// # INTERRUPT HANDLERS/SERVICE ROUTINES #
// #######################################

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

// This function is what will get called when an interrupt occurs from the 5 push buttons on the PL side.
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
		print("PL button pressed!\n\r");
	}
	else {
		print("PL button released!\n\r");
	}
	// Clear the interrupt!
	XGpio_InterruptClear(gpioPointer, 1);
	return;
}

// This function is what will get called when an interrupt occurs from the 2 push buttons on the PS side.
// TODO Find a way to be able to reference the timer in this interrupt handler.
void gpioPushButtonsPSInterruptHandler(void *CallbackRef) {
	XGpioPs* gpio = (XGpioPs*) CallbackRef;

//	if (ignoreButtonPress == 0) {
//		ignoreButtonPress = 1;
		print("PS Button pressed!\n\r");
//		XScuTimer_LoadTimer(&psTimer, 5000);
//	}


	// Clear the interrupt!
	// TODO: Remove the hardcoded MIO pins.
	XGpioPs_IntrClearPin(gpio, 50);
	XGpioPs_IntrClearPin(gpio, 51);

	return;
}

// This function is what will get called when the timer interrupt occurs.
void timerInterruptHandler(void *CallbackRef) {
	XScuTimer* timer = (XScuTimer *) CallbackRef;

	if (ignoreButtonPress == 0) {
		ignoreButtonPress = 1;

	}
	// Stub function for now.

	// Clear the interrupt!
	XScuTimer_ClearInterruptStatus(timer);
	return;
}
