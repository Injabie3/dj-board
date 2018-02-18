// Interrupt Handlers
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

#ifndef SRC_LUIINTERRUPTS_H_
#define SRC_LUIINTERRUPTS_H_

// This sets up the interrupt controller that is on the PS for the GPIO supplied.
// Parameters:
// - interruptController: 	A pointer to the interrupt controller.
// - gpio:					A pointer to the GPIO instance to associate with the interrupt controller.
// - interruptID			The interrupt ID.
// - interruptHandler		The function you want to execute when an interrupt from the GPIO occurs.
// Returns:
// - XST_SUCCESS, if the interrupt system was set up successfully!
// - XST_FAILURE, if the interrupt system could not be set up.
//int setupInterruptSystemXIntc(XIntc* interruptController, XGpio* gpio, u32 interruptID, Xil_ExceptionHandler interruptHandler);

// This sets up the interrupt controller that is on the PS for the GPIO supplied.
// Parameters:
// - interruptController: 	A pointer to the interrupt controller.
// - gpio:					A pointer to the GPIO instance to associate with the interrupt controller.
// - interruptID			The interrupt ID.
// - interruptHandler		The function you want to execute when an interrupt from the GPIO occurs.
// Returns:
// - XST_SUCCESS, if the interrupt system was set up successfully!
// - XST_FAILURE, if the interrupt system could not be set up.
int setupInterruptSystemXScuGic(XScuGic* interruptController, XGpio* gpio, int interruptID, Xil_ExceptionHandler interruptHandler);

// Registers the interrupt handler in the vector table, and enables IRQ interrupts in the ARM processor.
void registerInterruptHandler(XScuGic* interruptController);

// This function is what will get called when an interrupt occurs from the switches
void gpioSwitchesInterruptHandler(void *CallbackRef);

// This function is what will get called when an interrupt occurs from the 5 push buttons
void gpioPushButtonsInterruptHandler(void *CallbackRef);

#endif /* SRC_LUIMEMORYLOCATIONS_H_ */
