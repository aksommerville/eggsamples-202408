#include <egg/egg.h>
#include "font.h"
#include "text.h"
#if USE_REAL_STDLIB
  #include <string.h>
  #include <stdlib.h>
  #include <limits.h>
#else
  #include "stdlib/egg-stdlib.h"
#endif

/* Object definition.
 */
 
struct font_glyph {
  int16_t x,y,w; // (h) is constant across the font.
};
 
struct font_page {
  int codepoint;
  int count;
  int w,h,stride;
  uint8_t *v; // The image. 1-bit big-endian, rows are byte-aligned.
  struct font_glyph *glyphv;
};
 
struct font {
  int rowh;
  int spacew;
  struct font_page *pagev;
  int pagec,pagea;
};

/* Delete.
 */
 
static void font_page_cleanup(struct font_page *page) {
  if (page->v) free(page->v);
  if (page->glyphv) free(page->glyphv);
}
 
void font_del(struct font *font) {
  if (!font) return;
  if (font->pagev) {
    while (font->pagec-->0) font_page_cleanup(font->pagev+font->pagec);
    free(font->pagev);
  }
  free(font);
}

/* New.
 */

struct font *font_new(int rowh) {
  if (rowh<1) return 0;
  struct font *font=calloc(1,sizeof(struct font));
  if (!font) return 0;
  font->rowh=rowh;
  font->spacew=rowh>>1;
  return font;
}

/* Page list.
 */
 
static int font_pagev_search(const struct font *font,int codepoint) {
  int lo=0,hi=font->pagec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct font_page *page=font->pagev+ck;
         if (codepoint<page->codepoint) hi=ck;
    else if (codepoint>=page->codepoint+page->count) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static struct font_page *font_pagev_insert(struct font *font,int p,int codepoint) {
  if ((p<0)||(p>font->pagec)) return 0;
  if (font->pagec>=font->pagea) {
    int na=font->pagea+8;
    if (na>INT_MAX/sizeof(struct font_page)) return 0;
    void *nv=realloc(font->pagev,sizeof(struct font_page)*na);
    if (!nv) return 0;
    font->pagev=nv;
    font->pagea=na;
  }
  struct font_page *page=font->pagev+p;
  memmove(page+1,page,sizeof(struct font_page)*(font->pagec-p));
  font->pagec++;
  memset(page,0,sizeof(struct font_page));
  page->codepoint=codepoint;
  return page;
}

/* True if a single-pixel-wide box in the 1-bit image is all zeroes.
 */
 
static int font_column_is_empty(const uint8_t *src,int x,int h,int stride) {
  src+=x>>3;
  uint8_t mask=0x80>>(x&7);
  for (;h-->0;src+=stride) if ((*src)&mask) return 0;
  return 1;
}

/* Add glyph to page.
 * Accepts and returns the allocated length (we don't record that in (page)).
 */
 
static int font_page_add_glyph(struct font *font,struct font_page *page,int x,int y,int w,int a) {
  if (page->count>=a) {
    a+=32;
    if (a>INT_MAX/sizeof(struct font_glyph)) return -1;
    void *nv=realloc(page->glyphv,sizeof(struct font_glyph)*a);
    if (!nv) return -1;
    page->glyphv=nv;
  }
  struct font_glyph *glyph=page->glyphv+page->count++;
  glyph->x=x;
  glyph->y=y;
  glyph->w=w;
  return a;
}

/* With image populated, analyze it and generate (count,glyphv).
 * At the end, we'll verify that we don't overlap the next page.
 */
 
static int font_page_split(struct font *font,struct font_page *page) {
  int rowc=page->h/font->rowh;
  int ribbonstride=page->stride*font->rowh;
  const uint8_t *srcrow=page->v;
  int y=0,glypha=0;
  for (;rowc-->0;y+=font->rowh,srcrow+=ribbonstride) {
    int x=0; for (;x<page->w;x++) {
      if (font_column_is_empty(srcrow,x,font->rowh,page->stride)) continue;
      int glyphw=1;
      while ((x+glyphw<page->w)&&!font_column_is_empty(srcrow,x+glyphw,font->rowh,page->stride)) glyphw++;
      if ((glypha=font_page_add_glyph(font,page,x,y,glyphw,glypha))<0) return -1;
      x+=glyphw; // loop control +1 too; that skips the sentinel column
    }
  }
  int p=page-font->pagev;
  if ((p>=0)&&(p<font->pagec-1)) {
    const struct font_page *next=page+1;
    if (page->codepoint+page->count>next->codepoint) return -1;
  }
  return 0;
}

/* Add page from decoded pixels.
 * Hands off pixel buffer on success.
 */
 
static int font_handoff_decoded_page(struct font *font,int codepoint,void *v,int w,int h,int stride) {
  int p=font_pagev_search(font,codepoint);
  if (p>=0) return -1;
  p=-p-1;
  struct font_page *page=font_pagev_insert(font,p,codepoint);
  if (!page) return -1;
  page->v=v;
  page->w=w;
  page->h=h;
  page->stride=stride;
  if (font_page_split(font,page)<0) {
    page->v=0;
    font_page_cleanup(page);
    font->pagec--;
    memmove(page,page+1,sizeof(struct font_page)*(font->pagec-p));
    return -1;
  }
  return 0;
}

/* Add page.
 */

int font_add_page(struct font *font,int imageid,int codepoint) {
  if (!font) return -1;
  int w=0,h=0,stride=0,fmt=0;
  egg_image_get_header(&w,&h,&stride,&fmt,0,imageid);
  if ((w<1)||(h<1)||(stride<1)) return -1;
  if (h%font->rowh) return -1;
  switch (fmt) {
    case EGG_TEX_FMT_A1: break;
    default: return -1;
  }
  int pixelslen=stride*h;
  void *pixels=calloc(stride,h);
  if (!pixels) return -1;
  int err=egg_image_decode(pixels,pixelslen,0,imageid);
  if (err!=pixelslen) {
    free(pixels);
    return -1;
  }
  if (font_handoff_decoded_page(font,codepoint,pixels,w,h,stride)<0) {
    free(pixels);
    return -1;
  }
  return 0;
}

/* Trivial accessors.
 */
 
int font_get_rowh(const struct font *font) {
  if (!font) return 0;
  return font->rowh;
}

int font_count_glyphs(const struct font *font) {
  if (!font) return 0;
  int c=0,i=font->pagec;
  const struct font_page *page=font->pagev;
  for (;i-->0;page++) c+=page->count;
  return c;
}

int font_count_pages(const struct font *font) {
  if (!font) return 0;
  return font->pagec;
}

/* Render to texture, high-level conveniences.
 */

int font_render_to_texture(int texid,const struct font *font,const char *src,int srcc,int wlimit,int rgb) {

  // Start by breaking lines and measuring.
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  #define LINELIMIT 32
  int startv[LINELIMIT];
  int startc;
  if (wlimit>0) {
    if (wlimit>4096) wlimit=4096; // Otherwise we might try to allocate really enormous images.
    startc=font_break_lines(startv,LINELIMIT,font,src,srcc,wlimit);
    if (startc<0) return -1;
    if (startc>=LINELIMIT) startc=LINELIMIT-1; // Ensure we always have a "next line"; drop the last one.
    else if (!startc) {
      startv[0]=0;
      startv[1]=srcc;
      startc=1;
    } else startv[startc]=srcc;
  } else {
    startv[0]=0;
    startv[1]=srcc;
    startc=1;
    if ((wlimit=font_measure(font,src,srcc))<1) wlimit=1;
  }
  #undef LINELIMIT
  
  // Generate an image to render into.
  int w=wlimit;
  int h=startc*font->rowh;
  int stride=w<<2;
  void *pixels=calloc(stride,h);
  if (!pixels) return -1;
  
  // Render line by line.
  int y=0,p=0;
  for (;p<startc;p++,y+=font->rowh) {
    const char *line=src+startv[p];
    int linec=startv[p+1]-startv[p];
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    font_render_string_rgba(pixels,w,h,stride,0,y,font,line,linec,rgb);
  }
  
  // Upload and clean up.
  int err=egg_texture_upload(texid,w,h,stride,EGG_TEX_FMT_RGBA,pixels,stride*h);
  free(pixels);
  return err;
}

int font_render_new_texture(const struct font *font,const char *src,int srcc,int wlimit,int rgb) {
  int texid=egg_texture_new();
  if (texid<1) return -1;
  if (font_render_to_texture(texid,font,src,srcc,wlimit,rgb)<0) {
    egg_texture_del(texid);
    return -1;
  }
  return texid;
}

/* Measure width of single line of text.
 */
 
int font_measure_glyph(const struct font *font,int codepoint) {
  if ((codepoint==0x20)||(codepoint==0xa0)) return font->spacew;
  if (codepoint==0x09) return font->rowh;
  int pagep=font_pagev_search(font,codepoint);
  if (pagep<0) return 0;
  const struct font_page *page=font->pagev+pagep;
  const struct font_glyph *glyph=page->glyphv+(codepoint-page->codepoint);
  return glyph->w;
}

int font_measure(const struct font *font,const char *src,int srcc) {
  if (!font||!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int srcp=0,w=0;
  while (srcp<srcc) {
    int ch=0,seqlen;
    if ((seqlen=text_utf8_decode(&ch,src+srcp,srcc-srcp))<1) {
      ch=(uint8_t)(src[srcp]);
      seqlen=1;
    }
    srcp+=seqlen;
    w+=font_measure_glyph(font,ch);
    w++; // We add one empty column between glyphs.
  }
  if (w) w--; // Remove the last spacer.
  return w;
}

/* Break lines intelligently.
 */
 
// Longest stretch of (src) that fits within (wlimit), including trailing space.
static int font_break_line_1(const struct font *font,const char *src,int srcc,int wlimit) {

  // First check for newlines. If one is present, we stop hard immediately after it.
  // It's easier to chomp the input there, than to check for it during the measure loop.
  int i=0; for (;i<srcc;i++) if (src[i]==0x0a) srcc=i+1;
  
  int w=0,srcp=0;
  while (srcp<srcc) {
    int ch=0,seqlen;
    if ((seqlen=text_utf8_decode(&ch,src+srcp,srcc-srcp))<1) {
      ch=(unsigned char)(src[srcp]);
      seqlen=1;
    }
  
    // Add whitespace even if it overruns.
    if (ch<=0x20) {
      w+=font_measure_glyph(font,ch);
      w++;
      srcp+=seqlen;
      continue;
    }
    
    // If we're already breached, stop. Don't stop if we're at the first position, but I don't think that's possible.
    if ((w>=wlimit)&&srcp) return srcp;
    
    // Measure to the end of this word and stop if we breach.
    int wordw=font_measure_glyph(font,ch)+1;
    int srcpnext=srcp+seqlen;
    while (srcpnext<srcc) {
      if ((seqlen=text_utf8_decode(&ch,src+srcpnext,srcc-srcpnext))<1) {
        ch=(unsigned char)(src[srcpnext]);
        seqlen=1;
      }
      if (ch<=0x20) break;
      int gw=font_measure_glyph(font,ch);
      if (w+wordw+gw>wlimit) { // Too long.
        if (srcp) return srcp;
        return seqlen;
      }
      wordw+=gw+1;
      srcpnext+=seqlen;
    }
    w+=wordw;
    srcp=srcpnext;
  }
  return srcp;
}

int font_break_lines(int *startv,int starta,const struct font *font,const char *src,int srcc,int wlimit) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int startc=0,srcp=0;
  while (srcp<srcc) {
    if (startc<starta) startv[startc]=srcp; startc++;
    int chc=font_break_line_1(font,src+srcp,srcc-srcp,wlimit);
    if (chc<1) return -1;
    srcp+=chc;
  }
  return startc;
}

/* Basic blit with output bounds checking.
 * Input is always a1 with valid bounds.
 */
 
static void font_blit_rgba(
  uint8_t *dst,int dstw,int dsth,int dststride,int dstx,int dsty,
  const uint8_t *src,int srcw,int srch,int srcstride,int srcx,int srcy,
  int w,int h,int rgb
) {
  if (dstx<0) { w+=dstx; srcx-=dstx; dstx=0; }
  if (dsty<0) { h+=dsty; srcy-=dsty; dsty=0; }
  if (dstx>dstw-w) w=dstw-dstx;
  if (dsty>dsth-h) h=dsth-dsty;
  if ((w<1)||(h<1)) return;
  uint8_t r=rgb>>16,g=rgb>>8,b=rgb;
  dst+=dsty*dststride+(dstx<<2);
  src+=srcy*srcstride+(srcx>>3);
  uint8_t mask0=0x80>>(srcx&7);
  int yi=h; for (;yi-->0;dst+=dststride,src+=srcstride) {
    uint8_t *dstp=dst;
    const uint8_t *srcp=src;
    uint8_t mask=mask0;
    int xi=w; for (;xi-->0;dstp+=4) {
      if ((*srcp)&mask) {
        dstp[0]=r;
        dstp[1]=g;
        dstp[2]=b;
        dstp[3]=0xff;
      }
      if (mask==1) { mask=0x80; srcp++; }
      else mask>>=1;
    }
  }
}
 
static void font_blit_a1(
  uint8_t *dst,int dstw,int dsth,int dststride,int dstx,int dsty,
  const uint8_t *src,int srcw,int srch,int srcstride,int srcx,int srcy,
  int w,int h
) {
  if (dstx<0) { w+=dstx; srcx-=dstx; dstx=0; }
  if (dsty<0) { h+=dsty; srcy-=dsty; dsty=0; }
  if (dstx>dstw-w) w=dstw-dstx;
  if (dsty>dsth-h) h=dsth-dsty;
  if ((w<1)||(h<1)) return;
  dst+=dsty*dststride+(dstx>>3);
  src+=srcy*srcstride+(srcx>>3);
  uint8_t dstmask0=0x80>>(dstx&7);
  uint8_t srcmask0=0x80>>(srcx&7);
  int yi=h; for (;yi-->0;dst+=dststride,src+=srcstride) {
    uint8_t *dstp=dst;
    const uint8_t *srcp=src;
    uint8_t dstmask=dstmask0;
    uint8_t srcmask=srcmask0;
    int xi=w; for (;xi-->0;) {
      if ((*srcp)&srcmask) (*dstp)|=dstmask;
      if (dstmask==1) { dstmask=0x80; dstp++; }
      else dstmask>>=1;
      if (srcmask==1) { srcmask=0x80; srcp++; }
      else srcmask>>=1;
    }
  }
}

/* Render one line to RGBA.
 */

int font_render_string_rgba(
  void *dst,int dstw,int dsth,int dststride,
  int dstx,int dsty,
  const struct font *font,
  const char *src,int srcc,
  int rgb
) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int dstx0=dstx;
  int srcp=0;
  while (srcp<srcc) {
    int ch=0,seqlen;
    if ((seqlen=text_utf8_decode(&ch,src+srcp,srcc-srcp))<1) {
      ch=(uint8_t)(src[srcp]);
      seqlen=1;
    }
    srcp+=seqlen;
    if ((ch==0x20)||(ch==0xa0)) {
      dstx+=font->spacew;
    } else if (ch==0x09) {
      dstx+=font->rowh;
    } else {
      int pagep=font_pagev_search(font,ch);
      if (pagep>=0) {
        const struct font_page *page=font->pagev+pagep;
        const struct font_glyph *glyph=page->glyphv+(ch-page->codepoint);
        font_blit_rgba(
          dst,dstw,dsth,dststride,dstx,dsty,
          page->v,page->w,page->h,page->stride,glyph->x,glyph->y,
          glyph->w,font->rowh,rgb
        );
        dstx+=glyph->w;
      }
    }
    dstx++;
  }
  return dstx-dstx0;
}

/* Render one line to A1.
 */

int font_render_string_a1(
  void *dst,int dstw,int dsth,int dststride,
  int dstx,int dsty,
  const struct font *font,
  const char *src,int srcc
) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int dstx0=dstx;
  int srcp=0;
  while (srcp<srcc) {
    int ch=0,seqlen;
    if ((seqlen=text_utf8_decode(&ch,src+srcp,srcc-srcp))<1) {
      ch=(uint8_t)(src[srcp]);
      seqlen=1;
    }
    srcp+=seqlen;
    if ((ch==0x20)||(ch==0xa0)) {
      dstx+=font->spacew;
    } else if (ch==0x09) {
      dstx+=font->rowh;
    } else {
      int pagep=font_pagev_search(font,ch);
      if (pagep>=0) {
        const struct font_page *page=font->pagev+pagep;
        const struct font_glyph *glyph=page->glyphv+(ch-page->codepoint);
        font_blit_a1(
          dst,dstw,dsth,dststride,dstx,dsty,
          page->v,page->w,page->h,page->stride,glyph->x,glyph->y,
          glyph->w,font->rowh
        );
        dstx+=glyph->w;
      }
    }
    dstx++;
  }
  return dstx-dstx0;
}

/* Render one glyph to RGBA.
 */
 
int font_render_char_rgba(
  void *dst,int dstw,int dsth,int dststride,
  int dstx,int dsty,
  const struct font *font,
  int codepoint,
  int rgb
) {
  if ((codepoint==0x20)||(codepoint==0xa0)||(codepoint==0x09)) return 0;
  int pagep=font_pagev_search(font,codepoint);
  if (pagep<0) return 0;
  const struct font_page *page=font->pagev+pagep;
  const struct font_glyph *glyph=page->glyphv+(codepoint-page->codepoint);
  font_blit_rgba(
    dst,dstw,dsth,dststride,dstx-(glyph->w>>1),dsty-(font->rowh>>1),
    page->v,page->w,page->h,page->stride,glyph->x,glyph->y,
    glyph->w,font->rowh,rgb
  );
  return 0;
}

/* Render one glyph to A1.
 */
 
int font_render_char_a1(
  void *dst,int dstw,int dsth,int dststride,
  int dstx,int dsty,
  const struct font *font,
  int codepoint
) {
  if ((codepoint==0x20)||(codepoint==0xa0)||(codepoint==0x09)) return 0;
  int pagep=font_pagev_search(font,codepoint);
  if (pagep<0) return 0;
  const struct font_page *page=font->pagev+pagep;
  const struct font_glyph *glyph=page->glyphv+(codepoint-page->codepoint);
  font_blit_a1(
    dst,dstw,dsth,dststride,dstx-(glyph->w>>1),dsty-(font->rowh>>1),
    page->v,page->w,page->h,page->stride,glyph->x,glyph->y,
    glyph->w,font->rowh
  );
  return 0;
}
