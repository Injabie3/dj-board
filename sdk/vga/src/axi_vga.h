/*
 * axi_vga.h
 *
 *  Created on: Feb 25, 2018
 *      Author: slippman
 */

#ifndef _AXI_VGA_H_
#define _AXI_VGA_H_

#include "global.h"

#include "xparameters.h"
#include "xaxidma.h"
#include "xil_exception.h"
#include "interrupts.h"

#define DMA_DEV_ID			XPAR_AXI_DMA_VGA_DEVICE_ID
#define TX_INTR_ID			XPAR_FABRIC_AXIDMA_3_VEC_ID

#define FRAME1_BASE 		(MEM_BASE_ADDR)
#define FRAME2_BASE 		(FRAME1_BASE + FRAME_LEN)

#define RESET_TIMEOUT_COUNTER	10000

struct Image
{
	u16 * imgPtr;
	u16 height;
	u16 width;
};

int InitVGA(void);
void DestroyVGA(void);
void DrawFrame(u16 * img);

#endif /* _AXI_VGA_H_ */
