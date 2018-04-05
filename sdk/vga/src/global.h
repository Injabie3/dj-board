#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "xparameters.h"
#include "xil_exception.h"
#include "../../../audioTest/src/luiMemoryLocations.h"
#include "xdebug.h"
#include "string.h"
#include "sleep.h"

//#define DEBUG

#define DDR_BASE_ADDR			XPAR_PS7_DDR_0_S_AXI_BASEADDR
#define MEM_BASE_ADDR		  	(DDR_BASE_ADDR + 0x1000000)

#define FRAME_WIDTH 			(640)
#define FRAME_HEIGHT			(480)
#define FRAME_LEN				(FRAME_WIDTH * FRAME_HEIGHT * 2) /* 640x480 = 307200 pixels * 2 bytes/pixel= 614400 bytes */

#define VIDEO_MEM_SIZE		 	(FRAME_LEN * 2) /* two frames */

#define GAME_BASE_ADDR			(MEM_BASE_ADDR + VIDEO_MEM_SIZE)

#define IMG_HEADER_LEN			(4)
#define CONTENT_BASE_ADDR		(DDR_BASE_ADDR + 0x2000000)

/********************************
 *
 * Image and song file locations
 *
 *******************************/
// Splash screen
#define SPLASH_SCREEN				(CONTENT_BASE_ADDR) 				// 0x0210 0000, size (bytes): 614404

// Text
#define VGA_TEXT_ECHO				(SPLASH_SCREEN + 614404) 			// 0x0219 6004, size (bytes): 5884
#define VGA_TEXT_PITCH				(VGA_TEXT_ECHO + 5884) 				// 0x0219 7700, size (bytes): 5764
#define VGA_TEXT_EQUALIZER			(VGA_TEXT_PITCH + 5764)				// 0x0219 8D84, size (bytes): 14724
#define VGA_TEXT_ON					(VGA_TEXT_EQUALIZER + 14724)		// 0x0219 C708, size (bytes): 3724, 184px x 40px
#define VGA_TEXT_OFF				(VGA_TEXT_ON + 3724)				// 0x0219 D594, size (bytes): 3724
#define VGA_TEXT_HIGH				(VGA_TEXT_OFF + 3724)				// 0x0219 E420, size (bytes): 7044
#define VGA_TEXT_MID				(VGA_TEXT_HIGH + 7044)				// 0x0219 FFA4, size (bytes): 4084
#define VGA_TEXT_LOW				(VGA_TEXT_MID + 4084) 				// 0x021A 0F98, size (bytes): 4804
#define VGA_TEXT_SFX				(VGA_TEXT_LOW + 4804)				// 0x021A 225C, size (bytes): 4804

#define SUCCESS 	 0
#define FAILURE 	-1

extern void xil_printf(const char *format, ...);

typedef unsigned char bool;

#endif
