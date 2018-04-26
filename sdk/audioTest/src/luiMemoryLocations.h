/*
 * luiMemoryLocations.h
 *
 *  Created on: 2018/02/16
 *      Author: ryan
 */

#include "xparameters.h"
#include "config.h"			// User configurable settings, all in one place.

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
#define DEVICE_ID_DMA_FFT				XPAR_AXI_DMA_FFT_DEVICE_ID
#define DEVICE_ID_DMA_MIX               XPAR_AXI_DMA_MIXER_DEVICE_ID
#define DEVICE_ID_DMA_RECORDED          XPAR_AXI_DMA_MIXER_RECORDED_SOUNDS_DEVICE_ID
#define DEVICE_ID_DMA_STORED            XPAR_AXI_DMA_MIXER_STORED_SOUNDS_DEVICE_ID
#define DEVICE_ID_TIMER					XPAR_PS7_SCUTIMER_0_DEVICE_ID

// TODO Make these relative to each other.
#define LUI_DDR_BASE_ADDR				(XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x700000) // 0x800000
#define LUI_MEM_SWITCHES 				(LUI_DDR_BASE_ADDR + 0x00)
#define LUI_MEM_PS_PUSHBUTTON_LEFT		(LUI_DDR_BASE_ADDR + 0x04)
#define LUI_MEM_PS_PUSHBUTTON_RIGHT		(LUI_DDR_BASE_ADDR + 0x08)
#define LUI_MEM_PL_PUSHBUTTONS			(LUI_DDR_BASE_ADDR + 0x0C)
#define PITCH_CNTR_LOCATION				(LUI_DDR_BASE_ADDR + 0x10)
#define ECHO_CNTR_LOCATION				(LUI_DDR_BASE_ADDR + 0x14)
#define EQUAL_CNTR_LOCATION   			(LUI_DDR_BASE_ADDR + 0x18)
#define EQUAL_SEC_LOCATION    			(LUI_DDR_BASE_ADDR + 0x1C)
#define SWITCH_UP_ECHO					(LUI_DDR_BASE_ADDR + 0x20)
#define SWITCH_UP_PITCH					(LUI_DDR_BASE_ADDR + 0x24)
#define RECORD_COUNTER                  (LUI_DDR_BASE_ADDR + 0x28) // this counter can go up to 48000 (*10)
#define MAX_RECORD_COUNTER              (LUI_DDR_BASE_ADDR + 0x2C)
#define PLAYBACK_COUNTER                (LUI_DDR_BASE_ADDR + 0x30)
#define STORED_SOUND_1_ENABLED			(LUI_DDR_BASE_ADDR + 0x34) 	// DJ Khaled - Another one
#define STORED_SOUND_2_ENABLED			(LUI_DDR_BASE_ADDR + 0x38)	// Airhorn
#define LOOPBACK_ENABLED				(LUI_DDR_BASE_ADDR + 0x3C)
#define RECORD2_COUNTER 				(LUI_DDR_BASE_ADDR + 0x40)
#define RECORD2_ENABLED 				(LUI_DDR_BASE_ADDR + 0x44)
#define PS_INTERRUPT_CONTROLLER			(LUI_DDR_BASE_ADDR + 0x48) // Size: 12 bytes.
#define PS_TIMER						(LUI_DDR_BASE_ADDR + 0x78) // Size: 16 bytes.


// Memory location defines for where to store audio data
#define LUI_BUFFER_SIZE_32B				(LUI_FFT_SIZE * 4)						// Size: FFT_SIZE samples * 4 bytes per sample (32 bits)
#define LUI_BUFFER_SIZE_64B				(LUI_FFT_SIZE * 8)						// Size: FFT_SIZE samples * 8 bytes per sample (64 bits)
#define LUI_BUFFER_SIZE_REC				(10 * 48000 * 4)						// Size: 10 seconds * 48k samples per second * 4 bytes per sample

#define DDR_BASE                  		XPAR_PS7_DDR_0_S_AXI_BASEADDR
#define CIRCULAR_BUFFER_BASE			(DDR_BASE + 0x00600000) // 0x00700000
#define TX_BUFFER_BASE             		(DDR_BASE + 0x00100000)								// Size: 2048 samples * 8 bytes per sample = 0x4000  | 0x00100000
#define TX_BUFFER_WINDOWED_BASE         (TX_BUFFER_BASE          + LUI_BUFFER_SIZE_64B)     // Size: 2048 samples * 8 bytes per sample = 0x4000  | 0x00104000
#define MX_BUFFER_BASE            		(TX_BUFFER_WINDOWED_BASE + LUI_BUFFER_SIZE_64B)     // Size: 2048 samples * 8 bytes per sample = 0x4000  | 0x00108000
#define MX_SHIFT_BUFFER_BASE            (MX_BUFFER_BASE          + LUI_BUFFER_SIZE_64B)     // Size: 2048 samples * 8 bytes per sample = 0x4000  | 0x0010C000
#define RX_BUFFER_BASE            		(MX_SHIFT_BUFFER_BASE    + LUI_BUFFER_SIZE_64B)     // Size: 2048 samples * 8 bytes per sample = 0x4000  | 0x00110000
#define RX_2_BUFFER_BASE            	(RX_BUFFER_BASE          + LUI_BUFFER_SIZE_64B)     // Size: 2048 samples * 8 bytes per sample = 0x4000  | 0x00114000
#define RX_SHIFT_BUFFER_BASE      		(RX_2_BUFFER_BASE        + LUI_BUFFER_SIZE_64B)     // Size: 2048 samples * 8 bytes per sample = 0x4000  | 0x00118000
#define RX_2_SHIFT_BUFFER_BASE      	(RX_SHIFT_BUFFER_BASE    + LUI_BUFFER_SIZE_64B)     // Size: 2048 samples * 8 bytes per sample = 0x4000  | 0x0011C000
#define REC_BUFFER_BASE      	        (RX_2_SHIFT_BUFFER_BASE  + LUI_BUFFER_SIZE_64B)     // Size: 10 seconds * 48k samples per second * 4 bytes per sample = 0x1D4C00
#define REC_2_BUFFER_BASE               (REC_BUFFER_BASE		 + LUI_BUFFER_SIZE_REC)     // Another recorded sound cause WHY NOT
#define RX_MIXED_BUFFER_BASE            (REC_2_BUFFER_BASE       + LUI_BUFFER_SIZE_REC)		// Size: 1024 samples * 4bytes
#define RX_TOMIX_BUFFER_BASE            (RX_MIXED_BUFFER_BASE    + LUI_BUFFER_SIZE_32B)
#define STORED_SOUND_ANOTHER_ONE		(DDR_BASE + 0x00404000) // 0x00504000, size (bytes): 174184
#define STORED_SOUND_AIRHORN			(DDR_BASE + 0x00430000) // 0x00530000, size (bytes): 577652

#define STORED_SOUND_ANOTHER_ONE_LENGTH	(43008)					// Round down, align to 1024 sample borders.
#define STORED_SOUND_AIRHORN_LENGTH		(144384)				// Round down, align to 1024 sample borders.
#endif /* SRC_LUIMEMORYLOCATIONS_H_ */

