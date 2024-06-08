#include "gravedigger.h"
 
void redraw_terrain(uint32_t *dst,int dirth) {
  uint32_t *dst0=dst;
  if (dirth<0) dirth=0;
  else if (dirth>SCREENH) dirth=SCREENH;
  int skyh=SCREENH-dirth;
  int y=0;
  for (;y<skyh;y++) {
    int xi=SCREENW;
    for (;xi-->0;dst++) *dst=COLOR_SKY;
  }
  for (;y<SCREENH;y++) {
    int xi=SCREENW;
    for (;xi-->0;dst++) *dst=COLOR_DIRT;
  }
  
  /*XXX chop a hole somewhere.
  int midx=40;
  int midy=90;
  int radius=30;
  int radius2=radius*radius;
  int xlo=midx-radius;
  int ylo=midy-radius;
  int w=radius<<1;
  int h=radius<<1;
  dst=dst0+ylo*SCREENW+xlo;
  int yi=h;
  for (y=ylo;yi-->0;y++,dst+=SCREENW) {
    int dy2=(y-midy); dy2*=dy2;
    int xi=w;
    int x=xlo;
    uint32_t *dstp=dst;
    for (;xi-->0;x++,dstp++) {
      int r2=(x-midx); r2*=r2; r2+=dy2;
      if (r2<radius2) *dstp=COLOR_SKY;
    }
  }
  /**/
}

void copy_framebuffer(uint32_t *dst,const uint32_t *src) {
  int i=SCREENW*SCREENH;
  for (;i-->0;src++,dst++) *dst=*src;
}

void blit_sprite(uint32_t *dst,int dstx,int dsty,const uint32_t *src,int srcx,int srcy,int w,int h,int flop) {
  // Output may go OOB but input must not.
  if (flop) {
    if (dstx<0) { w+=dstx; dstx=0; }
    if (dsty<0) { h+=dsty; dsty=0; }
    if (dstx>SCREENW-w) { srcx+=dstx+w-SCREENW; w=SCREENW-dstx; }
    if (dsty>SCREENH-h) { srcy+=dsty+h-SCREENH; h=SCREENH-dsty; }
  } else {
    if (dstx<0) { w+=dstx; srcx-=dstx; dstx=0; }
    if (dsty<0) { h+=dsty; srcy-=dsty; dsty=0; }
    if (dstx>SCREENW-w) w=SCREENW-dstx;
    if (dsty>SCREENH-h) h=SCREENH-dsty;
  }
  if ((w<1)||(h<1)) return;
  // Pixels are completely opaque or completely transparent; only exactly zero is transparent.
  dst+=dsty*SCREENW+dstx;
  src+=srcy*SPRITESW+srcx;
  int yi=h;
  if (flop) {
    dst+=w-1;
    for (;yi-->0;dst+=SCREENW,src+=SPRITESW) {
      uint32_t *dstp=dst;
      const uint32_t *srcp=src;
      int xi=w;
      for (;xi-->0;dstp--,srcp++) {
        if (*srcp) *dstp=*srcp;
      }
    }
  } else {
    for (;yi-->0;dst+=SCREENW,src+=SPRITESW) {
      uint32_t *dstp=dst;
      const uint32_t *srcp=src;
      int xi=w;
      for (;xi-->0;dstp++,srcp++) {
        if (*srcp) *dstp=*srcp;
      }
    }
  }
}

int find_highest_dirt(const uint32_t *fb,int x,int y,int w,int h) {
  if (x<0) { w+=x; x=0; }
  if (x>SCREENW-w) w=SCREENW-x;
  if (y<0) { h+=y; y=0; }
  if (y>SCREENH-h) h=SCREENH-y;
  if ((w<1)||(h<1)) return 0;
  fb+=y*SCREENW+x;
  int yi=h,ry=0;
  for (;yi-->0;fb+=SCREENW,ry++) {
    const uint32_t *fbp=fb;
    int xi=w;
    for (;xi-->0;fbp++) {
      if (COLOR_IS_DIRT(*fbp)) return ry;
    }
  }
  return h;
}
