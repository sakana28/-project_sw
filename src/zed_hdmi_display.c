//----------------------------------------------------------------
//      _____
//     /     \
//    /____   \____
//   / \===\   \==/
//  /___\===\___\/  AVNET
//       \======/
//        \====/
//---------------------------------------------------------------
//
// This design is the property of Avnet.  Publication of this
// design is not authorized without written consent from Avnet.
//
// Please direct any questions to:  technical.support@avnet.com
//
// Disclaimer:
//    Avnet, Inc. makes no warranty for the use of this code or design.
//    This code is provided  "As Is". Avnet, Inc assumes no responsibility for
//    any errors, which may appear in this code, nor does it make a commitment
//    to update the information contained herein. Avnet, Inc specifically
//    disclaims any implied warranties of fitness for a particular purpose.
//                     Copyright(c) 2014 Avnet, Inc.
//                             All rights reserved.
//
//----------------------------------------------------------------
//
// Create Date:         Oct 24, 2012
// Design Name:         FMC-IMAGEON HDMI Video Frame Buffer
// Module Name:         zed_hdmi_display.c
// Project Name:        FMC-IMAGEON HDMI Video Frame Buffer Program
// Target Devices:      Spartan-6, Virtex-6, Kintex-6
// Hardware Boards:     FMC-IMAGEON
// 
//
// Tool versions:       Vivado 2013.3
//
// Description:         FMC-IMAGEON HDMI Display Controller Program
//                      This application will configure the ADV7611 on the FMC-IMAGEON module
//                      - HDMI Output
//                         - ADV7511 configured for 16 bit YCbCr 4:2:2 mode
//                           with embedded syncs
//                      It will also configure the Video Timing Controller
//                      to generate the video timing.
//
// Dependencies:
//
// Revision:            Feb 26, 2014: 1.00 Initial version
//
//----------------------------------------------------------------

#include <stdio.h>

#include "zed_hdmi_display.h"
#include "SDoperation.h"
#include"sobel_dma.h"
#define ADV7511_ADDR   0x72
#define CARRIER_HDMI_OUT_CONFIG_LEN  (40)
Xuint8 carrier_hdmi_out_config[CARRIER_HDMI_OUT_CONFIG_LEN][3] = { {
		ADV7511_ADDR >> 1, 0x15, 0x01 }, // Input YCbCr 4:2:2 with seperate syncs
		{ ADV7511_ADDR >> 1, 0x16, 0x38 }, // Output format 444, Input Color Depth = 8
										   //    R0x16[  7] = Output Video Format = 0 (444)
										   //    R0x16[5:4] = Input Video Color Depth = 11 (8 bits/color)
										   //    R0x16[3:2] = Input Video Style = 10 (style 1)
										   //    R0x16[  1] = DDR Input Edge = 0 (falling edge)
										   //    R0x16[  0] = Output Color Space = 0 (RGB)

		// HDTV YCbCr (16to235) to RGB (16to235)
		{ ADV7511_ADDR >> 1, 0x18, 0xAC }, { ADV7511_ADDR >> 1, 0x19, 0x53 }, {
		ADV7511_ADDR >> 1, 0x1A, 0x08 }, { ADV7511_ADDR >> 1, 0x1B, 0x00 }, {
				ADV7511_ADDR >> 1, 0x1C, 0x00 }, {
		ADV7511_ADDR >> 1, 0x1D, 0x00 }, { ADV7511_ADDR >> 1, 0x1E, 0x19 }, {
				ADV7511_ADDR >> 1, 0x1F, 0xD6 }, {
		ADV7511_ADDR >> 1, 0x20, 0x1C }, { ADV7511_ADDR >> 1, 0x21, 0x56 }, {
				ADV7511_ADDR >> 1, 0x22, 0x08 }, {
		ADV7511_ADDR >> 1, 0x23, 0x00 }, { ADV7511_ADDR >> 1, 0x24, 0x1E }, {
				ADV7511_ADDR >> 1, 0x25, 0x88 }, {
		ADV7511_ADDR >> 1, 0x26, 0x02 }, { ADV7511_ADDR >> 1, 0x27, 0x91 }, {
				ADV7511_ADDR >> 1, 0x28, 0x1F }, {
		ADV7511_ADDR >> 1, 0x29, 0xFF }, { ADV7511_ADDR >> 1, 0x2A, 0x08 }, {
				ADV7511_ADDR >> 1, 0x2B, 0x00 }, {
		ADV7511_ADDR >> 1, 0x2C, 0x0E }, { ADV7511_ADDR >> 1, 0x2D, 0x85 }, {
				ADV7511_ADDR >> 1, 0x2E, 0x18 }, {
		ADV7511_ADDR >> 1, 0x2F, 0xBE },

		{ ADV7511_ADDR >> 1, 0x41, 0x10 }, // Power down control
										   //    R0x41[  6] = PowerDown = 0 (power-up)
		{ ADV7511_ADDR >> 1, 0x48, 0x08 }, // Video Input Justification
										   //    R0x48[8:7] = Video Input Justification = 01 (right justified)
		{ ADV7511_ADDR >> 1, 0x55, 0x00 }, // Set RGB in AVinfo Frame
										   //    R0x55[6:5] = Output Format = 00 (RGB)
		{ ADV7511_ADDR >> 1, 0x56, 0x28 }, // Aspect Ratio
										   //    R0x56[5:4] = Picture Aspect Ratio = 10 (16:9)
										   //    R0x56[3:0] = Active Format Aspect Ratio = 1000 (Same as Aspect Ratio)
		{ ADV7511_ADDR >> 1, 0x98, 0x03 }, // ADI Recommended Write
		{ ADV7511_ADDR >> 1, 0x9A, 0xE0 }, // ADI Recommended Write
		{ ADV7511_ADDR >> 1, 0x9C, 0x30 }, // PLL Filter R1 Value
		{ ADV7511_ADDR >> 1, 0x9D, 0x61 }, // Set clock divide
		{ ADV7511_ADDR >> 1, 0xA2, 0xA4 }, // ADI Recommended Write
		{ ADV7511_ADDR >> 1, 0xA3, 0xA4 }, // ADI Recommended Write
		{ ADV7511_ADDR >> 1, 0xAF, 0x04 }, // HDMI/DVI Modes
										   //    R0xAF[  7] = HDCP Enable = 0 (HDCP disabled)
										   //    R0xAF[  4] = Frame Encryption = 0 (Current frame NOT HDCP encrypted)
										   //    R0xAF[3:2] = 01 (fixed)
										   //    R0xAF[  1] = HDMI/DVI Mode Select = 0 (DVI Mode)
		//{ADV7511_ADDR>>1, 0xBA, 0x00}, // Programmable delay for input video clock = 000 = -1.2ns
		{ ADV7511_ADDR >> 1, 0xBA, 0x60 }, // Programmable delay for input video clock = 011 = no delay
		{ ADV7511_ADDR >> 1, 0xE0, 0xD0 }, // Must be set to 0xD0 for proper operation
		{ ADV7511_ADDR >> 1, 0xF9, 0x00 } // Fixed I2C Address (This should be set to a non-conflicting I2C address)
};
static bmp_meta picsrc;
u32 data_buffer[400000] = { 0 };
u32 sobel_buffer[400000] = { 0 };
int zed_hdmi_display_init(zed_hdmi_display_t *pDemo) {
	int ret;
	u32 iterations = 0;

	xil_printf("\n\r");
	xil_printf("------------------------------------------------------\n\r");
	xil_printf("--        ZedBoard HDMI Display Controller          --\n\r");
	xil_printf("------------------------------------------------------\n\r");
	xil_printf("\n\r");

	// Set HDMI output to 1080P60 resolution
	pDemo->hdmio_resolution = VIDEO_RESOLUTION_1080P;
	pDemo->hdmio_width = 1920;
	pDemo->hdmio_height = 1080;

	xil_printf("HDMI IIC Initialization ...\n\r");
	ret = zed_iic_axi_init(&(pDemo->hdmi_out_iic), "ZED HDMI I2C Controller",
			pDemo->uBaseAddr_IIC_HdmiOut);
	if (!ret) {
		xil_printf("ERROR : Failed to initialize IIC driver\n\r");
		return -1;
	}
	xil_printf("HDMI Output Initialization ...\n\r");
	{
		Xuint8 num_bytes;
		int i;

		for (i = 0; i < CARRIER_HDMI_OUT_CONFIG_LEN; i++) {
			//xil_printf( "[ZedBoard HDMI] IIC Write - Device = 0x%02X, Address = 0x%02X, Data = 0x%02X\n\r", carrier_hdmi_out_config[i][0]<<1, carrier_hdmi_out_config[i][1], carrier_hdmi_out_config[i][2] );
			num_bytes = pDemo->hdmi_out_iic.fpIicWrite(&(pDemo->hdmi_out_iic),
					carrier_hdmi_out_config[i][0],
					carrier_hdmi_out_config[i][1],
					&(carrier_hdmi_out_config[i][2]), 1);
		}
	}

	// Clear frame stores
	xil_printf("Clear Frame Buffer\n\r");
	zed_hdmi_display_clear(pDemo);

	// Initialize Output Side of AXI VDMA
	xil_printf("Video DMA (Output Side) Initialization ...\n\r");
	vfb_common_init(pDemo->uDeviceId_VDMA_HdmiDisplay,     // uDeviceId
			&(pDemo->vdma_hdmi)                    // pAxiVdma
			);
	vfb_tx_init(&(pDemo->vdma_hdmi),                   // pAxiVdma
			&(pDemo->vdmacfg_hdmi_read),           // pReadCfg
			pDemo->hdmio_resolution,               // uVideoResolution
			pDemo->hdmio_resolution,               // uStorageResolution
			pDemo->uBaseAddr_MEM_HdmiDisplay,      // uMemAddr
			pDemo->uNumFrames_HdmiDisplay          // uNumFrames
			);

	// Configure VTC on output data path
	xil_printf("Video Timing Controller (generator) Initialization ...\n\r");
	vgen_init(&(pDemo->vtc_hdmio_generator),
			pDemo->uDeviceId_VTC_HdmioGenerator);
	vgen_config(&(pDemo->vtc_hdmio_generator), pDemo->hdmio_resolution, 1);

	// Configure VPSS on output data path
	xil_printf("VPSS Initialization ...\n\r");

	XSys_Init(&(pDemo->VpssPtr));
	setup_video_io(&pDemo, &(pDemo->VpssPtr));
	start_system(&(pDemo->VpssPtr));
	sobel_setup(&picsrc);

	while (1) {
		if (iterations > 0) {
			xil_printf("\n\rPress ENTER to re-start ...\n\r");
			getchar();
		}
		iterations++;

		// Display Color Bars
		xil_printf("Generate Color Bars\n\r");
		vfb_tx_stop(&(pDemo->vdma_hdmi));
		zed_hdmi_display_clear(pDemo);
		zed_hdmi_display_cbars(pDemo);
		vfb_tx_start(&(pDemo->vdma_hdmi));

		xil_printf("HDMI Output Re-Initialization ...\n\r");
		{
			Xuint8 num_bytes;
			int i;

			for (i = 0; i < CARRIER_HDMI_OUT_CONFIG_LEN; i++) {
				//xil_printf( "[ZedBoard HDMI] IIC Write - Device = 0x%02X, Address = 0x%02X, Data = 0x%02X\n\r", carrier_hdmi_out_config[i][0]<<1, carrier_hdmi_out_config[i][1], carrier_hdmi_out_config[i][2] );
				num_bytes = pDemo->hdmi_out_iic.fpIicWrite(
						&(pDemo->hdmi_out_iic), carrier_hdmi_out_config[i][0],
						carrier_hdmi_out_config[i][1],
						&(carrier_hdmi_out_config[i][2]), 1);
			}
		}

#if 0 // Activate for debug
		sleep(1);
		// Status of AXI VDMA
		vfb_dump_registers( &(pDemo->vdma_hdmi) );
		if ( vfb_check_errors( &(pDemo->vdma_hdmi), 1/*clear errors, if any*/) )
		{
			vfb_dump_registers( &(pDemo->vdma_hdmi) );
		}
#endif

		xil_printf("\n\r");
		xil_printf("Done\n\r");
		xil_printf("\n\r");
	}

	return 0;
}

int zed_hdmi_display_clear(zed_hdmi_display_t *pDemo) {
	// Clear frame stores
	xil_printf("Video Frame Buffer Initialization ...\n\r");
	u32 frame, row, col;
	u32 pixel;
	volatile u32 *pStorageMem = (u32 *) pDemo->uBaseAddr_MEM_HdmiDisplay;
	for (frame = 0; frame < pDemo->uNumFrames_HdmiDisplay; frame++) {
		for (row = 0; row < pDemo->hdmio_height; row++) {
			for (col = 0; col < pDemo->hdmio_width; col++) {
				//pixel = 0x00000000; // Black
				pixel = 0x00FFFFFF; // White
				*pStorageMem++ = pixel;
			}
		}
	}
	return 0;
}



int zed_hdmi_display_cbars(zed_hdmi_display_t *pDemo) {

	volatile u32 *pStorageMem = (u32 *) pDemo->uBaseAddr_MEM_HdmiDisplay;
	draw_frame(pStorageMem,data_buffer,sobel_buffer,pDemo->hdmio_width, pDemo->hdmio_height,&picsrc);
	// Wait for DMA to synchronize.
	Xil_DCacheFlush();

	return 0;
}