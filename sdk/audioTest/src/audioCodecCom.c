
#include <stdio.h>
#include "xparameters.h"
#include "xstatus.h"
#include "audioCodecCom.h"
#include "xil_printf.h"
#include "luiMemoryLocations.h"
#include "xil_exception.h"
#include "fftCom.h"
#include "luiHanning.h"

volatile u64 *TxBufferPtr = (u64*)TX_BUFFER_BASE;
volatile u64 *TxBufferWindowedPtr = (u64*)TX_BUFFER_WINDOWED_BASE;
volatile u64 *MxBufferPtr = (u64*)MX_BUFFER_BASE;
volatile u64 *RxBufferPtr = (u64*)RX_BUFFER_BASE;
volatile u64 *RxShiftBufferPtr = (u64*)RX_SHIFT_BUFFER_BASE;
volatile u32* AUDIOCHIP = ((volatile u32*)XPAR_AUDIOINOUT16_0_S00_AXI_BASEADDR);

const int FFT_SIZE = 256;
// QUICK DEBUG SWITCHES
#define FFT_256_HAMMING	// Apply 256pt hamming.
//#define FFT_512_HAMMING

//#define FFT_256_NO_WIN // FFT 256pt No Window Overlap
//#define FFT_256_2_WIN // FFT 256pt 2. Window Overlap
//#define FFT_256_3_WIN // FFT 256pt 3. Window Overlap
#define FFT_256_4_WIN // FFT 256pt 4. Window Overlap

//#define FFT_512_2_WIN	// FFT 512pt 2. Window Overlap
//#define FFT_512_3_WIN // FFT 512pt 3. Window Overlap
//#define FFT_512_4_WIN // FFT 512pt 4. Window Overlap

void audioDriver(){
	int configStatus, status;

	// loop on audio
	while (1) {


//		for (int index = 0; index < 96; index++) { // For 128pt 3. Window Overlap
//			TxBufferPtr[index] = TxBufferPtr[32+index];
//		}

#ifdef FFT_256_2_WIN
		for (int index = 0; index < 128; index++) { // For 256pt 2. Window Overlap
			TxBufferPtr[index] = TxBufferPtr[128+index];
		}
#endif // FFT_256_2_WIN

#ifdef FFT_256_3_WIN
		for (int index = 0; index < 192; index++) { // For 256pt 3. Window Overlap
			TxBufferPtr[index] = TxBufferPtr[64+index];
		}
#endif // FFT_256_3_WIN

#ifdef FFT_256_4_WIN
		for (int index = 0; index < 224; index++) { // For 256pt 4. Window Overlap
			TxBufferPtr[index] = TxBufferPtr[32+index];
		}
#endif // FFT_256_4_WIN

		// read in audio data from ADC FIFO

//		dataIn(32, TxBufferPtr, 96);		// For 128pt 3. Window Overlap.

#ifdef FFT_256_NO_WIN
		dataIn(256, TxBufferPtr, 0);		// For 256pt no Window Overlap.
#endif // FFT_256_NO_WIN

#ifdef FFT_256_2_WIN
		dataIn(128, TxBufferPtr, 128);		// For 256pt 2. Window Overlap.
#endif // FFT_256_2_WIN

#ifdef FFT_256_3_WIN
		dataIn(64, TxBufferPtr, 192);		// For 256pt 3. Window Overlap.
#endif // FFT_256_3_WIN

#ifdef FFT_256_4_WIN
		dataIn(32, TxBufferPtr, 224);		// For 256pt 4. Window Overlap.
#endif // #ifdef FFT_256_4_WIN

		// Apply Hanning Window Overlap.
		// 0x0000RRRR0000LLLL
#ifdef FFT_256_HAMMING
		for (int index = 0; index < 256; index++) {		// For 256pt FFT
			u64 temp = TxBufferPtr[index];
			s16 tempLeft = (s16)temp;
			s16 tempRight = (s16)(temp >> 32);
			tempRight = (float)tempRight * hanning256[index];
			tempLeft = (float)tempLeft * hanning256[index];
			temp = (((u64)tempRight) << 32) | ((u64) tempLeft & 0xFFFF);
			TxBufferWindowedPtr[index] = temp;
		}
#endif // FFT_256_HAMMING

#ifdef FFT_512_HAMMING
		for (int index = 0; index < 512; index++) {		// For 512pt FFT
			u64 temp = TxBufferPtr[index];
			s16 tempLeft = (s16)temp;
			s16 tempRight = (s16)(temp >> 32);
			tempRight = (float)tempRight * hanning512[index];
			tempLeft = (float)tempLeft * hanning512[index];
			temp = (((u64)tempRight) << 32) | ((u64) tempLeft & 0xFFFF);
			TxBufferWindow OverlapedPtr[index] = temp;
		}
#endif // FFT_512_HAMMING

		configStatus = XGpio_FftConfig();
		status = XAxiDma_FftDataTransfer(DEVICE_ID_DMA, TxBufferWindowedPtr, MxBufferPtr);

		if (status != XST_SUCCESS) {
			xil_printf("XAxiDma_SimplePoll Example Failed\r\n");
		}


//		for (int index = 0; index < 126; index++) {
//			MxBufferPtr[127-index] = MxBufferPtr[127-(index+1)];
//			MxBufferPtr[255-(127-index)] = MxBufferPtr[255-(127-index+1)];
//		}
//		MxBufferPtr[0] = 0;
//		MxBufferPtr[255] = 0;
//		for (int index = 0; index < 254; index++) {
//			MxBufferPtr[255-index] = MxBufferPtr[255-(index+1)];
//		}
//		MxBufferPtr[0] = 0;

		// want to convert data back so we send it through IFFT

		configStatus = XGpio_IFftConfig();
		status = XAxiDma_FftDataTransfer(DEVICE_ID_DMA, MxBufferPtr, RxBufferPtr);

		if (status != XST_SUCCESS) {
				xil_printf("XAxiDma_SimplePoll Example Failed\r\n");
			}
		//need to convert output because it is shifted by 3 bits
		shiftBits(RxBufferPtr, RxShiftBufferPtr);

//		dataOut(32, RxShiftBufferPtr, 32);	// For 128pt 3. Window Overlap.

#ifdef FFT_256_NO_WIN
		dataOut(256, RxShiftBufferPtr, 0);	// For 256pt no Window Overlap.
#endif // FFT_256_NO_WIN

#ifdef FFT_256_2_WIN
		dataOut(128, RxShiftBufferPtr, 64);	// For 256pt 2. Window Overlap.
#endif // FFT_256_2_WIN

#ifdef FFT_256_3_WIN
		dataOut(64, RxShiftBufferPtr, 96);	// For 256pt 3. Window Overlap.
#endif // FFT_256_3_WIN

#ifdef FFT_256_4_WIN
		dataOut(32, RxShiftBufferPtr, 112);	// For 256pt 4. Window Overlap.
#endif // FFT_256_4_WIN
//		dataOut(32, RxShiftBufferPtr, 112);
	}
}


// Receives data from line in.
// Parameters:
// samplesToRead:	The number of samples to read.
// toBuffer:		A pointer to the buffer in which the read data will be stored.
// offset:			Start storing samples from the specified offset of the buffer above.
void dataIn(int samplesToRead, volatile u64* toBuffer, int offset) {
	//keep looping until we've read in 64 data samples
	u32 dataIn = 0;
	u32 temp = 0;
	u32 tempLeft = 0;
	u32 tempRight = 0;
	while (dataIn < samplesToRead){
		// check if the ADC FIFO is not empty
		if ((AUDIOCHIP[0] & 1<<2)==0){
			// get right and left channel
			// pad with 0s for 64bit input to FFT
			//0000RRRR0000LLLL
			temp = AUDIOCHIP[2];
			tempRight = temp & 0xFFFF0000;
			tempLeft = temp & 0xFFFF;
			tempRight>>=16;
			toBuffer[offset+dataIn] = (((u64)tempRight << 32) | tempLeft);
			dataIn++;
		}
	}
}

// Sends data out to headphone out.
// Parameters:
// samplesToSend:	The number of samples to send.
// fromBuffer:		A pointer to the buffer containing the samples to send.
// offset:			Start sending samples from the specified offset of the buffer above.
void dataOut(int samplesToSend, volatile u64* fromBuffer, int offset) {
	int dataOut = 0;
	u32 temp = 0;
	u32 tempLeft = 0;
	u32 tempRight = 0;
	//now we want to check the DAC
	while(dataOut < samplesToSend){
		// if DAC FIFO is not FULL we can write data to it
		if ((AUDIOCHIP[0] & 1<<5)==0) {
			tempRight = fromBuffer[offset+dataOut]>>16;
			tempLeft = fromBuffer[offset+dataOut];
			temp = (tempRight & 0xFFFF0000)| (tempLeft & 0xFFFF);
			AUDIOCHIP[1] = temp;
			dataOut++;
		}
	}
}
