#include "text.h"

/* Decode UTF-8.
 */
 
int text_utf8_decode(int *codepoint,const void *src,int srcc) {
  if (!src||(srcc<1)) return 0;
  const unsigned char *SRC=src;
  if (!(SRC[0]&0x80)) {
    *codepoint=SRC[0];
    return 1;
  }
  if (!(SRC[0]&0x40)) return 0;
  if (!(SRC[0]&0x20)) {
    if (srcc<2) return 0;
    if ((SRC[1]&0xc0)!=0x80) return 0;
    *codepoint=((SRC[0]&0x1f)<<6)|(SRC[1]&0x3f);
    return 2;
  }
  if (!(SRC[0]&0x10)) {
    if (srcc<3) return 0;
    if ((SRC[1]&0xc0)!=0x80) return 0;
    if ((SRC[2]&0xc0)!=0x80) return 0;
    *codepoint=((SRC[0]&0x0f)<<12)|((SRC[1]&0x3f)<<6)|(SRC[2]&0x3f);
    return 3;
  }
  if (!(SRC[0]&0x08)) {
    if (srcc<4) return 0;
    if ((SRC[1]&0xc0)!=0x80) return 0;
    if ((SRC[2]&0xc0)!=0x80) return 0;
    if ((SRC[3]&0xc0)!=0x80) return 0;
    *codepoint=((SRC[0]&0x07)<<18)|((SRC[1]&0x3f)<<12)|((SRC[2]&0x3f)<<6)|(SRC[3]&0x3f);
    return 4;
  }
  return 0;
}

/* Encode UTF-8.
 */
 
int text_utf8_encode(void *dst,int dsta,int codepoint) {
  unsigned char *DST=dst;
  if (codepoint<0) return 0;
  if (codepoint<0x80) {
    if (dsta>=1) {
      DST[0]=codepoint;
    }
    return 1;
  }
  if (codepoint<0x800) {
    if (dsta>=2) {
      DST[0]=0xc0|(codepoint>>6);
      DST[1]=0x80|(codepoint&0x3f);
    }
    return 2;
  }
  if (codepoint<0x1000) {
    if (dsta>=3) {
      DST[0]=0xe0|(codepoint>>12);
      DST[1]=0x80|((codepoint>>6)&0x3f);
      DST[2]=0x80|(codepoint&0x3f);
    }
    return 3;
  }
  if (codepoint<0x110000) {
    if (dsta>=4) {
      DST[0]=0xf0|(codepoint>>18);
      DST[1]=0x80|((codepoint>>12)&0x3f);
      DST[2]=0x80|((codepoint>>6)&0x3f);
      DST[3]=0x80|(codepoint&0x3f);
    }
    return 4;
  }
  return 0;
}
