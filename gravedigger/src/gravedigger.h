#ifndef GRAVEDIGGER_H
#define GRAVEDIGGER_H

#include <stdint.h>

// Must agree with metdata.
#define SCREENW 240
#define SCREENH 134

// Dimensions of the full RGBA spritesheet. (image:0:2)
#define SPRITESW 128
#define SPRITESH 128

// Colors are 0xAABBGGRR. (Wasm is always little-endian).
#define COLOR_SKY 0xffe0b060
#define COLOR_DIRT 0xff102040
#define COLOR_IS_DIRT(px) (!((px)&0x00c00000))

// gd_render.c
void redraw_terrain(uint32_t *dst,int dirth);
void copy_framebuffer(uint32_t *dst,const uint32_t *src);
void blit_sprite(uint32_t *dst,int dstx,int dsty,const uint32_t *src,int srcx,int srcy,int w,int h,int flop);
int find_highest_dirt(const uint32_t *fb,int x,int y,int w,int h); // => 0..h, position of highest dirt pixel in this box

#endif
