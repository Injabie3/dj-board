
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "xparameters.h"
#include "xstatus.h"
#include "xgpio.h" 					// GPIO drivers, PL side.
#include "audioCodecCom.h"
#include "xaxidma.h"
#include "xil_printf.h"
#include "luiMemoryLocations.h"
#include "xil_exception.h"
#include "fftCom.h"
#include "luiHanning.h"
#include "luiCircularBuffer.h"


#define DEVICE_ID_PLAYBACKINTERRUPT		XPAR_AXI_GPIO_PLAY_INTERRUPT_DEVICE_ID

volatile u64 *TxBufferPtr = (u64*)TX_BUFFER_BASE;
volatile u64 *TxBufferWindowedPtr = (u64*)TX_BUFFER_WINDOWED_BASE;
volatile u64 *MxBufferPtr = (u64*)MX_BUFFER_BASE;
volatile u64 *RxBufferPtr = (u64*)RX_BUFFER_BASE;
volatile u64 *Rx2BufferPtr = (u64*)RX_2_BUFFER_BASE;
volatile u64 *RxShiftBufferPtr = (u64*)RX_SHIFT_BUFFER_BASE;
volatile u32 *RecBufferPtr = (volatile u32*)REC_BUFFER_BASE;
volatile u32 *Rec2BufferPtr = (volatile u32*)REC_2_BUFFER_BASE;
volatile u64 *Rx2ShiftBufferPtr = (u64*)RX_2_SHIFT_BUFFER_BASE;

volatile u32 *RxToMixBufferPtr = (u32*)RX_TOMIX_BUFFER_BASE;
volatile u32 *RxMixedBufferPtr = (u32*)RX_MIXED_BUFFER_BASE;


volatile u32* AUDIOCHIP = ((volatile u32*)XPAR_AUDIOINOUT16_0_S00_AXI_BASEADDR);
int* echoCounter = (int *) ECHO_CNTR_LOCATION;
circular_buf_t circularBuffer;
int *maxRecordCounter = (int*) MAX_RECORD_COUNTER;
int *recordCounter = (int*) RECORD_COUNTER;
int *record2Counter = (int*) RECORD2_COUNTER;
int *playBackCounter = (int*) PLAYBACK_COUNTER;

static XGpio gpioPlaybackInterrupt; // AXI GPIO object for play back interrupt
XAxiDma axiDmaFft;		// DMA for the FFT.
XAxiDma axiDmaRecord;	// DMA for the recorded data
XAxiDma axiDmaRx;		// DMA for the standard audio data
XAxiDma axiDmaStore;

// QUICK DEBUG SWITCHES
//#define FFT_256_HANNING		// Apply 256pt hanning.
#define FFT_512_HANNING	// Apply 512pt hanning

#define MIRROR_FFT

#define FFT_512_2_WIN_SUM		// FFT 512pt 2. Window Overlap, except using Dan's summing suggestion.
#define OVERLAY



void audioDriver(){
	int status;
	int bufferedSamples = 0;

	int* switchUpStoredSound1 = (int*) STORED_SOUND_1_ENABLED;
	int* switchUpStoredSound2 = (int*) STORED_SOUND_2_ENABLED;
	int* switchUpLoopback = (int*) LOOPBACK_ENABLED;
	int* recordSound2 = (int*) RECORD2_ENABLED;

	u32* psRightPushButtonEnabled = (u32 *) LUI_MEM_PS_PUSHBUTTON_RIGHT;

	// Instantiate the circular buffer.
	circularBuffer.size = 48000*3;
	circularBuffer.buffer = (uint32_t*) CIRCULAR_BUFFER_BASE;
	circular_buf_reset(&circularBuffer);
	circularBuffer.startingIndex = 24000;

	//configure the GPIO to send the playback interrupt to hardware block
	status = XGpio_Initialize(&gpioPlaybackInterrupt, DEVICE_ID_PLAYBACKINTERRUPT);

	if (status != XST_SUCCESS) {
		xil_printf("Error: GPIO Playback interrupt initialization failed!\r\n");
		return;
	}

	// Configure the FFT DMA.
	status = setupDma(&axiDmaFft, DEVICE_ID_DMA_FFT);

	if (status != XST_SUCCESS) {
		xil_printf("Error: FFT DMA initialization failed!\r\n");
		return;
	}

	// Configure the FFT GPIO
	status = setUpFftGpio(DEVICE_ID_FFT_GPIO);

	if (status != XST_SUCCESS) {
		xil_printf("Error: FFT GPIO initialization failed!\r\n");
		return;
	}

	// Configure the DMAs for the audio mixer.
	status = setupDma(&axiDmaRecord, DEVICE_ID_DMA_RECORDED);
	if (status != XST_SUCCESS) {
		xil_printf("Error: Mixer (Record) DMA initialization failed!\r\n");
		return;
	}
	status = setupDma(&axiDmaRx, DEVICE_ID_DMA_MIX);
	if (status != XST_SUCCESS) {
		xil_printf("Error: Mixer (Mix) DMA initialization failed!\r\n");
		return;
	}
	status = setupDma(&axiDmaStore, DEVICE_ID_DMA_STORED);
	if (status != XST_SUCCESS) {
		xil_printf("Error: Mixer (Store) DMA initialization failed!\r\n");
		return;
	}

	XGpio_DiscreteWrite(&gpioPlaybackInterrupt, 1, 0);
	XGpio_DiscreteWrite(&gpioPlaybackInterrupt, 2, 0);

	// Loop on audio
	while (1) {

		// Shift half the samples over before reading the next half in the next iteration.
		for (int index = 0; index < (LUI_FFT_SIZE_HALF); index++) { // For 1024pt 2. Window Overlap, except using Dan's summing suggestion.
			TxBufferPtr[index] = TxBufferPtr[(LUI_FFT_SIZE_HALF)+index];
		}


		// read in audio data from ADC FIFO
		// Get 512 samples per iteration.
		dataIn(LUI_FFT_SIZE_HALF, TxBufferPtr, LUI_FFT_SIZE_HALF);		// For 512pt 2. Window Overlap, except using Dan's summing suggestion.


		// Apply Hanning Window Overlap.
		// 0x0000RRRR0000LLLL
		for (int index = 0; index < LUI_FFT_SIZE; index++) {		// For 512pt FFT
			u64 temp = TxBufferPtr[index];
			s16 tempLeft = (s16)temp;
			s16 tempRight = (s16)(temp >> 32);
			tempRight = (float)tempRight * hanning[index];
			tempLeft = (float)tempLeft * hanning[index];
			temp = (((u64)tempRight) << 32) | ((u64) tempLeft & 0xFFFF);
			TxBufferWindowedPtr[index] = temp;
		}

		fftConfigForward();
		status = XAxiDma_FftDataTransfer(DEVICE_ID_DMA_FFT, TxBufferWindowedPtr, MxBufferPtr, &axiDmaFft);

		if (status != XST_SUCCESS) {
			xil_printf("XAxiDma_SimplePoll Example Failed\r\n");
		}

		adjustPitch();

		equalize();

		// want to convert data back so we send it through IFFT

		fftConfigInverse();
		status = XAxiDma_FftDataTransfer(DEVICE_ID_DMA_FFT, MxBufferPtr, Rx2BufferPtr, &axiDmaFft);

#ifdef LUI_DEBUG
		if (status != XST_SUCCESS) {
			xil_printf("XAxiDma_SimplePoll Example Failed\r\n");
		}
#endif // LUI_DEBUG

		// need to convert output because it is shifted by 3 bits
		shiftBits(Rx2BufferPtr, Rx2ShiftBufferPtr);

		// Sum bits
		for (int index = 0; index < (LUI_FFT_SIZE_HALF); index++) {
			s16 tempRight = ((s16)(RxShiftBufferPtr[(LUI_FFT_SIZE_HALF)+index] >> 32) & 0xFFFF) + ((s16)(Rx2ShiftBufferPtr[index] >> 32) & 0xFFFF);
			s16 tempLeft = (s16)(RxShiftBufferPtr[(LUI_FFT_SIZE_HALF)+index] & 0xFFFF) + (s16)(Rx2ShiftBufferPtr[index] & 0xFFFF);

			RxShiftBufferPtr[(LUI_FFT_SIZE_HALF)+index] = (((u64)tempRight & 0xFFFF) << 32) | ((tempLeft & 0xFFFF));
		}

		// drive both interrupts to 0 so ivana's hardware block doesn't die
		 XGpio_DiscreteWrite(&gpioPlaybackInterrupt, 1, 0);
		 XGpio_DiscreteWrite(&gpioPlaybackInterrupt, 2, 0);

		// if interrupt is enabled
		if (*psRightPushButtonEnabled){
		// start DATA TRANSFER of recorded sample with DMA



//////////----------TO PLAY ANOTHER ONE ----------------------///////////
			 if (*switchUpStoredSound1 == 1){
				 if ((*playBackCounter) < (STORED_SOUND_ANOTHER_ONE_LENGTH)){
					 // send recordPlayback interrupt
					 XGpio_DiscreteWrite(&gpioPlaybackInterrupt, 2, *psRightPushButtonEnabled);
					 status = XAxiDma_MixerDataTransfer(DEVICE_ID_DMA_STORED, (u32*)STORED_SOUND_ANOTHER_ONE+(*playBackCounter), RxMixedBufferPtr, &axiDmaStore, 0);
					(*playBackCounter)+=LUI_FFT_SIZE_HALF;
				 }
				 else {
					 *playBackCounter = 0;
					 *psRightPushButtonEnabled = 0;
					 XGpio_DiscreteWrite(&gpioPlaybackInterrupt, 2, 0);
					}

			}

			else if (*switchUpStoredSound2 == 1){
				 if ((*playBackCounter) < (STORED_SOUND_AIRHORN_LENGTH)){
					 // send recordPlayback interrupt
					 XGpio_DiscreteWrite(&gpioPlaybackInterrupt, 2, *psRightPushButtonEnabled);
					 status = XAxiDma_MixerDataTransfer(DEVICE_ID_DMA_STORED, (u32*)STORED_SOUND_AIRHORN+(*playBackCounter), RxMixedBufferPtr, &axiDmaStore, 0);
					(*playBackCounter)+=LUI_FFT_SIZE_HALF;
				 }
				 else {
					 *playBackCounter = 0;
					 *psRightPushButtonEnabled = 0;
					 XGpio_DiscreteWrite(&gpioPlaybackInterrupt, 2, 0);
					 }
			}



			else if (*switchUpLoopback == 1){
				// need to determine which sound to play
				// to play recorded sound #2
				if (*recordSound2 == 1){
					 if ((*playBackCounter) < ((*record2Counter)-LUI_FFT_SIZE_HALF)){
						 // send recordPlayback interrupt
						 XGpio_DiscreteWrite(&gpioPlaybackInterrupt, 1, *psRightPushButtonEnabled);
					    status = XAxiDma_MixerDataTransfer(DEVICE_ID_DMA_RECORDED, Rec2BufferPtr+(*playBackCounter), RxMixedBufferPtr, &axiDmaRecord, 0);
						(*playBackCounter)+=LUI_FFT_SIZE_HALF;
					 }
					 else {
					 		*playBackCounter = 0;
					 		//*psRightPushButtonEnabled = 1;
					 		XGpio_DiscreteWrite(&gpioPlaybackInterrupt, 1, 0);
					 	 }

				}
				// to play recorded sound #1
				else {
					 if ((*playBackCounter) < ((*recordCounter)-LUI_FFT_SIZE_HALF)){
						 // send recordPlayback interrupt
						 XGpio_DiscreteWrite(&gpioPlaybackInterrupt, 1, *psRightPushButtonEnabled);
							 status = XAxiDma_MixerDataTransfer(DEVICE_ID_DMA_RECORDED, RecBufferPtr+(*playBackCounter), RxMixedBufferPtr, &axiDmaRecord, 0);
						(*playBackCounter)+=LUI_FFT_SIZE_HALF;
					 }
					 else {
					 		*playBackCounter = 0;
					 		//*psRightPushButtonEnabled = 1;
					 		XGpio_DiscreteWrite(&gpioPlaybackInterrupt, 1, 0);
					 	 }
				}
			}

			else if (*recordSound2 == 1){
				 if ((*playBackCounter) < ((*record2Counter)-LUI_FFT_SIZE_HALF)){
					 // send recordPlayback interrupt
					 XGpio_DiscreteWrite(&gpioPlaybackInterrupt, 1, *psRightPushButtonEnabled);
					 status = XAxiDma_MixerDataTransfer(DEVICE_ID_DMA_RECORDED, Rec2BufferPtr+(*playBackCounter), RxMixedBufferPtr, &axiDmaRecord, 0);
					(*playBackCounter)+=LUI_FFT_SIZE_HALF;
				 }
				 else {
					 *playBackCounter = 0;
					 *psRightPushButtonEnabled = 0;
					 XGpio_DiscreteWrite(&gpioPlaybackInterrupt, 1, 0);
			    }
			}

			else {
				 if ((*playBackCounter) < ((*recordCounter)-LUI_FFT_SIZE_HALF)){
					 // send recordPlayback interrupt
					 XGpio_DiscreteWrite(&gpioPlaybackInterrupt, 1, *psRightPushButtonEnabled);
					 status = XAxiDma_MixerDataTransfer(DEVICE_ID_DMA_RECORDED, RecBufferPtr+(*playBackCounter), RxMixedBufferPtr, &axiDmaRecord, 0);
					(*playBackCounter)+=LUI_FFT_SIZE_HALF;
				 }
				 else {
					 *playBackCounter = 0;
					 *psRightPushButtonEnabled = 0;
					 XGpio_DiscreteWrite(&gpioPlaybackInterrupt, 1, 0);
					 }
			}
		}

		sendToMixer(RxShiftBufferPtr, RxMixedBufferPtr);

		// Send last 512 samples
		if (bufferedSamples <= 24000) {
			dataOut(LUI_FFT_SIZE_HALF, RxMixedBufferPtr, 0, true);	// For 1024pt 2. Window Overlap, except with Dan's summing suggestion
			bufferedSamples += LUI_FFT_SIZE_HALF;
		}
		else {
			dataOut(LUI_FFT_SIZE_HALF, RxMixedBufferPtr, 0, false);	// For 1024pt 2. Window Overlap, except with Dan's summing suggestion
		}


		// Move Rx2Buffer to RxBuffer
		for (int index = 0; index < LUI_FFT_SIZE; index++) {
			RxShiftBufferPtr[index] = Rx2ShiftBufferPtr[index];
		}


	}
}

void sendToMixer(volatile u64* toSendBuffer, volatile u32* recieveBuffer){
	// here we will receive a 64 bit buffer and need to get rid of the extra stuff
	//u32 temp = 0;
	u32 tempLeft = 0;
	u32 tempRight = 0;

	for (int i=LUI_FFT_SIZE_HALF; i<LUI_FFT_SIZE; i++){
		tempRight = toSendBuffer[i]>>32;
		tempLeft = toSendBuffer[i];
		RxToMixBufferPtr[i-LUI_FFT_SIZE_HALF] = ((tempRight << 16) | (tempLeft & 0xFFFF));
	}
	// send data to HW block through DMA and get it back
	XAxiDma_MixerDataTransfer(DEVICE_ID_DMA_MIX, RxToMixBufferPtr, recieveBuffer, &axiDmaRx, 1);
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
	int* recordSound2 = (int*) RECORD2_ENABLED;

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
					// want to see which sample we are recording
					if (*recordSound2 == 1){
						Rec2BufferPtr[*maxRecordCounter] = temp;
						(*record2Counter)++;
						(*maxRecordCounter)++;
					}
					else {
						// want to write samples for 5 seconds to a recording buffer
						RecBufferPtr[*maxRecordCounter] = temp;
						(*recordCounter)++;
						(*maxRecordCounter)++;
						}
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

// Sends data out to headphone out.
// Parameters:
// samplesToSend:	The number of samples to send.
// fromBuffer:		A pointer to the buffer containing the samples to send.
// offset:			Start sending samples from the specified offset of the buffer above.
void dataOut(int samplesToSend, volatile u32* fromBuffer, int offset, bool circularBufferOnly) {
	int dataOut = 0;
	u32 temp = 0;
//	int16_t tempLeft = 0;
//	int16_t tempRight = 0;
//	int16_t tempLeftRec = 0;
//	int16_t tempRightRec = 0;
	int echoAmount = *echoCounter;
	//u32* psRightPushButtonEnabled = (u32 *) LUI_MEM_PS_PUSHBUTTON_RIGHT;

	//now we want to check the DAC
	while(dataOut < (samplesToSend)){
		// if DAC FIFO is not FULL we can write data to it

		if ((AUDIOCHIP[0] & 1<<5)==0) {
				temp = fromBuffer[dataOut+offset];

				if (!circularBufferOnly) { // This is used to get us enough samples before we start using delay.
					if (echoAmount == 0)
						circular_buf_get(&circularBuffer, &AUDIOCHIP[1]);
					else
						circular_buf_getSummedTaps(&circularBuffer, &AUDIOCHIP[1], echoAmount*120);
				}
				circular_buf_put(&circularBuffer, temp);

			dataOut++;
		}
	}
}

// Adjust pitch based on counter value
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
//			for (int i=0; i<255 - pitchVal;i++){
//				MxBufferPtr[256-i] = MxBufferPtr[256-(i+pitchVal+1)];
//			}
//			for (int i=0; i<254 - pitchVal; i++) {
//				MxBufferPtr[511-(254-i)] = MxBufferPtr[511-(254-i-pitchVal)];
//			}
//			for (int j=0; j<pitchVal;j++){
//				MxBufferPtr[j+1] = 0;
//			}
//			for (int j=0; j<pitchVal;j++) {
//				MxBufferPtr[511-j]=0;
//			}

			// Experimental - Not working.
			for (int i=512; i>=0; i--){
				MxBufferPtr[2*i] = MxBufferPtr[i];
				MxBufferPtr[4095-2*i] = MxBufferPtr[4095-i];
			}
			for (int i=0; i<512; i++) {
				MxBufferPtr[1+2*i] = 0;
				MxBufferPtr[4095-1-2*i] = 0;

			}

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
//			for (int i=0; i<255 - pitchValPositive;i++){
//				MxBufferPtr[i] = MxBufferPtr[i+pitchValPositive];
//				MxBufferPtr[511-i] = MxBufferPtr[511-(i+pitchValPositive)];
//			}
//			for (int j=0;j<pitchValPositive;j++){
//
//				MxBufferPtr[255-j] = 0;
//				MxBufferPtr[256+j] = 0;
//			}

// Experimental - not working.
			for (int i=256; i>=0; i--){
				MxBufferPtr[511-2*i] = MxBufferPtr[511-i];
				MxBufferPtr[512+2*i] = MxBufferPtr[512+i];
			}
			for (int i=0; i<256; i++) {
				MxBufferPtr[511-1-2*i] = 0;
				MxBufferPtr[512+1+2*i] = 0;
			}
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
//	int* equalizeCounter = (int*) EQUAL_CNTR_LOCATION;
	if (*equalizePart!= 0){
		// want to amplify LOW frequency sounds
		if (*equalizePart == 1){
			// for 512 point FFT we define
			// low frequencies in bins 1 to 10
			for (int i=0;i<10;i++) {
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
//				if (*equalizeCounter > 0){
//					lChMag *= (*equalizeCounter); // so just going to add to it rn not sure what effect this will have
//					rChMag *= (*equalizeCounter);
//				}
//				else if (*equalizeCounter < 0){
//					lChMag /= (-1)*(*equalizeCounter);
//					rChMag /= (-1)*(*equalizeCounter);
//				}

//------------------------Equalizing will no longer be user manipulated - can only be "on" or "off" --------------------------------//

				lChMag /= 10;
				rChMag /= 10;
				//now need to convert back to real/imaginary
				lChReal = lChMag *cos(lChPhase);
				lChImag = lChMag *sin(lChPhase);
				rChReal = rChMag * cos(rChPhase);
				rChImag = rChMag * sin(rChPhase);

				// but back in MxBuf to be sent to IFFT
				MxBufferPtr[i] = ((((uint64_t)rChImag) << 48) | (((uint64_t)rChReal) << 32) | (((uint64_t)lChImag) << 16) | (uint64_t)(lChReal));

				rChImag = (MxBufferPtr[1023-i] & 0xFFFF000000000000) >> 48;
				rChReal = (MxBufferPtr[1023-i] & 0xFFFF00000000) >> 32;
				lChImag = (MxBufferPtr[1023-i] & 0xFFFF0000) >> 16;
				lChReal = MxBufferPtr[1023-i] & 0xFFFF;



				lChMag = sqrt(((double)lChImag*(double)lChImag) + ((double)lChReal*(double)lChReal));
				lChPhase = atan((double)lChImag/(double)lChReal);
				rChMag = sqrt((rChImag*rChImag) + (rChReal*rChReal));
				rChPhase = atan((double)rChImag/(double)rChReal);


				// now need to increase magnitude by counter amount
				// now need to increase magnitude by counter amount
//				if (*equalizeCounter > 0){
//					lChMag *= (*equalizeCounter); // so just going to add to it rn not sure what effect this will have
//					rChMag *= (*equalizeCounter);
//				}
//				else if (*equalizeCounter < 0){
//					lChMag /= (-1)*(*equalizeCounter);
//					rChMag /= (-1)*(*equalizeCounter);
//				}
//------------------------Equalizing will no longer be user manipulated - can only be "on" or "off" --------------------------------//

				lChMag /= 10;
				rChMag /= 10;
				//now need to convert back to real/imaginary
				lChReal = lChMag * cos(lChPhase);
				lChImag = lChMag * sin(lChPhase);
				rChReal = rChMag * cos(rChPhase);
				rChImag = rChMag * sin(rChPhase);

				// but back in MxBuf to be sent to IFFT
				MxBufferPtr[1023-i] = ((((uint64_t)rChImag) << 48) | (((uint64_t)rChReal) << 32) | (((uint64_t)lChImag) << 16) | (uint64_t)(lChReal));

			}
		}
		// want to amplify MID frequency sounds
		else if(*equalizePart == 2){
			for (int i=40;i<160;i++) {
				rChImag = (MxBufferPtr[i] & 0xFFFF000000000000) >> 48;
				rChReal = (MxBufferPtr[i] & 0xFFFF00000000) >> 32;
				lChImag = (MxBufferPtr[i] & 0xFFFF0000) >> 16;
				lChReal = MxBufferPtr[i] & 0xFFFF;

				// shift to get correct values


				lChMag = sqrt(((double)lChImag*(double)lChImag) + ((double)lChReal*(double)lChReal));
				lChPhase = atan((double)lChImag/(double)lChReal);
				rChMag = sqrt((rChImag*rChImag) + (rChReal*rChReal));
				rChPhase = atan((double)rChImag/(double)rChReal);


//				// now need to increase magnitude by counter amount
//				if (*equalizeCounter > 0){
//					lChMag *= (*equalizeCounter); // so just going to add to it rn not sure what effect this will have
//					rChMag *= (*equalizeCounter);
//				}
//				else if (*equalizeCounter < 0){
//					lChMag /= (-1)*(*equalizeCounter);
//					rChMag /= (-1)*(*equalizeCounter);
//				}
//------------------------Equalizing will no longer be user manipulated - can only be "on" or "off" --------------------------------//


				lChMag /= 10;
				rChMag /= 10;
				//now need to convert back to real/imaginary
				lChReal = lChMag *cos(lChPhase);
				lChImag = lChMag *sin(lChPhase);
				rChReal = rChMag * cos(rChPhase);
				rChImag = rChMag * sin(rChPhase);

				// but back in MxBuf to be sent to IFFT
				MxBufferPtr[i] = ((((uint64_t)rChImag) << 48) | (((uint64_t)rChReal) << 32) | (((uint64_t)lChImag) << 16) | (uint64_t)(lChReal));

				rChImag = (MxBufferPtr[1023-i] & 0xFFFF000000000000) >> 48;
				rChReal = (MxBufferPtr[1023-i] & 0xFFFF00000000) >> 32;
				lChImag = (MxBufferPtr[1023-i] & 0xFFFF0000) >> 16;
				lChReal = MxBufferPtr[1023-i] & 0xFFFF;



				lChMag = sqrt(((double)lChImag*(double)lChImag) + ((double)lChReal*(double)lChReal));
				lChPhase = atan((double)lChImag/(double)lChReal);
				rChMag = sqrt((rChImag*rChImag) + (rChReal*rChReal));
				rChPhase = atan((double)rChImag/(double)rChReal);


				// now need to increase magnitude by counter amount
				// now need to increase magnitude by counter amount
//				if (*equalizeCounter > 0){
//					lChMag *= (*equalizeCounter); // so just going to add to it rn not sure what effect this will have
//					rChMag *= (*equalizeCounter);
//				}
//				else if (*equalizeCounter < 0){
//					lChMag /= (-1)*(*equalizeCounter);
//					rChMag /= (-1)*(*equalizeCounter);
//				}

				lChMag /= 10;
				rChMag /= 10;
				//now need to convert back to real/imaginary
				lChReal = lChMag * cos(lChPhase);
				lChImag = lChMag * sin(lChPhase);
				rChReal = rChMag * cos(rChPhase);
				rChImag = rChMag * sin(rChPhase);

				// but back in MxBuf to be sent to IFFT
				MxBufferPtr[1023-i] = ((((uint64_t)rChImag) << 48) | (((uint64_t)rChReal) << 32) | (((uint64_t)lChImag) << 16) | (uint64_t)(lChReal));

			}

		}
		// want to amplify HIGH frequency sounds
		else if (*equalizePart == 3){
			for (int i=40;i<60;i++) {
				rChImag = (MxBufferPtr[i] & 0xFFFF000000000000) >> 48;
				rChReal = (MxBufferPtr[i] & 0xFFFF00000000) >> 32;
				lChImag = (MxBufferPtr[i] & 0xFFFF0000) >> 16;
				lChReal = MxBufferPtr[i] & 0xFFFF;

				// shift to get correct values


				lChMag = sqrt(((double)lChImag*(double)lChImag) + ((double)lChReal*(double)lChReal));
				lChPhase = atan((double)lChImag/(double)lChReal);
				rChMag = sqrt((rChImag*rChImag) + (rChReal*rChReal));
				rChPhase = atan((double)rChImag/(double)rChReal);


//				// now need to increase magnitude by counter amount
//				if (*equalizeCounter > 0){
//					lChMag *= (*equalizeCounter); // so just going to add to it rn not sure what effect this will have
//					rChMag *= (*equalizeCounter);
//				}
//				else if (*equalizeCounter < 0){
//					lChMag /= (-1)*(*equalizeCounter);
//					rChMag /= (-1)*(*equalizeCounter);
//				}
//------------------------Equalizing will no longer be user manipulated - can only be "on" or "off" --------------------------------//


				lChMag *= 10;
				rChMag *= 10;
				//now need to convert back to real/imaginary
				lChReal = lChMag *cos(lChPhase);
				lChImag = lChMag *sin(lChPhase);
				rChReal = rChMag * cos(rChPhase);
				rChImag = rChMag * sin(rChPhase);

				// but back in MxBuf to be sent to IFFT
				MxBufferPtr[i] = ((((uint64_t)rChImag) << 48) | (((uint64_t)rChReal) << 32) | (((uint64_t)lChImag) << 16) | (uint64_t)(lChReal));

				rChImag = (MxBufferPtr[1023-i] & 0xFFFF000000000000) >> 48;
				rChReal = (MxBufferPtr[1023-i] & 0xFFFF00000000) >> 32;
				lChImag = (MxBufferPtr[1023-i] & 0xFFFF0000) >> 16;
				lChReal = MxBufferPtr[1023-i] & 0xFFFF;



				lChMag = sqrt(((double)lChImag*(double)lChImag) + ((double)lChReal*(double)lChReal));
				lChPhase = atan((double)lChImag/(double)lChReal);
				rChMag = sqrt((rChImag*rChImag) + (rChReal*rChReal));
				rChPhase = atan((double)rChImag/(double)rChReal);


				// now need to increase magnitude by counter amount
				// now need to increase magnitude by counter amount
				//				// now need to increase magnitude by counter amount
				//				if (*equalizeCounter > 0){
				//					lChMag *= (*equalizeCounter); // so just going to add to it rn not sure what effect this will have
				//					rChMag *= (*equalizeCounter);
				//				}
				//				else if (*equalizeCounter < 0){
				//					lChMag /= (-1)*(*equalizeCounter);
				//					rChMag /= (-1)*(*equalizeCounter);
				//				}
//------------------------Equalizing will no longer be user manipulated - can only be "on" or "off" --------------------------------//


				lChMag *= 10;
				rChMag *= 10;
				//now need to convert back to real/imaginary
				lChReal = lChMag * cos(lChPhase);
				lChImag = lChMag * sin(lChPhase);
				rChReal = rChMag * cos(rChPhase);
				rChImag = rChMag * sin(rChPhase);

				// but back in MxBuf to be sent to IFFT
				MxBufferPtr[511-i] = ((((uint64_t)rChImag) << 48) | (((uint64_t)rChReal) << 32) | (((uint64_t)lChImag) << 16) | (uint64_t)(lChReal));


			} // ends for loop

			for (int i=0;i<5;i++) {
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
//				if (*equalizeCounter > 0){
//					lChMag *= (*equalizeCounter); // so just going to add to it rn not sure what effect this will have
//					rChMag *= (*equalizeCounter);
//				}
//				else if (*equalizeCounter < 0){
//					lChMag /= (-1)*(*equalizeCounter);
//					rChMag /= (-1)*(*equalizeCounter);
//				}

//------------------------Equalizing will no longer be user manipulated - can only be "on" or "off" --------------------------------//

				lChMag /= 5;
				rChMag /= 5;
				//now need to convert back to real/imaginary
				lChReal = lChMag *cos(lChPhase);
				lChImag = lChMag *sin(lChPhase);
				rChReal = rChMag * cos(rChPhase);
				rChImag = rChMag * sin(rChPhase);

				// but back in MxBuf to be sent to IFFT
				MxBufferPtr[i] = ((((uint64_t)rChImag) << 48) | (((uint64_t)rChReal) << 32) | (((uint64_t)lChImag) << 16) | (uint64_t)(lChReal));

				rChImag = (MxBufferPtr[1023-i] & 0xFFFF000000000000) >> 48;
				rChReal = (MxBufferPtr[1023-i] & 0xFFFF00000000) >> 32;
				lChImag = (MxBufferPtr[1023-i] & 0xFFFF0000) >> 16;
				lChReal = MxBufferPtr[1023-i] & 0xFFFF;



				lChMag = sqrt(((double)lChImag*(double)lChImag) + ((double)lChReal*(double)lChReal));
				lChPhase = atan((double)lChImag/(double)lChReal);
				rChMag = sqrt((rChImag*rChImag) + (rChReal*rChReal));
				rChPhase = atan((double)rChImag/(double)rChReal);


				// now need to increase magnitude by counter amount
				// now need to increase magnitude by counter amount
//				if (*equalizeCounter > 0){
//					lChMag *= (*equalizeCounter); // so just going to add to it rn not sure what effect this will have
//					rChMag *= (*equalizeCounter);
//				}
//				else if (*equalizeCounter < 0){
//					lChMag /= (-1)*(*equalizeCounter);
//					rChMag /= (-1)*(*equalizeCounter);
//				}
//------------------------Equalizing will no longer be user manipulated - can only be "on" or "off" --------------------------------//

				lChMag /= 5;
				rChMag /= 5;
				//now need to convert back to real/imaginary
				lChReal = lChMag * cos(lChPhase);
				lChImag = lChMag * sin(lChPhase);
				rChReal = rChMag * cos(rChPhase);
				rChImag = rChMag * sin(rChPhase);

				// but back in MxBuf to be sent to IFFT
				MxBufferPtr[1023-i] = ((((uint64_t)rChImag) << 48) | (((uint64_t)rChReal) << 32) | (((uint64_t)lChImag) << 16) | (uint64_t)(lChReal));
			}

		} // ends if condition for which part to equalize
	} // ends if condition for if we want to equalize at all


} // ends function
