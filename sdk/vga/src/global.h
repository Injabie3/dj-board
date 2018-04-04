#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "xparameters.h"
#include "xil_exception.h"
#include "../../../audioTest/src/luiMemoryLocations.h"
#include "xdebug.h"
#include "string.h"
#include "sleep.h"

#define DEBUG

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



// Main Menu
#define MAIN_MENU_BACKGROUND		(SPLASH_SCREEN + 614404) 			// size(bytes): 614404
#define MAIN_MENU_PRESS_START		(MAIN_MENU_BACKGROUND + 614404) 	// size(bytes): 6388

// Game Over
#define GAME_OVER_BACKGROUND		(MAIN_MENU_PRESS_START + 6388) 		// size(bytes): 614404
#define GAME_OVER_DONT_GIVE_UP		(GAME_OVER_BACKGROUND + 614404)		// size(bytes): 37048
#define GAME_OVER_STAY_DETERMINED	(GAME_OVER_DONT_GIVE_UP + 37048)	// size(bytes): 37048
#define GAME_OVER_PRESS_START		(GAME_OVER_STAY_DETERMINED + 37048)	// size(bytes): 37048

// Pause Menu
#define PAUSE_MENU_BACKGROUND		(GAME_OVER_PRESS_START + 37048) 	// size(bytes): 150004
#define PAUSE_MENU_CONTINUE			(PAUSE_MENU_BACKGROUND + 150004)	// size(bytes): 3844
#define PAUSE_MENU_EXIT				(PAUSE_MENU_CONTINUE + 3844)		// size(bytes): 3844

// Game Scene
#define GAME_BACKGROUND 			(PAUSE_MENU_EXIT + 3844)			// size(bytes):	614404
#define FRISK_STILL_ADDR			(GAME_BACKGROUND + 614404)			// size(bytes): 1412
#define FRISK_LADDER_ADDR			(FRISK_STILL_ADDR + 1412)			// size(bytes): 1412
#define FRISK_LEFT_0_ADDR			(FRISK_LADDER_ADDR + 1412)			// size(bytes): 1412
#define FRISK_LEFT_1_ADDR			(FRISK_LEFT_0_ADDR + 1412)			// size(bytes): 1412
#define FRISK_RIGHT_0_ADDR			(FRISK_LEFT_1_ADDR + 1412)			// size(bytes): 1412
#define FRISK_RIGHT_1_ADDR			(FRISK_RIGHT_0_ADDR + 1412)			// size(bytes): 1412
#define PLATFORM_BASE_ADDR			(FRISK_RIGHT_1_ADDR + 1412)			// size(bytes): 40964
#define PLATFORM_LVL_ADDR			(PLATFORM_BASE_ADDR + 40964)		// size(bytes): 18436
#define LADDER_ADDR					(PLATFORM_LVL_ADDR + 18436)			// size(bytes): 4036
#define BONUS_ADDR					(LADDER_ADDR + 4036)				// size(bytes): 436

#define SUCCESS 	 0
#define FAILURE 	-1

extern void xil_printf(const char *format, ...);

typedef unsigned char bool;

#endif
