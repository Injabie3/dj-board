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

// Uncomment the following line for UART debug code.
//#define LUI_DEBUG

// ####################
// # GLOBAL VARIABLES #
// ####################
int ignoreButtonPress = 0;
int currentButtonState = 0;
XScuTimer* timerPointer;
XGpioPs* gpioPsPushButtons;
XGpio* gpioPushButtons;
XGpio* gpioSwitches;
int* pitchCounter;
int* echoCounter;
int* switchUpEcho = (int*) SWITCH_UP_ECHO;


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

void setUpInterruptCounters() {
	pitchCounter = (int*) PITCH_CNTR_LOCATION;
	echoCounter = (int*) ECHO_CNTR_LOCATION;
	*pitchCounter = 0;
	*echoCounter = 0;
}

// #######################################
// # INTERRUPT HANDLERS/SERVICE ROUTINES #
// #######################################

// This function is what will get called when an interrupt occurs from the switches
void gpioSwitchesInterruptHandler(void *CallbackRef) {
	XGpio *gpioPointer = (XGpio *) CallbackRef;
	int switches = XGpio_DiscreteRead(gpioPointer, 1);
	int switchVar = switches;


	int switchNum = 0;
	if ((switches & (1 << 4)) != 0) {
		*switchUpEcho = 1;
	}
	else {
		*switchUpEcho = 0;
	}

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
	print("Success! PL Switch interrupts work!\r\n");
}

// This function is what will get called when an interrupt occurs from the 5 push buttons on the PL side.
void gpioPushButtonsInterruptHandler(void *CallbackRef) {
	XGpio *gpioPointer = (XGpio *) CallbackRef;
	u32* plPushButtonEnabled = (u32 *) LUI_MEM_PL_PUSHBUTTONS;

	// Get the buttons that are pushed down.
	u32 buttonStatus = XGpio_DiscreteRead(gpioPointer, 1);

	// From the constraints XDC in Vivado, we have set it as follows:
	// Bit 0: BTNC
	// Bit 1: BTND !!
	// Bit 2: BTNL
	// Bit 3: BTNR
	// Bit 4: BTNU !!
	// want to increase/decrease the pitch
	// need to check which button is pressed (up or down)
	// and increment counter based on that
	if ((buttonStatus & 0b11111) != 0x0) {

#ifdef LUI_DEBUG
		print("PL button pressed!\n\r");
#endif // LUI_DEBUG

		*plPushButtonEnabled = 1;

		// this tells us that the down button is pressed
		if ((buttonStatus & (1<<1)) != 0x0) {
			(*pitchCounter)--;
		}
		// this tells us that the UP button is pressed
		if ((buttonStatus & (1<<4)) != 0x0){
			// TODO Put an upper limit on this.
			(*pitchCounter)++;
		}
		// this tells us that the CTR button is pressed
		if ((buttonStatus & (1<<0)) != 0x0) {
			*pitchCounter = 0;
		}

		// Only adjust echo if SW4 is up.
		if ((*switchUpEcho) == 1) {
			// Do not let *echoCounter go less than 0
			if ((buttonStatus & (1<<1)) != 0x0 && (*echoCounter) > 0) {
				(*echoCounter)--;
			}
			// this tells us that the UP button is pressed
			if ((buttonStatus & (1<<4)) != 0x0){
				(*echoCounter)++;
			}
			// this tells us that the CTR button is pressed
			if ((buttonStatus & (1<<0)) != 0x0) {
				*echoCounter = 0;
			}
			// TODO Check to see if the below is dangerous.
			char buffer[50];
			sprintf(buffer, "Echo adjusted. Currently at %02d.\n\r", *echoCounter);
			print(&buffer);
		}
	}
	else {
#ifdef LUI_DEBUG
		print("PL button released!\n\r");
#endif // LUI_DEBUG
		//*plPushButtonEnabled = 0;
	}

#ifdef LUI_DEBUG
	print("Success! PL Push Button interrupts work!\r\n");
#endif // LUI_DEBUG
	// Clear the interrupt!
	XGpio_InterruptClear(gpioPointer, 1);
	return;
}

// This function is what will get called when an interrupt occurs from the 2 push buttons on the PS side.
void gpioPushButtonsPSInterruptHandler(void *CallbackRef) {
	XGpioPs* gpio = (XGpioPs*) CallbackRef;
	u32* psPushButtonEnabled = (u32 *) LUI_MEM_PS_PUSHBUTTONS;
	u32 leftButton = XGpioPs_ReadPin(gpio, 50);
	u32 rightButton = XGpioPs_ReadPin(gpio, 51);

	if (ignoreButtonPress == 0) {
		if(leftButton == 1 || rightButton == 1) {
#ifdef LUI_DEBUG
			print("PS Button pressed!\n\r");
#endif // LUI_DEBUG
			*psPushButtonEnabled = 1;
		}
		else {
#ifdef LUI_DEBUG
			print("PS Button released!\n\r");
#endif // LUI_DEBUG
			*psPushButtonEnabled = 0;
		}
		if (timerPointer) {
			ignoreButtonPress = 1;
			XScuTimer_LoadTimer(timerPointer, 0x1FFFFFF);
			XScuTimer_Start(timerPointer);
		}
	}

#ifdef LUI_DEBUG
	print("Success! PS Push Button interrupts work!\r\n");
#endif // LUI_DEBUG
	// Clear the interrupt!
	// MIO 50 - Left button | MIO 51 - Right button
	XGpioPs_IntrClearPin(gpio, 50);
	XGpioPs_IntrClearPin(gpio, 51);

	return;
}

// This function is what will get called when the timer interrupt occurs.
void timerInterruptHandler(void *CallbackRef) {
	XScuTimer* timer = (XScuTimer *) CallbackRef;
	if (ignoreButtonPress == 1) {
		ignoreButtonPress = 0;

	}

	// Clear the interrupt!
	XScuTimer_ClearInterruptStatus(timer);
	return;
}

// #############################
// # GPIO/TIMER OBJECT SETTERS #
// #############################
// Allows for interrupt handlers to reference each other.

// This function takes in the timer object pointer, and enables the PS push buttons to be debounced.
void interruptSetTimer(XScuTimer* timer) {
	timerPointer = timer;
}

// This function takes in the GPIO switches object pointer, and sets it to allow other interrupt handlers to reference it if needed.
void interruptSetGpioSwitches(XGpio* switches) {
	gpioSwitches = switches;
}

// This function takes in the GPIO PS switches object pointer, and sets it to allow other interrupt handlers to reference it if needed.
void interruptSetGpioPsPushButtons(XGpioPs* pushButtons) {
	gpioPsPushButtons = pushButtons;
}
