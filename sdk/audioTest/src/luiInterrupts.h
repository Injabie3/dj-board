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


// ##############################
// # INTERRUPT SET UP FUNCTIONS #
// ##############################

// This sets up the interrupt controller that is on the PS for the GPIO supplied.
// Parameters:
// - interruptController: 	A pointer to the interrupt controller.
// - gpio:					A pointer to the GPIO instance to associate with the interrupt controller.
// - interruptID			The interrupt ID.
// - interruptHandler		The function you want to execute when an interrupt from the GPIO occurs.
// Returns:
// - XST_SUCCESS, if the interrupt system was set up successfully!
// - XST_FAILURE, if the interrupt system could not be set up.
int setupInterruptSystemGpio(XScuGic* interruptController, XGpio* gpio, int interruptID, Xil_ExceptionHandler interruptHandler);

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
int setupInterruptSystemGpioPs(XScuGic* interruptController, XGpioPs* gpio, int interruptID, int pin, Xil_ExceptionHandler interruptHandler, int triggerType);

// This sets up the interrupt controller that is on the PS for the timer supplied (for timer on the PS).
// Parameters:
// - interruptController: 	A pointer to the interrupt controller.
// - timer:					A pointer to the PS Timer instance to associate with the interrupt controller.
// - interruptID			The interrupt ID.
// - interruptHandler		The function you want to execute when an interrupt from the GPIO occurs.
// Returns:
// - XST_SUCCESS, if the interrupt system was set up successfully!
// - XST_FAILURE, if the interrupt system could not be set up.
int setupInterruptSystemTimerPs(XScuGic* interruptController, XScuTimer* timer, int interruptID, Xil_ExceptionHandler interruptHandler);

// Registers the interrupt handler in the vector table, and enables IRQ interrupts in the ARM processor.
void registerInterruptHandler(XScuGic* interruptController);

void setUpInterruptCounters();

// #######################################
// # INTERRUPT HANDLERS/SERVICE ROUTINES #
// #######################################
// This function is what will get called when an interrupt occurs from the switches
void gpioSwitchesInterruptHandler(void *CallbackRef);

// This function is what will get called when an interrupt occurs from the 5 push buttons on the PL side.
void gpioPushButtonsInterruptHandler(void *CallbackRef);

// This function is what will get called when an interrupt occurs from the left push button on the PS side.
void gpioPushButtonsPSInterruptHandler(void *CallbackRef);

// This function is what will get called when an interrupt occurs from the right push button on the PS side.
//void gpioRightPushButtonPSInterruptHandler(void *CallbackRef);

// This function is what will get called when the timer interrupt occurs.
void timerInterruptHandler(void *CallbackRef);

// This function takes in the timer object pointer, and enables the PS push buttons to be debounced.
void interruptSetTimer(XScuTimer* timer);

// This function takes in the GPIO switches object pointer, and sets it to allow other interrupt handlers to reference it if needed.
void interruptSetGpioSwitches(XGpio* switches);

// This function takes in the GPIO PS switches object pointer, and sets it to allow other interrupt handlers to reference it if needed.
void interruptSetGpioPsPushButtons(XGpioPs* pushButtons);

#endif /* SRC_LUIINTERRUPTS_H_ */
