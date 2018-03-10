/*
 * luiMemoryLocations.h
 *
 *  Created on: 2018/02/16
 *      Author: ryan
 */

#include "xparameters.h"

#ifndef SRC_LUIMEMORYLOCATIONS_H_
#define SRC_LUIMEMORYLOCATIONS_H_


// Device IDs
#define MIDDLEC_ADDRESS 				XPAR_AXI_GPIO_MIDDLEC_BASEADDR
#define DEVICE_ID_MIDDLEC 				XPAR_AXI_GPIO_MIDDLEC_DEVICE_ID
#define DEVICE_ID_SWITCHES 				XPAR_AXI_GPIO_SWITCHES_DEVICE_ID
#define DEVICE_ID_PUSHBUTTONS			XPAR_AXI_GPIO_BUTTONS_DEVICE_ID
#define DEVICE_ID_PS_PUSHBUTTONS		XPAR_XGPIOPS_0_DEVICE_ID
#define DEVICE_ID_FFT_GPIO				XPAR_AXI_GPIO_FFT_CONFIG_DEVICE_ID
#define DEVICE_ID_INTERRUPTCONTROLLER	XPAR_INTC_0_DEVICE_ID
#define DEVICE_ID_DMA					XPAR_AXIDMA_0_DEVICE_ID
#define DEVICE_ID_TIMER					XPAR_PS7_SCUTIMER_0_DEVICE_ID

#define LUI_DDR_BASE_ADDR				XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x700000
#define LUI_MEM_SWITCHES 				LUI_DDR_BASE_ADDR + 0x00
#define LUI_MEM_PS_PUSHBUTTONS			LUI_DDR_BASE_ADDR + 0x04
#define LUI_MEM_PL_PUSHBUTTONS			LUI_DDR_BASE_ADDR + 0x08
#define PITCH_CNTR_LOCATION				LUI_DDR_BASE_ADDR + 0x0C
#define ECHO_CNTR_LOCATION				LUI_DDR_BASE_ADDR + 0x10
#define EQUAL_CNTR_LOCATION   			LUI_DDR_BASE_ADDR + 0x14
#define EQUAL_SEC_LOCATION    			LUI_DDR_BASE_ADDR + 0x18
#define SWITCH_UP_ECHO					LUI_DDR_BASE_ADDR + 0x1C
#define SWITCH_UP_PITCH					LUI_DDR_BASE_ADDR + 0x20

//defines for memory locations to store audio data
#define DDR_BASE                  		XPAR_PS7_DDR_0_S_AXI_BASEADDR
#define CIRCULAR_BUFFER_BASE			(DDR_BASE + 0x00600000)
#define TX_BUFFER_BASE             		(DDR_BASE + 0x00100000)
#define TX_BUFFER_WINDOWED_BASE         (DDR_BASE + 0x00200000)
#define MX_BUFFER_BASE            		(DDR_BASE + 0x00300000)
#define MX_SHIFT_BUFFER_BASE            (DDR_BASE + 0x00310000)
#define RX_BUFFER_BASE            		(DDR_BASE + 0x00400000)
#define RX_2_BUFFER_BASE            	(DDR_BASE + 0x00440000)
#define RX_SHIFT_BUFFER_BASE      		(DDR_BASE + 0x00500000)
#define RX_2_SHIFT_BUFFER_BASE      	(DDR_BASE + 0x00540000)

#endif /* SRC_LUIMEMORYLOCATIONS_H_ */

