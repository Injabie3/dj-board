/*
 * luiMemoryLocations.h
 *
 *  Created on: 2018/02/16
 *      Author: ryan
 */

#include "xparameters.h"

#ifndef SRC_LUIMEMORYLOCATIONS_H_
#define SRC_LUIMEMORYLOCATIONS_H_

#define LUI_DDR_BASE_ADDR		XPAR_PS7_DDR_0_S_AXI_BASEADDR
#define LUI_MEM_SWITCHES 		LUI_DDR_BASE_ADDR + 0x0

// Bit 2: Left
// Bit 1: Right
// Bit 0: Both
#define LUI_MEM_AUDIO_CHANNELS	LUI_DDR_BASE_ADDR + 0x4

#endif /* SRC_LUIMEMORYLOCATIONS_H_ */
