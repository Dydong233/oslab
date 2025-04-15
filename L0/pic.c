#include<am.h>
#include<amdev.h>
#include<klib-macros.h>
#include<klib.h>
#include"pic/football.h"

static AM_GPU_CONFIG_T cfg;
static AM_GPU_FBDRAW_T drw;
static AM_INPUT_KEYBRD_T key;

void scale_blit_nearest(uint32_t *dst, int dst_w, int dst_h, uint32_t *src, int src_w, int src_h) {
 	for (int y = 0; y < dst_h; y++) {
 		int src_y = y * src_h / dst_h;
		for (int x = 0; x < dst_w; x++) {
 			int src_x = x * src_w / dst_w;
 			dst[y * dst_w + x] = src[src_y * src_w + src_x];
    		}
  	}
}

int main()
{
	ioe_init();
	ioe_read(AM_GPU_CONFIG,&cfg);
	printf("Display: %dx%d, vmem size: %d\n", cfg.width, cfg.height, cfg.vmemsz);
	uint32_t *mem = malloc(sizeof(uint32_t)*cfg.width*cfg.height);

	// copy data from imageData to mem
	scale_blit_nearest(mem,cfg.width,cfg.height,imageData,imgWidth,imgHeight);

	// set the reg
	drw.x = 0;
	drw.y = 0;
	drw.pixels = mem;
	drw.w = cfg.width;
	drw.h = cfg.height;
	drw.sync = true;
	ioe_write(AM_GPU_FBDRAW,&drw);

	// wait to esc
	while(key.keycode != AM_KEY_ESCAPE){
		ioe_read(AM_INPUT_KEYBRD,&key);
	}
	return 0;
}
