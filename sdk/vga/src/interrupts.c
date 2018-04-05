#include "interrupts.h"

static XScuGic* Intc = (XScuGic *)PS_INTERRUPT_CONTROLLER;		/* Instance of the Interrupt Controller */
static bool isConfigured = FALSE;

int InterruptInit(void)
{
	int Status = SUCCESS;
	// Commenting out the following section, since this is done in ARM core 0.
//	XScuGic_Config *IntcConfig;

//	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
//	if (NULL == IntcConfig)
//	{
//		return FAILURE;
//	}
//
//	//Status = XScuGic_CfgInitialize(&Intc, IntcConfig, IntcConfig->CpuBaseAddress);
//	if (Status != SUCCESS)
//	{
//		return FAILURE;
//	}
//
//	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			(Xil_ExceptionHandler) XScuGic_InterruptHandler, (void *) Intc);
//
	Xil_ExceptionEnable();

	isConfigured = TRUE;

	return SUCCESS;
}
void InterruptDestroy(void)
{

}

int EnableInterrupts(void * DevicePtr, u16 IntrId, void (*callback)(void * data), u8 priority)
{
	int Status;

	if (isConfigured == FALSE)
	{
		Status = InterruptInit();

		if (Status != SUCCESS)
		{
			return Status;
		}
	}

	XScuGic_SetPriorityTriggerType(Intc, IntrId, priority, 0x3); // Rising Edge

	Status = XScuGic_Connect(Intc, IntrId, (Xil_InterruptHandler)callback, DevicePtr);

	if (Status != SUCCESS)
	{
		return Status;
	}

	XScuGic_Enable(Intc, IntrId);

	return SUCCESS;
}

void DisableInterrupt(u16 IntrId)
{
	XScuGic_Disconnect(&Intc, IntrId);
}

bool IsConfigured(void)
{
	return isConfigured;
}
