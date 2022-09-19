#include <stdio.h>
#include "xil_cache.h"
#include "xparameters.h"
#include "xparameters_ps.h"
#include "rgbtoyuv422.h"
#include "xvprocss_vdma.h"
#include "xvprocss.h"
#include "xvidc.h"
#include "zed_hdmi_display.h"

/************************** Local Constants *********************************/
#define XVPROCSS_SW_VER "v2.00"
#define VERBOSE_MODE 0
#define TOPOLOGY_COUNT 6
#define USECASE_COUNT 2
#define VIDEO_MONITOR_LOCK_TIMEOUT (2000000)
#define DDR_BASEADDR XPAR_DDR_MEM_BASEADDR
#define USR_FRAME_BUF_BASEADDR     (DDR_BASEADDR + (0x20000000))

#define PROC_DELAY 2

/* Canonical definitions for peripheral RGB_TO_YUV422_RESET_SEL_AXIS */
#define GPIO_BASE XPAR_GPIO_0_BASEADDR


#define VDMA_BASE XPAR_AXI_VDMA_0_BASEADDR



/************************** Local Routines **********************************/

/*
* This function is the system level initialization routine. It in turn calls
* each of the included subsystem initialization function
*
* @param	pPeriph is a pointer to the peripherals instance
* @param	pVprocss is a pointer to the video proc subsystem instance
*
* @return
*		- XST_SUCCESS if init. was successful.
*		- XST_FAILURE if init. was unsuccessful.
*
* @note		None.
*
******************************************************************************/
int XSys_Init(XVprocSs *pVprocss)
{
  int status;
  XVprocSs_Config *VprocSsConfigPtr;

  Xil_AssertNonvoid(pVprocss != NULL);

  //Init all instance variables to 0
  memset(pVprocss, 0, sizeof(XVprocSs));

  VprocSsConfigPtr = XVprocSs_LookupConfig(XPAR_XVPROCSS_0_DEVICE_ID);
  if(VprocSsConfigPtr == NULL)
  {
	xil_printf("ERR:: VprocSs device not found\r\n");
    return (XST_DEVICE_NOT_FOUND);
  }
   xil_printf("VPSS power on successful\r\n");
  switch (VprocSsConfigPtr->Topology)
  {
    case XVPROCSS_TOPOLOGY_FULL_FLEDGED:
    case XVPROCSS_TOPOLOGY_DEINTERLACE_ONLY:
	  XVprocSs_SetFrameBufBaseaddr(pVprocss, USR_FRAME_BUF_BASEADDR);
      break;

    default:
      break;
  }

  /* Start capturing event log. */
  XVprocSs_LogReset(pVprocss);

  status = XVprocSs_CfgInitialize(pVprocss,
		                          VprocSsConfigPtr,
		                          VprocSsConfigPtr->BaseAddress);

  if(status != XST_SUCCESS)
  {
	 xil_printf("ERR:: Video Processing Subsystem Init. error\n\r");
	 return(XST_FAILURE);
  }

  return(status);
}
int XSys_SetStreamParam(XVprocSs *pVprocss, u16 Direction, u16 Width,
			u16 Height, XVidC_FrameRate FrameRate, XVidC_ColorFormat cfmt, u16 IsInterlaced)
{
	XVidC_VideoMode resId;
	XVidC_VideoStream Stream;
	XVidC_VideoTiming const *TimingPtr;

	//resId = XVidC_GetVideoModeId(Width, Height, FrameRate, IsInterlaced);
	resId = XVIDC_VM_1920x1080_60_P;
	if (resId == XVIDC_VM_NOT_SUPPORTED)
	{
		xil_printf("ERR:: XVIDC_VM_NOT_SUPPORTED\n\r");
		return XST_INVALID_PARAM;
	}

	TimingPtr = XVidC_GetTimingInfo(resId);

	/* Setup Video Processing Subsystem */
	Stream.VmId           = resId;
	Stream.Timing         = *TimingPtr;
	Stream.ColorFormatId  = cfmt;
	Stream.ColorDepth     = pVprocss->Config.ColorDepth;
	Stream.PixPerClk      = pVprocss->Config.PixPerClock;
	Stream.FrameRate      = FrameRate;
	Stream.IsInterlaced   = FALSE;

	if (Direction == XSYS_VPSS_STREAM_IN)
		XVprocSs_SetVidStreamIn(pVprocss, &Stream);
	else
		XVprocSs_SetVidStreamOut(pVprocss, &Stream);

	return XST_SUCCESS;
}

/************************** Variable Definitions *****************************/


int setup_video_io( zed_hdmi_display_t *pDemo, XVprocSs *VpssPtr)
{

	int status = XST_FAILURE;




	/*
	 * Set VPSS Video Input AXI Stream to match the TPG
	 * Note that framerate is hardwired to 60Hz in the example design
	 */

	/*int XSys_SetStreamParam(XVprocSs *pVprocss, u16 Direction, u16 Width,
				u16 Height, XVidC_FrameRate FrameRate,
				XVidC_ColorFormat cfmt, u16 IsInterlaced)*/
	status = XSys_SetStreamParam(VpssPtr, XSYS_VPSS_STREAM_IN,
			pDemo->hdmio_width,
			pDemo->hdmio_height, XVIDC_FR_60HZ,
			XVIDC_CSF_RGB, FALSE);
	if (status != XST_SUCCESS)
      xil_printf( "Fail to setup video input\n\r" );


	/*
	 * Set VPSS Video Output AXI Stream
	 * Note that output video is always progressive
	 */
	status = XSys_SetStreamParam(VpssPtr, XSYS_VPSS_STREAM_OUT,
			pDemo->hdmio_width, pDemo->hdmio_height,
				     XVIDC_FR_60HZ, XVIDC_CSF_YCRCB_422,
				     FALSE);
	if (status != XST_SUCCESS)
	{
	      xil_printf( "Fail to setup video output\n\r" );
		return XST_FAILURE;
	}

	return status;
}

/*****************************************************************************/
/**
*
* @local routine start_system()
*
*  Configure and Start the video system.
*
*  @return Returns XST_SUCCESS for successful configuration, else XST_FAILURE.
*
******************************************************************************/
int start_system(XVprocSs *VpssPtr)
{
    int status;


      // Configure and Start the VPSS IP
      // (reset logic for multi-IP VPSS cases is done here)
      status = XVprocSs_SetSubsystemConfig(VpssPtr);
  	if (status != XST_SUCCESS)
  	{
  	      xil_printf( "Fail to start system\n\r" );
  		return XST_FAILURE;
  	}
  	xil_printf("VPSS subsystem Configure successful\r\n");
      return status;
}

/*****************************************************************************/
