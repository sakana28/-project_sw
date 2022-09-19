typedef struct bmp_meta {
	u8 bmp_head[54];
	int* bmp_width;
	int* bmp_height;
	int rowsize;
} bmp_meta;

void readBMPhead(bmp_meta *pic);
void load_sd_bmp(bmp_meta *pic, u32 *data_buffer);

void draw_frame(u32 *pixelMEM, u32 *data_buffer, u32 *sobel_buffer,
		u32 frame_width, u32 frame_height, bmp_meta *pic);
