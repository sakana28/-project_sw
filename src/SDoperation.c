#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "integer.h"
#include "string.h"
#include "ff.h"
#include "SDoperation.h"
#include "math.h"

static FIL filsrc;
static FIL fildes;
static FATFS fatfs;
static u8 zero = 0X00;
static u8* ZeroAddr = &zero;

void readBMPhead(bmp_meta *pic) {

	UINT br;
	FRESULT status;
	BYTE work[FF_MAX_SS];
	status = f_mount(&fatfs, "", 0);
	if (status != FR_OK) {
		xil_printf("volume is not FAT format\n");

		f_mkfs("", FM_FAT32, 0, work, sizeof work);
		f_mount(&fatfs, "", 0);
	}

	f_open(&filsrc, "lena.bmp", FA_READ);
	f_lseek(&filsrc, 0);
	f_read(&filsrc, pic->bmp_head, 54, &br);
	xil_printf("Bmp head: \n\r");
	pic->bmp_width = (UINT *) (pic->bmp_head + 0x12);
	pic->bmp_height = (UINT *) (pic->bmp_head + 0x16);
	pic->rowsize = floor((24 * (*(pic->bmp_width)) + 31) / 32) * 4;
	xil_printf("\n width = %d, height = %d, row size = %d bytes \n\r",
			*(pic->bmp_width), *(pic->bmp_height), pic->rowsize);
	f_close(&filsrc);
}

void load_sd_bmp(bmp_meta *pic, u32 *data_buffer) {

	UINT br;
	int i;
	int j;
	int height;
	int width;
	int rowsize;
	u32 pixel;
	u8 BlueBuffer;
	u8 RedBuffer;
	u8 GreenBuffer;
	BYTE work[FF_MAX_SS];
	FRESULT status;
	//在 FatFs 模块上注册 /注销一个工作区 (文件系统对象 )
	status = f_mount(&fatfs, "", 0);
	if (status != FR_OK) {
		xil_printf("volume is not FAT format\n");
		//格式化SD卡
		f_mkfs("", FM_FAT32, 0, work, sizeof work);
		f_mount(&fatfs, "", 0);
	}

	//打开文件
	f_open(&filsrc, "lena.bmp", FA_READ);

	//移动文件读写指针到文件开头
	f_lseek(&filsrc, 54);
	width = *(pic->bmp_width);
	height = *(pic->bmp_height);
	rowsize = pic->rowsize;

	//读出图片，写入 DDR

	for (i = 1; i < height + 1; i++) {
		for (j = 1; j < width + 1; j++) {
			f_read(&filsrc, &BlueBuffer, 1, &br);
			f_read(&filsrc, &GreenBuffer, 1, &br);
			f_read(&filsrc, &RedBuffer, 1, &br);
			pixel = 0x00000000 | (RedBuffer << 16) | (BlueBuffer << 8)
					| (GreenBuffer);
			*(data_buffer + i * (width + 2) + j) = pixel;
		}
		f_lseek(&filsrc, 54 + rowsize * (i - 1));
	}
	f_close(&filsrc);
}

void draw_frame(u32 *pixelMEM, u32 *data_buffer, u32 *sobel_buffer,
		u32 frame_width, u32 frame_height, bmp_meta *pic) {
	/*u8 RedBuffer;
	 u8 GreenBuffer;
	 u8 BlueBuffer;*/

	int i;
	int j;
	int height;
	int width;
	u32 *keep_pixelMEM;
	keep_pixelMEM = pixelMEM;
	//在 FatFs 模块上注册 /注销一个工作区 (文件系统对象 )

	//打印 BMP 图片分辨率和大小

	width = *(pic->bmp_width);
	height = *(pic->bmp_height);

	//move pointer to bottom-left of original image, 30*2 pixels is gap between original image and contours
	pixelMEM = pixelMEM + frame_width * (frame_height / 2 + (int) (height / 2))
			+ (frame_width / 2 - width) - 30;

	for (i = 1; i < (height+1); i++) {
		for (j = 1; j < (width+1); j++) {
			*pixelMEM = *(data_buffer+i*(width+2)+j);
			pixelMEM++;
		}
		pixelMEM = pixelMEM - (frame_width + width);

	}
	//put frame pointer back to start point
	pixelMEM = keep_pixelMEM;
	//move pointer to bottom-left of contours
	pixelMEM = pixelMEM + frame_width * (frame_height / 2 + (int) (height / 2))
			+ frame_width / 2 + 30;
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			*pixelMEM = *(sobel_buffer+i*width+j);
			pixelMEM++;
		}

		pixelMEM = pixelMEM - (frame_width + width);

	}
}

