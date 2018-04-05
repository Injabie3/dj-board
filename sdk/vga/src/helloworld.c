/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include "global.h"
#include "../../../audioTest/src/luiMemoryLocations.h"
#include "interrupts.h"
#include "axi_vga.h"
#include "video.h"
#include <stdio.h>
#include <stdbool.h>
#include "platform.h"
#include "xil_printf.h"


int main()
{
	u16 * Frame1 = (u16 *)FRAME1_BASE;	// Double buffer, frame 1
	u16 * Frame2 = (u16 *)FRAME2_BASE;	// Double buffer, frame 2.

    init_platform();

    int* p0Echo = (int *)ECHO_CNTR_LOCATION;
    int* p0Pitch= (int *)PITCH_CNTR_LOCATION;
    u16* p0Equalizer = (u16 *)EQUAL_SEC_LOCATION;
    int* p0Sfx = (int *)LUI_MEM_PS_PUSHBUTTON_RIGHT;
    int currentEcho = 0;
    int currentPitch = 0;
    int currentEqualizer = 0;
    int currentSfx = 0;
    bool echoChanged = false;
    bool pitchChanged = false;
    bool equalizerChanged = false;
    bool sfxChanged = false;

    InitVGA();

    // Draw the initial sprites, with all audio effects off.
    DrawImage(Frame1, (struct image *) SPLASH_SCREEN, 0, 0);
    DrawImage(Frame1, (struct image *) VGA_TEXT_ECHO, 136, 50);
    DrawImage(Frame1, (struct image *) VGA_TEXT_PITCH, 136, 100);
    DrawImage(Frame1, (struct image *) VGA_TEXT_SFX, 154, 150);
    DrawImage(Frame1, (struct image *) VGA_TEXT_EQUALIZER, 50, 200);
    DrawImage(Frame1, (struct image *) VGA_TEXT_HIGH, 146, 250);
    DrawImage(Frame1, (struct image *) VGA_TEXT_MID, 166, 300);
    DrawImage(Frame1, (struct image *) VGA_TEXT_LOW, 154, 350);

    DrawImage(Frame1, (struct image *) VGA_TEXT_OFF, 250, 50);		// For Echo
    DrawImage(Frame1, (struct image *) VGA_TEXT_OFF, 250, 100);		// For Pitch
    DrawImage(Frame1, (struct image *) VGA_TEXT_OFF, 250, 150);		// For SFX
    DrawImage(Frame1, (struct image *) VGA_TEXT_OFF, 250, 350);		// For Low equalizer
    DrawImage(Frame1, (struct image *) VGA_TEXT_OFF, 250, 250);		// For High equalizer
    DrawImage(Frame1, (struct image *) VGA_TEXT_OFF, 250, 300);		// For Mid equalizer
    DrawImage(Frame1, (struct image *) VGA_TEXT_OFF, 250, 350);		// For Low equalizer



    DrawImage(Frame2, (struct image *) SPLASH_SCREEN, 0, 0);
    DrawImage(Frame2, (struct image *) VGA_TEXT_ECHO, 136, 50);
    DrawImage(Frame2, (struct image *) VGA_TEXT_PITCH, 136, 100);
    DrawImage(Frame2, (struct image *) VGA_TEXT_SFX, 154, 150);
    DrawImage(Frame2, (struct image *) VGA_TEXT_EQUALIZER, 50, 200);
    DrawImage(Frame2, (struct image *) VGA_TEXT_HIGH, 146, 250);
    DrawImage(Frame2, (struct image *) VGA_TEXT_MID, 166, 300);
    DrawImage(Frame2, (struct image *) VGA_TEXT_LOW, 154, 350);

    DrawImage(Frame2, (struct image *) VGA_TEXT_OFF, 250, 50);		// For Echo
    DrawImage(Frame2, (struct image *) VGA_TEXT_OFF, 250, 100);		// For Pitch
    DrawImage(Frame2, (struct image *) VGA_TEXT_OFF, 250, 150);		// For SFX
    DrawImage(Frame2, (struct image *) VGA_TEXT_OFF, 250, 350);		// For Low equalizer
    DrawImage(Frame2, (struct image *) VGA_TEXT_OFF, 250, 250);		// For High equalizer
    DrawImage(Frame2, (struct image *) VGA_TEXT_OFF, 250, 300);		// For Mid equalizer
    DrawImage(Frame2, (struct image *) VGA_TEXT_OFF, 250, 350);		// For Low equalizer

	Xil_DCacheFlushRange((UINTPTR)Frame1, FRAME_LEN);
	Xil_DCacheFlushRange((UINTPTR)Frame2, FRAME_LEN);


    while(1)
    {
    	// Check to see if anything was updated from the other core.
    	if (currentEcho != *p0Echo) {
    		currentEcho = *p0Echo;
    		echoChanged = true;
    	}

    	if (currentPitch != *p0Pitch) {
    		currentPitch = *p0Pitch;
    		pitchChanged = true;
    	}

    	if(currentEqualizer != *p0Equalizer) {
    		currentEqualizer = *p0Equalizer;
    		equalizerChanged = true;
    	}

    	if(currentSfx != *p0Sfx) {
    		currentSfx = *p0Sfx;
    		sfxChanged = true;
    	}

    	if (echoChanged == true) {
    		if (currentEcho == 0) { // Echo off
    			DrawImage(Frame1, (struct image *) VGA_TEXT_OFF, 250, 50);
    			DrawImage(Frame2, (struct image *) VGA_TEXT_OFF, 250, 50);
    		}
    		else { // currentEcho != 0
    			DrawImage(Frame1, (struct image *) VGA_TEXT_ON, 250, 50);
    			DrawImage(Frame2, (struct image *) VGA_TEXT_ON, 250, 50);
    		}
    		echoChanged = false;
    	}

    	if (pitchChanged == true) {
    		if (currentPitch == 0) { // Pitch off
    			DrawImage(Frame1, (struct image *) VGA_TEXT_OFF, 250, 100);
    			DrawImage(Frame2, (struct image *) VGA_TEXT_OFF, 250, 100);
    		}
    		else { // currentPitch != 0
    			DrawImage(Frame1, (struct image *) VGA_TEXT_ON, 250, 100);
    			DrawImage(Frame2, (struct image *) VGA_TEXT_ON, 250, 100);
    		}
    		pitchChanged = false;
    	}

    	// Based off of Ivana's HI/MID/LOW
    	// 3 - High
    	// 2 - Mid
    	// 1 - Low
    	// 0 - Off.
    	if (equalizerChanged == true) {

			if (currentEqualizer == 3) { // High frequency adjustmennt
				DrawImage(Frame1, (struct image *) VGA_TEXT_ON, 250, 250);
				DrawImage(Frame1, (struct image *) VGA_TEXT_OFF, 250, 300);
				DrawImage(Frame1, (struct image *) VGA_TEXT_OFF, 250, 350);
				DrawImage(Frame2, (struct image *) VGA_TEXT_ON, 250, 250);
				DrawImage(Frame2, (struct image *) VGA_TEXT_OFF, 250, 300);
				DrawImage(Frame2, (struct image *) VGA_TEXT_OFF, 250, 350);
			}
			else if (currentEqualizer == 2) { // Mid frequency adjustmennt
				DrawImage(Frame1, (struct image *) VGA_TEXT_OFF, 250, 250);
				DrawImage(Frame1, (struct image *) VGA_TEXT_ON, 250, 300);
				DrawImage(Frame1, (struct image *) VGA_TEXT_OFF, 250, 350);
				DrawImage(Frame2, (struct image *) VGA_TEXT_OFF, 250, 250);
				DrawImage(Frame2, (struct image *) VGA_TEXT_ON, 250, 300);
				DrawImage(Frame2, (struct image *) VGA_TEXT_OFF, 250, 350);
			}
			else if (currentEqualizer == 1) { // Low frequency adjustmennt
				DrawImage(Frame1, (struct image *) VGA_TEXT_OFF, 250, 250);
				DrawImage(Frame1, (struct image *) VGA_TEXT_OFF, 250, 300);
				DrawImage(Frame1, (struct image *) VGA_TEXT_ON, 250, 350);
				DrawImage(Frame2, (struct image *) VGA_TEXT_OFF, 250, 250);
				DrawImage(Frame2, (struct image *) VGA_TEXT_OFF, 250, 300);
				DrawImage(Frame2, (struct image *) VGA_TEXT_ON, 250, 350);
			}
			else { // if (currentEqualizer == 0) { // Pitch off
				DrawImage(Frame1, (struct image *) VGA_TEXT_OFF, 250, 250);
				DrawImage(Frame1, (struct image *) VGA_TEXT_OFF, 250, 300);
				DrawImage(Frame1, (struct image *) VGA_TEXT_OFF, 250, 350);
				DrawImage(Frame2, (struct image *) VGA_TEXT_OFF, 250, 250);
				DrawImage(Frame2, (struct image *) VGA_TEXT_OFF, 250, 300);
				DrawImage(Frame2, (struct image *) VGA_TEXT_OFF, 250, 350);
			}
			equalizerChanged = false;
		}

    	//
    	if (sfxChanged == true) {
    		if (currentSfx == 0) { // Pitch off
    			DrawImage(Frame1, (struct image *) VGA_TEXT_OFF, 250, 150);
    			DrawImage(Frame2, (struct image *) VGA_TEXT_OFF, 250, 150);
    		}
    		else { // currentPitch != 0
    			DrawImage(Frame1, (struct image *) VGA_TEXT_ON, 250, 150);
    			DrawImage(Frame2, (struct image *) VGA_TEXT_ON, 250, 150);
    		}
    		sfxChanged = false;
    	}

    	// Flush cache to immediately draw image on screen.
    	Xil_DCacheFlushRange((UINTPTR)Frame1, FRAME_LEN);
    	Xil_DCacheFlushRange((UINTPTR)Frame2, FRAME_LEN);


#ifdef LUI_DEBUG
    	print("Hello World from CPU 1\n\r");
    	for(int i=0; i< 20000000; i++);
#endif // LUI_DEBUG

    }
    cleanup_platform();
    return 0;
}
