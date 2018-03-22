/*
 * interrupts.h
 *
 *  Created on: Feb 25, 2018
 *      Author: slippman
 */

#ifndef _INTERRUPTS_H_
#define _INTERRUPTS_H_

#include "global.h"
#include "xscugic.h"
#include "xparameters.h"
#include "xil_exception.h"

#define INTC_DEVICE_ID  	XPAR_SCUGIC_SINGLE_DEVICE_ID

int InterruptInit(void);
void InterruptDestroy(void);

int EnableInterrupts(void * DevicePtr, u16 IntrId, void (*callback)(void * data), u8 priority);
void DisableInterrupt(u16 IntrId);

bool IsConfigured(void);

#endif /* _INTERRUPTS_H_ */
