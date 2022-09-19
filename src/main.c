#include <stdio.h>
#include "platform.h"
#include "zed_hdmi_display.h"
#include "xparameters.h"


zed_hdmi_display_t demo;


void print( const char *str);

int main()
{
init_platform();

print("Hello World\n\r");
demo.uBaseAddr_IIC_HdmiOut = XPAR_ZED_HDMI_IIC_0_BASEADDR;
demo.uDeviceId_VTC_HdmioGenerator =XPAR_V_TC_0_DEVICE_ID;
demo.uDeviceId_VDMA_HdmiDisplay = XPAR_AXI_VDMA_0_DEVICE_ID;
demo.uBaseAddr_MEM_HdmiDisplay = XPAR_DDR_MEM_BASEADDR + 0x10000000;
demo.uNumFrames_HdmiDisplay = XPAR_AXIVDMA_0_NUM_FSTORES;
zed_hdmi_display_init( &demo );
return 0;
}
