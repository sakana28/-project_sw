/* Description: Data transfer via DMA to Sobel user IP*/



#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xaxidma.h"
#include "xparameters.h"
#include "xil_exception.h"
#include "xscugic.h"
#include "SDoperation.h"
#include "sobel.h"
#include "xtime_l.h"

/************************** Constant Definitions *****************************/

/*
 * Device hardware build related constants.
 */

#define DMA_DEV_ID		XPAR_AXIDMA_0_DEVICE_ID
#define MEM_BASE_ADDR		(XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x10000000)

#define SOBEL_INTR_ID		XPAR_FABRIC_SOBEL_V1_0_0_INTR_O_INTR
#define S2MM_INTR_ID		XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR

#define TX_BUFFER_BASE		(MEM_BASE_ADDR + 0x00100000)
#define RX_BUFFER_BASE		(MEM_BASE_ADDR + 0x00300000)

#define INTC_DEVICE_ID          XPAR_SCUGIC_SINGLE_DEVICE_ID

#define INTC		XScuGic
#define INTC_HANDLER	XScuGic_InterruptHandler

/************************** Function Prototypes ******************************/
static void TxIntrHandler(void *Callback);
static void RxIntrHandler(void *Callback);
static int SetupIntrSystem(INTC * IntcInstancePtr, XAxiDma * AxiDmaPtr,
		u16 TxIntrId, u16 RxIntrId);
static void DisableIntrSystem(INTC * IntcInstancePtr, u16 TxIntrId,
		u16 RxIntrId);
/************************** Variable Definitions *****************************/
/*Device instance definitions*/

static XAxiDma AxiDma; /* Instance of the XAxiDma */

static INTC Intc; /* Instance of the Interrupt Controller */

/*Flags interrupt handlers use to notify the application context the events.*/

volatile int Done;
static int zero_width;
static int zero_height;
static int line_count;
extern u32 data_buffer[400000];
extern u32 sobel_buffer[400000];

static XTime t_begin, t_end;
static u32 t_used;

/*****************************************************************************/

int sobel_setup(bmp_meta *picsrc) {
	int Status;
	XAxiDma_Config *Config;

	xil_printf("\r\n--- Entering main() --- \r\n");

	Config = XAxiDma_LookupConfig(DMA_DEV_ID);
	if (!Config) {
		xil_printf("No config found for %d\r\n", DMA_DEV_ID);

		return XST_FAILURE;
	}

	/* Initialize DMA engine */
	Status = XAxiDma_CfgInitialize(&AxiDma, Config);

	if (Status != XST_SUCCESS) {
		xil_printf("Initialization failed %d\r\n", Status);
		return XST_FAILURE;
	}
	/* Get BMP meta data */
	readBMPhead(picsrc);
	zero_width = *(picsrc->bmp_width) + 2;
	zero_height = *(picsrc->bmp_height) + 2;
	xil_printf("zero width:%d\r\n zero height:%d\r\n", zero_width, zero_height);
	/* Width configuration */
	SOBEL_mWriteReg(XPAR_SOBEL_V1_0_0_BASEADDR, SOBEL_S00_AXI_SLV_REG0_OFFSET,
			(u32 )zero_width);
	/* Load image from SD to DDR */
	load_sd_bmp(picsrc, data_buffer);
	Xil_DCacheFlushRange((UINTPTR) data_buffer, 4 * zero_width * zero_height);

	/* Set up Interrupt system  */
	Status = SetupIntrSystem(&Intc, &AxiDma, SOBEL_INTR_ID, S2MM_INTR_ID);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed intr setup\r\n");
		return XST_FAILURE;
	}

	/* Disable all interrupts before setup */

	XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);

	XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);

	/* Enable all interrupts */

	XAxiDma_IntrEnable(&AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);

	/* Initialize flags before start transfer test  */
	Done = 0;
	/* get begin time*/
	XTime_GetTime(&t_begin);
	/* start transfer*/
	Status = XAxiDma_SimpleTransfer(&AxiDma, (UINTPTR) sobel_buffer,
			(u32) 4 * (zero_width - 2) * (zero_height - 2),
			XAXIDMA_DEVICE_TO_DMA);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed to initialize DMA RX\r\n");
		return XST_FAILURE;
	}
	xil_printf("set receive request\r\n");
	Status = XAxiDma_SimpleTransfer(&AxiDma, (UINTPTR) data_buffer,
			4 * zero_height * zero_width, XAXIDMA_DMA_TO_DEVICE);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed to initialize DMA TX\r\n");
		return XST_FAILURE;
	}
	xil_printf("set send request\r\n");
	line_count = 4;
	//XScuGic_Enable(&Intc, SOBEL_INTR_ID); Optional
	while (!Done) {
		/* NOP */
	}
	xil_printf("back to main\r\n");
	Xil_DCacheFlushRange((UINTPTR) sobel_buffer,
			4 * (zero_width - 2) * (zero_height - 2));

	xil_printf("Successfully ran AXI DMA interrupt Example\r\n");

	/* Disable TX and RX Ring interrupts and return success */

	DisableIntrSystem(&Intc, SOBEL_INTR_ID, S2MM_INTR_ID);
	t_used = ((t_end - t_begin) * 1000000000) / (COUNTS_PER_SECOND);
	xil_printf("time elapsed is %d ns\r\n", t_used);
	xil_printf("--- Exiting main() --- \r\n");

	return XST_SUCCESS;
}

/*****************************************************************************/
/*
 *
 * This is the DMA TX Interrupt handler function.
 *
 * It gets the interrupt status from the hardware, acknowledges it, and if any
 * error happens, it resets the hardware. Otherwise, if a completion interrupt
 * is present, then sets the TxDone.flag
 *
 * @param	Callback is a pointer to TX channel of the DMA engine.
 *
 * @return	None.
 *
 * @note		None.
 *
 ******************************************************************************/
static void TxIntrHandler(void *Callback) {
	int Status;
	XAxiDma *AxiDmaInst = (XAxiDma *) Callback;
	XScuGic_Disable(&Intc, SOBEL_INTR_ID);
	//xil_printf("GET SOBEL INTR %d\r\n",line_count);
	while (XAxiDma_Busy(&AxiDma, XAXIDMA_DMA_TO_DEVICE)) {
		/* Wait */
	}
	if (line_count <= zero_height) {
		Status = XAxiDma_SimpleTransfer(AxiDmaInst,
				(UINTPTR) &(data_buffer[line_count * zero_width]),
				4 * zero_width,
				XAXIDMA_DMA_TO_DEVICE);
		if (Status != XST_SUCCESS) {

			xil_printf("ERROR in DMA TX\r\n");
			return XST_FAILURE;
		}
		line_count++;
	}
	XScuGic_Enable(&Intc, SOBEL_INTR_ID);
}

/*****************************************************************************/
/*
 *
 * This is the DMA RX interrupt handler function
 *
 * It gets the interrupt status from the hardware, acknowledges it, and if any
 * error happens, it resets the hardware. Otherwise, if a completion interrupt
 * is present, then it sets the RxDone flag.
 *
 * @param	Callback is a pointer to RX channel of the DMA engine.
 *
 * @return	None.
 *
 * @note		None.
 *
 ******************************************************************************/
static void RxIntrHandler(void *Callback) {
	u32 IrqStatus;
	XAxiDma *AxiDmaInst = (XAxiDma *) Callback;
	xil_printf("GET S2MM INTR\r\n line count:%d\n", line_count);
	XTime_GetTime(&t_end);
	/* Read pending interrupts */
	IrqStatus = XAxiDma_IntrGetIrq(AxiDmaInst, XAXIDMA_DEVICE_TO_DMA);
	/* Acknowledge pending interrupts */
	XAxiDma_IntrAckIrq(AxiDmaInst, IrqStatus, XAXIDMA_DEVICE_TO_DMA);
	if ((IrqStatus & XAXIDMA_IRQ_IOC_MASK)) {

		Done = 1;
		xil_printf("set done\n");
		XScuGic_Disable(&Intc, SOBEL_INTR_ID);
	}

	else {
		xil_printf("Error in DMA RX \r\n");
	}
}

/*****************************************************************************/
/*
 *
 * This function setups the interrupt system so interrupts can occur for the
 * DMA, it assumes INTC component exists in the hardware system.
 *
 * @param	IntcInstancePtr is a pointer to the instance of the INTC.
 * @param	AxiDmaPtr is a pointer to the instance of the DMA engine
 * @param	TxIntrId is the TX channel Interrupt ID.
 * @param	RxIntrId is the RX channel Interrupt ID.
 *
 * @return
 *		- XST_SUCCESS if successful,
 *		- XST_FAILURE.if not successful
 *
 * @note		None.
 *
 ******************************************************************************/
static int SetupIntrSystem(INTC * IntcInstancePtr, XAxiDma * AxiDmaPtr,
		u16 TxIntrId, u16 RxIntrId) {
	int Status;

	XScuGic_Config *IntcConfig;

	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use.
	 */
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig,
			IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	XScuGic_SetPriorityTriggerType(IntcInstancePtr, TxIntrId, 0xA0, 0x3);

	XScuGic_SetPriorityTriggerType(IntcInstancePtr, RxIntrId, 0x00, 0x3);
	/*
	 * Connect the device driver handler that will be called when an
	 * interrupt for the device occurs, the handler defined above performs
	 * the specific interrupt processing for the device.
	 */
	Status = XScuGic_Connect(IntcInstancePtr, TxIntrId,
			(Xil_InterruptHandler) TxIntrHandler, AxiDmaPtr);
	if (Status != XST_SUCCESS) {
		return Status;
	}

	Status = XScuGic_Connect(IntcInstancePtr, RxIntrId,
			(Xil_InterruptHandler) RxIntrHandler, AxiDmaPtr);
	if (Status != XST_SUCCESS) {
		return Status;
	}

	XScuGic_Enable(IntcInstancePtr, RxIntrId);

	/* Enable interrupts from the hardware */

	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			(Xil_ExceptionHandler) INTC_HANDLER, (void *) IntcInstancePtr);

	Xil_ExceptionEnable()
	;

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
 *
 * This function disables the interrupts for DMA engine.
 *
 * @param	IntcInstancePtr is the pointer to the INTC component instance
 * @param	TxIntrId is interrupt ID associated w/ DMA TX channel
 * @param	RxIntrId is interrupt ID associated w/ DMA RX channel
 *
 * @return	None.
 *
 * @note		None.
 *
 ******************************************************************************/
static void DisableIntrSystem(INTC * IntcInstancePtr, u16 TxIntrId,
		u16 RxIntrId) {

	XScuGic_Disconnect(IntcInstancePtr, TxIntrId);
	XScuGic_Disconnect(IntcInstancePtr, RxIntrId);

}

