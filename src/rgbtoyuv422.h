#include "xvprocss.h"
#include "zed_hdmi_display.h"
/************************** Constant Definitions *****************************/
typedef enum
{
  XSYS_VPSS_STREAM_IN = 0,
  XSYS_VPSS_STREAM_OUT
}XSys_StreamDirection;

int XSys_Init(XVprocSs *pVprocss);
int setup_video_io(zed_hdmi_display_t *pDemo, XVprocSs *VpssPtr);

int start_system(XVprocSs *VpssPtr);

