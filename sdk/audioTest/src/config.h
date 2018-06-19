/*
 * config.h
 *
 *  Created on: 2018/04/05
 *      Author: ryan
 */

#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_

// Change beat detection threshold
//#define BEAT_DETECTION_SPEAKERS
#define BEAT_DETECTION_HEADPHONES // Use this threshold for headphones, so volume doesn't have to be super loud.
//#define BEAT_DETECTION_HEADPHONES_PIANO // Use this threshold for headphones, so volume doesn't have to be super loud.

// Turn the following on to debug with the UART console.  Used for interrupts.
//#define LUI_DEBUG

// Set the FFT size here.
#define LUI_FFT_SIZE		8192
#define LUI_FFT_SIZE_HALF	LUI_FFT_SIZE/2


#endif /* SRC_CONFIG_H_ */
