
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "xparameters.h"
#include "xstatus.h"
#include "audioCodecCom.h"
#include "xil_printf.h"
#include "luiMemoryLocations.h"
#include "xil_exception.h"
#include "fftCom.h"
#include "luiHanning.h"
#include "luiCircularBuffer.h"

volatile u64 *TxBufferPtr = (u64*)TX_BUFFER_BASE;
volatile u64 *TxBufferWindowedPtr = (u64*)TX_BUFFER_WINDOWED_BASE;
volatile u64 *MxBufferPtr = (u64*)MX_BUFFER_BASE;
volatile u64 *RxBufferPtr = (u64*)RX_BUFFER_BASE;
volatile u64 *Rx2BufferPtr = (u64*)RX_2_BUFFER_BASE;
volatile u64 *RxShiftBufferPtr = (u64*)RX_SHIFT_BUFFER_BASE;
volatile u32 *RecBufferPtr = (volatile u32*)REC_BUFFER_BASE;
volatile u64 *Rx2ShiftBufferPtr = (u64*)RX_2_SHIFT_BUFFER_BASE;
volatile u32* AUDIOCHIP = ((volatile u32*)XPAR_AUDIOINOUT16_0_S00_AXI_BASEADDR);
int* echoCounter = (int *) ECHO_CNTR_LOCATION;
circular_buf_t circularBuffer;
int *maxRecordCounter = (int*) MAX_RECORD_COUNTER;
int *recordCounter = (int*) RECORD_COUNTER;
int *playBackCounter = (int*) PLAYBACK_COUNTER;


// QUICK DEBUG SWITCHES
//#define FFT_256_HANNING		// Apply 256pt hanning.
#define FFT_512_HANNING	// Apply 512pt hanning

#define MIRROR_FFT

//#define FFT_256_NO_WIN 		// FFT 256pt No Window Overlap
//#define FFT_256_2_WIN 		// FFT 256pt 2. Window Overlap
//#define FFT_256_3_WIN 		// FFT 256pt 3. Window Overlap
//#define FFT_256_4_WIN 		// FFT 256pt 4. Window Overlap

//#define FFT_512_2_WIN			// FFT 512pt 2. Window Overlap
#define FFT_512_2_WIN_SUM		// FFT 512pt 2. Window Overlap, except using Dan's summing suggestion.
//#define FFT_512_3_WIN 		// FFT 512pt 3. Window Overlap
//#define FFT_512_4_WIN 		// FFT 512pt 4. Window Overlap
//#define FFT_512_5_WIN 		// FFT 512pt 5. Window Overlap
#define OVERLAY



void audioDriver(){
	int configStatus, status;
	int bufferedSamples = 0;
	// Instantiate the circular buffer.

	circularBuffer.size = 48000*3;
	circularBuffer.buffer = (uint32_t*) CIRCULAR_BUFFER_BASE;
	circular_buf_reset(&circularBuffer);
	circularBuffer.startingIndex = 24000;


	// loop on audio
	while (1) {


//		for (int index = 0; index < 96; index++) { // For 128pt 3. Window Overlap
//			TxBufferPtr[index] = TxBufferPtr[32+index];
//		}

#ifdef FFT_256_4_WIN
		for (int index = 0; index < 224; index++) { // For 256pt 4. Window Overlap
			TxBufferPtr[index] = TxBufferPtr[32+index];
		}
#endif // FFT_256_4_WIN

#ifdef FFT_512_2_WIN_SUM
		// Shift 256 samples over before reading 256 in the next iteration.
		for (int index = 0; index < 256; index++) { // For 512pt 2. Window Overlap, except using Dan's summing suggestion.
			TxBufferPtr[index] = TxBufferPtr[256+index];
		}
#endif // FFT_512_2_WIN_SUM

#ifdef FFT_512_4_WIN
		// Shift 448 samples over before reading 64 in the next iteration.
		for (int index = 0; index < 448; index++) { // For 512pt 4. Window Overlap
			TxBufferPtr[index] = TxBufferPtr[64+index];
		}
#endif // FFT_512_4_WIN

		// read in audio data from ADC FIFO

#ifdef FFT_256_4_WIN
		dataIn(32, TxBufferPtr, 224);		// For 256pt 4. Window Overlap.
#endif // #ifdef FFT_256_4_WIN

#ifdef FFT_512_2_WIN_SUM
		// Get 256 samples per iteration.
		dataIn(256, TxBufferPtr, 256);		// For 512pt 2. Window Overlap, except using Dan's summing suggestion.
#endif // FFT_512_2_WIN_SUM

#ifdef FFT_512_4_WIN
		// Get 64 samples per iteration.
		dataIn(64, TxBufferPtr, 448);		// For 512pt 2. Window Overlap.
#endif // FFT_512_4_WIN

		// Apply Hanning Window Overlap.
		// 0x0000RRRR0000LLLL
#ifdef FFT_256_HANNING
		for (int index = 0; index < 256; index++) {		// For 256pt FFT
			u64 temp = TxBufferPtr[index];
			s16 tempLeft = (s16)temp;
			s16 tempRight = (s16)(temp >> 32);
			tempRight = (float)tempRight * hanning256[index];
			tempLeft = (float)tempLeft * hanning256[index];
			temp = (((u64)tempRight) << 32) | ((u64) tempLeft & 0xFFFF);
			TxBufferWindowedPtr[index] = temp;
		}
#endif // FFT_256_HANNING

#ifdef FFT_512_HANNING
		for (int index = 0; index < 512; index++) {		// For 512pt FFT
			u64 temp = TxBufferPtr[index];
			s16 tempLeft = (s16)temp;
			s16 tempRight = (s16)(temp >> 32);
			tempRight = (float)tempRight * hanning512[index];
			tempLeft = (float)tempLeft * hanning512[index];
			temp = (((u64)tempRight) << 32) | ((u64) tempLeft & 0xFFFF);
			TxBufferWindowedPtr[index] = temp;
		}
#endif // FFT_512_HANNING

		configStatus = XGpio_FftConfig();
		status = XAxiDma_FftDataTransfer(DEVICE_ID_DMA, TxBufferWindowedPtr, MxBufferPtr);

		if (status != XST_SUCCESS) {
			xil_printf("XAxiDma_SimplePoll Example Failed\r\n");
		}

		adjustPitch();

		equalize();

		// want to convert data back so we send it through IFFT

		configStatus = XGpio_IFftConfig();
#ifdef FFT_512_2_WIN_SUM
		status = XAxiDma_FftDataTransfer(DEVICE_ID_DMA, MxBufferPtr, Rx2BufferPtr);

		if (status != XST_SUCCESS) {
				xil_printf("XAxiDma_SimplePoll Example Failed\r\n");
			}
		//need to convert output because it is shifted by 3 bits
		shiftBits(Rx2BufferPtr, Rx2ShiftBufferPtr);

		// Sum bits
		for (int index = 0; index < 256; index++) {
			RxShiftBufferPtr[256+index] += Rx2ShiftBufferPtr[index];
		}
#else
		status = XAxiDma_FftDataTransfer(DEVICE_ID_DMA, MxBufferPtr, RxBufferPtr);

		if (status != XST_SUCCESS) {
				xil_printf("XAxiDma_SimplePoll Example Failed\r\n");
			}
		//need to convert output because it is shifted by 3 bits
		shiftBits(RxBufferPtr, RxShiftBufferPtr);
#endif // FFT_512_2_WIN_SUM
//		dataOut(32, RxShiftBufferPtr, 32);	// For 128pt 3. Window Overlap.

#ifdef FFT_256_4_WIN
		// Send middle 32 samples
		dataOut(32, RxShiftBufferPtr, 112);	// For 512pt 4. Window Overlap.
#endif // FFT_256_4_WIN

#ifdef FFT_512_2_WIN_SUM
		// Send last 256 samples
		if (bufferedSamples <= 24000) {
			dataOut(256, RxShiftBufferPtr, 256, true);	// For 512pt 2. Window Overlap, except with Dan's summing suggestion
			bufferedSamples += 256;
		}
		else {
			dataOut(256, RxShiftBufferPtr, 256, false);	// For 512pt 2. Window Overlap, except with Dan's summing suggestion
		}


		// Move Rx2Buffer to RxBuffer
		for (int index = 0; index < 512; index++) {
			RxShiftBufferPtr[index] = Rx2ShiftBufferPtr[index];
		}
#endif // FFT_512_2_WIN_SUM

#ifdef FFT_512_4_WIN
		// Send middle 64 samples
		dataOut(64, RxShiftBufferPtr, 224);	// For 512pt 3. Window Overlap.
#endif // FFT_512_4_WIN

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
	u32* psLeftPushButtonEnabled = (u32 *) LUI_MEM_PS_PUSHBUTTON_LEFT;

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
			// want to record data in for how long button is being held down or a maximum of seconds
			if ((*psLeftPushButtonEnabled)) {
				if ((*maxRecordCounter) < 480000){
				// want to write samples for 5 seconds to a recording buffer
				RecBufferPtr[*maxRecordCounter] = temp;
				(*recordCounter)++;
				(*maxRecordCounter)++;
				}
				else {
				   *maxRecordCounter = 0;
				   *psLeftPushButtonEnabled = 0;
				}
			}
			else {
				*maxRecordCounter = 0;
//				// once the button is released
			}
		}
	}
}

// Sends data out to headphone out.
// Parameters:
// samplesToSend:	The number of samples to send.
// fromBuffer:		A pointer to the buffer containing the samples to send.
// offset:			Start sending samples from the specified offset of the buffer above.
void dataOut(int samplesToSend, volatile u64* fromBuffer, int offset, bool circularBufferOnly) {
	int dataOut = 0;
	u32 temp = 0;
	int16_t tempLeft = 0;
	int16_t tempRight = 0;
	int16_t tempLeftRec = 0;
	int16_t tempRightRec = 0;
	int echoAmount = *echoCounter;
	u32* psRightPushButtonEnabled = (u32 *) LUI_MEM_PS_PUSHBUTTON_RIGHT;

	//now we want to check the DAC
	while(dataOut < samplesToSend){
		// if DAC FIFO is not FULL we can write data to it
#ifndef OVERLAY
		if ((AUDIOCHIP[0] & 1<<5)==0) {
			if (*psRightPushButtonEnabled){

			 if ((*playBackCounter) < *recordCounter){
				AUDIOCHIP[1] = RecBufferPtr[*playBackCounter];
				(*playBackCounter)++;
			 }
			 else {
				 //*recordCounter = 0;
				 *playBackCounter = 0;
				 *psRightPushButtonEnabled = 0;
			 }
			}
			else {
				*psRightPushButtonEnabled = 0;
				*playBackCounter = 0;
			tempRight = fromBuffer[offset+dataOut]>>16;
			tempLeft = fromBuffer[offset+dataOut];
			temp = (tempRight & 0xFFFF0000)| (tempLeft & 0xFFFF);
			if (!circularBufferOnly) {
				if (echoAmount == 0)
					circular_buf_get(&circularBuffer, &AUDIOCHIP[1]);
				else
					circular_buf_getSummedTaps(&circularBuffer, &AUDIOCHIP[1], echoAmount*120);
			}
			circular_buf_put(&circularBuffer, temp);

			//AUDIOCHIP[1] = temp;

		}
#endif
#ifdef OVERLAY
		if ((AUDIOCHIP[0] & 1<<5)==0) {

				tempRight = fromBuffer[offset+dataOut]>>32;
				tempLeft = fromBuffer[offset+dataOut];
				if (*psRightPushButtonEnabled){

				 if ((*playBackCounter) < (*recordCounter)){
					tempLeftRec = (RecBufferPtr[*playBackCounter]) & 0xFFFF;
					tempRightRec = (RecBufferPtr[*playBackCounter])>>16;
					(*playBackCounter)++;
				 	 }
				 else {
					 //*recordCounter = 0;
					 *playBackCounter = 0;
					 *psRightPushButtonEnabled = 0;
				 	 }
				}

				temp = ((tempRight+tempRightRec)<<16)| ((tempLeft+tempLeftRec) & 0xFFFF);
				if (!circularBufferOnly) {
					if (echoAmount == 0)
						circular_buf_get(&circularBuffer, &AUDIOCHIP[1]);
					else
						circular_buf_getSummedTaps(&circularBuffer, &AUDIOCHIP[1], echoAmount*120);
				}
				circular_buf_put(&circularBuffer, temp);

				//AUDIOCHIP[1] = temp;

#endif
			dataOut++;
		}
	}
}

// adjust pitch based on counter value
//pitchCounter
void adjustPitch() {
	int* pitchCounter = (int*)PITCH_CNTR_LOCATION;
	if (*pitchCounter != 0){
		// do stuff with the MxBufferPtr to adjust the pitch
		//RRRRRRRRLLLLLLLL
		//RRRRRRRR
		//shift by amount of pitch counter
		int pitchVal = *pitchCounter;
		if (pitchVal > 0){

#ifdef MIRROR_FFT
			// for positive data shift to the RIGHT
			// for negative data shift to the LEFT
			// Original
			for (int i=0; i<255 - pitchVal;i++){
				MxBufferPtr[256-i] = MxBufferPtr[256-(i+pitchVal+1)];
			}
			for (int i=0; i<254 - pitchVal; i++) {
				MxBufferPtr[511-(254-i)] = MxBufferPtr[511-(254-i-pitchVal)];
			}
			for (int j=0; j<pitchVal;j++){
				MxBufferPtr[j+1] = 0;
			}
			for (int j=0; j<pitchVal;j++) {
				MxBufferPtr[511-j]=0;
			}

			// Experimental - Not working.
//			for (int i=128; i>=0; i--){
//				MxBufferPtr[2*i] = MxBufferPtr[i];
//				MxBufferPtr[511-2*i] = MxBufferPtr[511-i];
//			}
//			for (int i=0; i<128; i++) {
//				MxBufferPtr[1+2*i] = 0;
//				MxBufferPtr[511-1-2*i] = 0;
//
//			}

#endif

#ifndef MIRROR_FFT

			for (int i=0;i<512;i++){
				MxBufferPtr[511-i] = MxBufferPtr[511-i - pitchVal];
			}
			for (int j=0; j<pitchVal;j++){
				MxBufferPtr[j] = 0;
			}

#endif
		}

		//SHIFT DOWN
		else{
			int pitchValPositive = -pitchVal;
#ifdef MIRROR_FFT
// Original
			for (int i=0; i<255 - pitchValPositive;i++){
				MxBufferPtr[i] = MxBufferPtr[i+pitchValPositive];
				MxBufferPtr[511-i] = MxBufferPtr[511-(i+pitchValPositive)];
			}
			for (int j=0;j<pitchValPositive;j++){

				MxBufferPtr[255-j] = 0;
				MxBufferPtr[256+j] = 0;
			}

// Experimental - not working.
//			for (int i=8; i>=0; i--){
//				MxBufferPtr[255-240-2*i] = MxBufferPtr[255-i];
//				MxBufferPtr[256+240+2*i] = MxBufferPtr[256+i];
//			}
//			for (int i=0; i<8; i++) {
//				//MxBufferPtr[255-240-1-2*i] = 0;
//				//MxBufferPtr[256+240+1+2*i] = 0;
//			}
#endif

#ifndef MIRROR_FFT
			for (int i=0;i<512;i++){
				MxBufferPtr[i] = MxBufferPtr[i+pitchValPositive];
			}
			for (int j=0; j<pitchValPositive;j++){
				MxBufferPtr[511-j] = 0;
			}
#endif

		}

	}
}

// this function is to increase the volume of a user-specified pitch range (low mid high)
void equalize(){
	// define temp variables to store left and right channel data
	int16_t rChImag, rChReal, lChImag, lChReal = 0;
	double lChMag, lChPhase, rChMag, rChPhase = 0;

	//this variable tells us which frequency we which to amplify (need to set up with interrupt still)
	// will have to be a pointer value
	// can only hold 4 values
	//if 0 - do not equalize anything
	//1 - amplify LOW frequency sounds
	//2 - amplify MID frequency sounds
	//3 - amplify HIGH frequency sounds
	u16* equalizePart = (u16*) EQUAL_SEC_LOCATION;
	int* equalizeCounter = (int*) EQUAL_CNTR_LOCATION;
	if (*equalizePart!= 0){
		// want to amplify LOW frequency sounds
		if (*equalizePart == 1){
			// for 512 point FFT we define
			// low frequencies in bins 1 to 85
			for (int i=427;i<512;i++) {
				rChImag = (MxBufferPtr[i] & 0xFFFF000000000000) >> 48;
				rChReal = (MxBufferPtr[i] & 0xFFFF00000000) >> 32;
				lChImag = (MxBufferPtr[i] & 0xFFFF0000) >> 16;
				lChReal = MxBufferPtr[i] & 0xFFFF;

				// shift to get correct values


				lChMag = sqrt(((double)lChImag*(double)lChImag) + ((double)lChReal*(double)lChReal));
				lChPhase = atan((double)lChImag/(double)lChReal);
				rChMag = sqrt((rChImag*rChImag) + (rChReal*rChReal));
				rChPhase = atan((double)rChImag/(double)rChReal);


				// now need to increase magnitude by counter amount
				if (*equalizeCounter > 0){
					lChMag *= (*equalizeCounter); // so just going to add to it rn not sure what effect this will have
					rChMag *= (*equalizeCounter);
				}
				else if (*equalizeCounter < 0){
					lChMag /= (-1)*(*equalizeCounter);
					rChMag /= (-1)*(*equalizeCounter);
				}
				//now need to convert back to real/imaginary
				lChReal = lChMag *cos(lChPhase);
				lChImag = lChMag *sin(lChPhase);
				rChReal = rChMag * cos(rChPhase);
				rChImag = rChMag * sin(rChPhase);

				// but back in MxBuf to be sent to IFFT
				MxBufferPtr[i] = ((((uint64_t)rChImag) << 48) | (((uint64_t)rChReal) << 32) | (((uint64_t)lChImag) << 16) | (uint64_t)(lChReal));

				rChImag = (MxBufferPtr[511-i] & 0xFFFF000000000000) >> 48;
				rChReal = (MxBufferPtr[511-i] & 0xFFFF00000000) >> 32;
				lChImag = (MxBufferPtr[511-i] & 0xFFFF0000) >> 16;
				lChReal = MxBufferPtr[511-i] & 0xFFFF;



				lChMag = sqrt(((double)lChImag*(double)lChImag) + ((double)lChReal*(double)lChReal));
				lChPhase = atan((double)lChImag/(double)lChReal);
				rChMag = sqrt((rChImag*rChImag) + (rChReal*rChReal));
				rChPhase = atan((double)rChImag/(double)rChReal);


				// now need to increase magnitude by counter amount
				// now need to increase magnitude by counter amount
				if (*equalizeCounter > 0){
					lChMag *= (*equalizeCounter); // so just going to add to it rn not sure what effect this will have
					rChMag *= (*equalizeCounter);
				}
				else if (*equalizeCounter < 0){
					lChMag /= (-1)*(*equalizeCounter);
					rChMag /= (-1)*(*equalizeCounter);
				}
				//now need to convert back to real/imaginary
				lChReal = lChMag * cos(lChPhase);
				lChImag = lChMag * sin(lChPhase);
				rChReal = rChMag * cos(rChPhase);
				rChImag = rChMag * sin(rChPhase);

				// but back in MxBuf to be sent to IFFT
				MxBufferPtr[511-i] = ((((uint64_t)rChImag) << 48) | (((uint64_t)rChReal) << 32) | (((uint64_t)lChImag) << 16) | (uint64_t)(lChReal));

			}
		}
		// want to amplify MID frequency sounds
		else if(*equalizePart == 2){
			for (int i=85;i<171;i++) {
				rChImag = (MxBufferPtr[i] & 0xFFFF000000000000) >> 48;
				rChReal = (MxBufferPtr[i] & 0xFFFF00000000) >> 32;
				lChImag = (MxBufferPtr[i] & 0xFFFF0000) >> 16;
				lChReal = MxBufferPtr[i] & 0xFFFF;

				// shift to get correct values


				lChMag = sqrt(((double)lChImag*(double)lChImag) + ((double)lChReal*(double)lChReal));
				lChPhase = atan((double)lChImag/(double)lChReal);
				rChMag = sqrt((rChImag*rChImag) + (rChReal*rChReal));
				rChPhase = atan((double)rChImag/(double)rChReal);


				// now need to increase magnitude by counter amount
				if (*equalizeCounter > 0){
					lChMag *= (*equalizeCounter); // so just going to add to it rn not sure what effect this will have
					rChMag *= (*equalizeCounter);
				}
				else if (*equalizeCounter < 0){
					lChMag /= (-1)*(*equalizeCounter);
					rChMag /= (-1)*(*equalizeCounter);
				}
				//now need to convert back to real/imaginary
				lChReal = lChMag *cos(lChPhase);
				lChImag = lChMag *sin(lChPhase);
				rChReal = rChMag * cos(rChPhase);
				rChImag = rChMag * sin(rChPhase);

				// but back in MxBuf to be sent to IFFT
				MxBufferPtr[i] = ((((uint64_t)rChImag) << 48) | (((uint64_t)rChReal) << 32) | (((uint64_t)lChImag) << 16) | (uint64_t)(lChReal));

				rChImag = (MxBufferPtr[511-i] & 0xFFFF000000000000) >> 48;
				rChReal = (MxBufferPtr[511-i] & 0xFFFF00000000) >> 32;
				lChImag = (MxBufferPtr[511-i] & 0xFFFF0000) >> 16;
				lChReal = MxBufferPtr[511-i] & 0xFFFF;



				lChMag = sqrt(((double)lChImag*(double)lChImag) + ((double)lChReal*(double)lChReal));
				lChPhase = atan((double)lChImag/(double)lChReal);
				rChMag = sqrt((rChImag*rChImag) + (rChReal*rChReal));
				rChPhase = atan((double)rChImag/(double)rChReal);


				// now need to increase magnitude by counter amount
				// now need to increase magnitude by counter amount
				if (*equalizeCounter > 0){
					lChMag *= (*equalizeCounter); // so just going to add to it rn not sure what effect this will have
					rChMag *= (*equalizeCounter);
				}
				else if (*equalizeCounter < 0){
					lChMag /= (-1)*(*equalizeCounter);
					rChMag /= (-1)*(*equalizeCounter);
				}
				//now need to convert back to real/imaginary
				lChReal = lChMag * cos(lChPhase);
				lChImag = lChMag * sin(lChPhase);
				rChReal = rChMag * cos(rChPhase);
				rChImag = rChMag * sin(rChPhase);

				// but back in MxBuf to be sent to IFFT
				MxBufferPtr[511-i] = ((((uint64_t)rChImag) << 48) | (((uint64_t)rChReal) << 32) | (((uint64_t)lChImag) << 16) | (uint64_t)(lChReal));

			}

		}
		// want to amplify HIGH frequency sounds
		else if (*equalizePart == 3){
			for (int i=128;i<256;i++) {
				rChImag = (MxBufferPtr[i] & 0xFFFF000000000000) >> 48;
				rChReal = (MxBufferPtr[i] & 0xFFFF00000000) >> 32;
				lChImag = (MxBufferPtr[i] & 0xFFFF0000) >> 16;
				lChReal = MxBufferPtr[i] & 0xFFFF;

				// shift to get correct values


				lChMag = sqrt(((double)lChImag*(double)lChImag) + ((double)lChReal*(double)lChReal));
				lChPhase = atan((double)lChImag/(double)lChReal);
				rChMag = sqrt((rChImag*rChImag) + (rChReal*rChReal));
				rChPhase = atan((double)rChImag/(double)rChReal);


				// now need to increase magnitude by counter amount
				if (*equalizeCounter > 0){
					lChMag *= (*equalizeCounter); // so just going to add to it rn not sure what effect this will have
					rChMag *= (*equalizeCounter);
				}
				else if (*equalizeCounter < 0){
					lChMag /= (-1)*(*equalizeCounter);
					rChMag /= (-1)*(*equalizeCounter);
				}
				//now need to convert back to real/imaginary
				lChReal = lChMag *cos(lChPhase);
				lChImag = lChMag *sin(lChPhase);
				rChReal = rChMag * cos(rChPhase);
				rChImag = rChMag * sin(rChPhase);

				// but back in MxBuf to be sent to IFFT
				MxBufferPtr[i] = ((((uint64_t)rChImag) << 48) | (((uint64_t)rChReal) << 32) | (((uint64_t)lChImag) << 16) | (uint64_t)(lChReal));

				rChImag = (MxBufferPtr[511-i] & 0xFFFF000000000000) >> 48;
				rChReal = (MxBufferPtr[511-i] & 0xFFFF00000000) >> 32;
				lChImag = (MxBufferPtr[511-i] & 0xFFFF0000) >> 16;
				lChReal = MxBufferPtr[511-i] & 0xFFFF;



				lChMag = sqrt(((double)lChImag*(double)lChImag) + ((double)lChReal*(double)lChReal));
				lChPhase = atan((double)lChImag/(double)lChReal);
				rChMag = sqrt((rChImag*rChImag) + (rChReal*rChReal));
				rChPhase = atan((double)rChImag/(double)rChReal);


				// now need to increase magnitude by counter amount
				// now need to increase magnitude by counter amount
				if (*equalizeCounter > 0){
					lChMag *= (*equalizeCounter); // so just going to add to it rn not sure what effect this will have
					rChMag *= (*equalizeCounter);
				}
				else if (*equalizeCounter < 0){
					lChMag /= (-1)*(*equalizeCounter);
					rChMag /= (-1)*(*equalizeCounter);
				}
				//now need to convert back to real/imaginary
				lChReal = lChMag * cos(lChPhase);
				lChImag = lChMag * sin(lChPhase);
				rChReal = rChMag * cos(rChPhase);
				rChImag = rChMag * sin(rChPhase);

				// but back in MxBuf to be sent to IFFT
				MxBufferPtr[511-i] = ((((uint64_t)rChImag) << 48) | (((uint64_t)rChReal) << 32) | (((uint64_t)lChImag) << 16) | (uint64_t)(lChReal));


			} // ends for loop

		} // ends if condition for which part to equalize
	} // ends if condition for if we want to equalize at all


} // ends function
