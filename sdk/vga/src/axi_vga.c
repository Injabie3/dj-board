#include "axi_vga.h"


static XAxiDma AxiDma;		/* Instance of the XAxiDma */

static volatile int Error;			/* DMA Error flag */
static volatile u16 *FramePtr = NULL;
static volatile u8 newFrame = 0;

/* Double frame buffer */
static u16 * Frame1 = (u16 *)FRAME1_BASE;
static u16 * Frame2 = (u16 *)FRAME2_BASE;

static int DMAConfig(void);
static void FrameInit(void);

static void FrameIntrHandler(void *Callback);

/*****************************************************************************/
/**
*
* This function initializes and configures the AXI VGA
*
* @param	None.
*
* @return
*		- XST_SUCCESS if successful,
*		- XST_FAILURE.if not successful
*
* @note		None.
*
******************************************************************************/
int InitVGA(void)
{
	int Status;
	Error = 0;

	Status = DMAConfig();

	if (Status != XST_SUCCESS)
	{
		xil_printf("DMA configuration failed %d\r\n", Status);
		return XST_FAILURE;
	}

	FrameInit();

	Status = XAxiDma_SimpleTransfer(&AxiDma, (UINTPTR) FramePtr, FRAME_LEN,
			XAXIDMA_DMA_TO_DEVICE);

	if (Status != XST_SUCCESS)
	{
		xil_printf("Failed initial transfer... %d\r\n", Status);
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function configures the DMA engine
*
* @param	None.
*
* @return
*		- XST_SUCCESS if successful,
*		- XST_FAILURE.if not successful
*
* @note		None.
*
******************************************************************************/
int DMAConfig(void)
{
	XAxiDma_Config *config;

	config = XAxiDma_LookupConfig(DMA_DEV_ID);

	if (!config)
	{
		xil_printf("No config found for %d\r\n", DMA_DEV_ID);

		return XST_FAILURE;
	}

	/* Initialize DMA engine */
	int Status = XAxiDma_CfgInitialize(&AxiDma, config);

	if (Status != SUCCESS)
	{
		xil_printf("Initialization failed %d\r\n", Status);
		return FAILURE;
	}

	/* Disable all interrupts before setup */
	XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);

	/* Set up Interrupt system  */
	Status = EnableInterrupts(&AxiDma, TX_INTR_ID, FrameIntrHandler, 0xA0);
	if (Status != SUCCESS)
	{
		xil_printf("Failed intr setup\r\n");
		return FAILURE;
	}

	/* Enable all interrupts */
	XAxiDma_IntrEnable(&AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);

	return SUCCESS;
}

void FrameInit(void)
{
	//Xil_DCacheFlushRange((UINTPTR)Frame1, FRAME_LEN);
	//Xil_DCacheFlushRange((UINTPTR)Frame2, FRAME_LEN);
	//memset(Frame1, 0, FRAME_LEN);
	memset(Frame2, 0, FRAME_LEN);

	FramePtr = Frame1;
}

/*****************************************************************************/
/*
*
* This is the DMA TX Interrupt handler function.
*
* It gets the interrupt status from the hardware, acknowledges it, and if any
* error happens, it resets the hardware. Otherwise, if a completion interrupt
* is present, then starts a new transfer
*
* @param	Callback is a pointer to TX channel of the DMA engine.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void FrameIntrHandler(void *Callback)
{
	u32 IrqStatus;
	int TimeOut;
	XAxiDma *AxiDmaInst = (XAxiDma *)Callback;

	/* Read pending interrupts */
	IrqStatus = XAxiDma_IntrGetIrq(AxiDmaInst, XAXIDMA_DMA_TO_DEVICE);

	/* Acknowledge pending interrupts */
	XAxiDma_IntrAckIrq(AxiDmaInst, IrqStatus, XAXIDMA_DMA_TO_DEVICE);

	/* If no interrupt is asserted, we do not do anything */
	if (!(IrqStatus & XAXIDMA_IRQ_ALL_MASK))
	{
		return;
	}

	/*
	 * If error interrupt is asserted, raise error flag, reset the
	 * hardware to recover from the error, and return with no further
	 * processing.
	 */
	if ((IrqStatus & XAXIDMA_IRQ_ERROR_MASK))
	{
		Error = 1;

		/* Reset should never fail for transmit channel */
		XAxiDma_Reset(AxiDmaInst);

		TimeOut = RESET_TIMEOUT_COUNTER;

		while (TimeOut)
		{
			if (XAxiDma_ResetIsDone(AxiDmaInst))
			{
				break;
			}

			TimeOut -= 1;
		}

		return;
	}

	/* If Completion interrupt is asserted, then start new transfer */
	if ((IrqStatus & XAXIDMA_IRQ_IOC_MASK))
	{
		XAxiDma_SimpleTransfer(&AxiDma, (UINTPTR) FramePtr, FRAME_LEN, XAXIDMA_DMA_TO_DEVICE);

		if (newFrame)
		{
			FramePtr = newFrame == 1 ? Frame1 : Frame2;
			Xil_DCacheFlushRange((UINTPTR)FramePtr, FRAME_LEN);
		}
	}
}

/*****************************************************************************/
/**
*
* This function sends new image data to the VGA
*
* @param	IntcInstancePtr is the pointer to the INTC component instance
* @param	TxIntrId is interrupt ID associated w/ DMA TX channel
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void DrawFrame(u16 * img)
{
	newFrame = 0;

	if(FramePtr == Frame1)
	{
		memcpy((void *)Frame2, (void *)img, FRAME_LEN);
		newFrame = 2;
	}
	else
	{
		memcpy((void *)Frame1, (void *)img, FRAME_LEN);
		newFrame = 1;
	}
}

void DestroyVGA(void)
{
	DisableInterrupt(TX_INTR_ID);
}

