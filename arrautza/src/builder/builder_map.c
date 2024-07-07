#include "builder.h"

/* Evaluate command.
 */
 
static int mapcmd_eval(const char *src,int srcc) {
  #define _(tag) if ((srcc==sizeof(#tag)-1)&&!memcmp(src,#tag,srcc)) return MAPCMD_##tag;
  MAPCMD_FOR_EACH
  #undef _
  int n;
  if ((sr_int_eval(&n,src,srcc)>=2)&&(n>=0)&&(n<=0xff)) return n;
  return -1;
}

/* Compile one generic argument to any command.
 * Caller cuts the input (on whitespace); we return output length.
 */
 
static int compile_argument(uint8_t *dst,int dsta,uint8_t opcode,const char *src,int srcc) {
  if (srcc<1) return 0;
  int dstc=0,err;
  
  // Check for natural integers.
  // (lo,hi) should be determinable from (bytec), but it gets awkward for size 4. Just provide it for us.
  #define TRYINT(sfx,bytec,lo,hi) { \
    if ((srcc>sizeof(sfx)-1)&&!memcmp(src+srcc-sizeof(sfx)+1,sfx,sizeof(sfx)-1)) { \
      int v; \
      if (sr_int_eval(&v,src,srcc-sizeof(sfx)+1)>=1) { \
        if ((v<lo)||(v>hi)) { \
          fprintf(stderr,"Invalid integer '%.*s', must be in %d..%d\n",srcc,src,lo,hi); \
          return -2; \
        } \
        if (dsta>=bytec) { \
          int i=bytec; \
          while (i-->0) { \
            dst[i]=v; \
            v>>=8; \
          } \
        } \
        return bytec; \
      } \
    } \
  }
  TRYINT("",1,-0x80,0xff)
  TRYINT("u16",2,-0x8000,0xffff)
  TRYINT("u24",3,-0x800000,0xffffff)
  TRYINT("u32",4,0x80000000,0x7fffffff)
  #undef TRYINT
  
  // "@A,B[,C,D]" => 2 or 4 bytes. Equivalent to "A B [C D]" for us, but the editor treats these "@" ones special.
  if (src[0]=='@') {
    int srcp=1;
    while (srcp<srcc) {
      const char *inner=src+srcp;
      int innerc=0;
      while ((srcp<srcc)&&(src[srcp++]!=',')) innerc++;
      int v;
      if ((sr_int_eval(&v,inner,innerc)<2)||(v<0)||(v>0xff)) {
        fprintf(stderr,"Expected integer in 0..255, found '%.*s'\n",innerc,inner);
        return -2;
      }
      if (dstc<dsta) dst[dstc]=v;
      dstc++;
    }
    if ((dstc!=2)&&(dstc!=4)) {
      fprintf(stderr,"'@' must be followed by 2 or 4 integers, found %d ('%.*s')\n",dstc,srcc,src);
      return -2;
    }
    return dstc;
  }
  
  // Quoted strings.
  if ((srcc>=2)&&(src[0]=='"')&&(src[srcc-1]=='"')) {
    if ((dstc=sr_string_eval((char*)dst,dsta,src,srcc))<0) {
      fprintf(stderr,"Failed to evaluate string literal.\n");
      return -2;
    }
    return dstc;
  }
  
  // "FLD_*" for stobus fields.
  if ((srcc>=4)&&!memcmp(src,"FLD_",4)) {
    int id=builder_field_eval(0,src,srcc);
    if (id<1) {
      fprintf(stderr,"Unknown field '%.*s'\n",srcc,src);
      return -2;
    }
    if (id>0xff) {
      fprintf(stderr,"Only fields 1..255 may be referenced here (found %d)\n",id);
      return -2;
    }
    if (dsta>=1) dst[0]=id;
    return 1;
  }
  
  // "TYPE:NAME" for a resource ID, or "item:NAME" for item ID.
  // Hopefully nobody creates a resource type called "item".
  int colonp=-1;
  int i=0; for (;i<srcc;i++) {
    if (src[i]==':') {
      colonp=i;
      break;
    }
  }
  if (colonp>=0) {
    const char *tname=src;
    int tnamec=colonp;
    const char *rname=src+colonp+1;
    int rnamec=srcc-colonp-1;
    int tid;
    if ((tid=builder_restype_eval(tname,tnamec))>0) {
      int rid=builder_rid_eval(tid,rname,rnamec);
      if (rid<1) {
        fprintf(stderr,"Expected resource ID for type '%.*s', found '%.*s'\n",tnamec,tname,rnamec,rname);
        return -2;
      }
      if (dsta>=2) {
        dst[0]=rid>>8;
        dst[1]=rid;
      }
      return 2;
    }
    if ((tnamec==4)&&!memcmp(tname,"item",4)) {
      int itemid=builder_item_eval(rname,rnamec);
      if (itemid<1) {
        fprintf(stderr,"Expected item name, found '%.*s'\n",rnamec,rname);
        return -2;
      }
      if (dsta>=1) dst[0]=itemid;
      return 1;
    }
    fprintf(stderr,"Expected resource type or 'item', found '%.*s'\n",tnamec,tname);
    return -2;
  }
  
  // Plenty of other constructions are possible but we don't define any more.
  return -1;
}

/* Compile command.
 * Return encoded length.
 * Fail if too long.
 */
 
static int compile_command(uint8_t *dst,int dsta,const char *src,int srcc) {
  int srcp=0,dstc=0,err;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *kw=src+srcp,*token;
  int kwc=0,tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; kwc++; }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  #define RDTOKEN { \
    token=src+srcp; \
    tokenc=0; \
    while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++; \
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++; \
  }
  #define APPEND8(v) { \
    if (dstc>=dsta) return -1; \
    dst[dstc++]=(v); \
  }
  
  int opcode=mapcmd_eval(kw,kwc);
  if (opcode<0) {
    fprintf(stderr,"Unknown map command '%.*s'\n",kwc,kw);
    return -2;
  }
  APPEND8(opcode)
  
  // Opcodes in 0xe0..0xef, append a placeholder length byte.
  if ((opcode>=0xe0)&&(opcode<=0xef)) {
    APPEND8(0)
  }
  
  switch (opcode) {
  
    //TODO Can add handlers for specific opcodes, if they have their own formatting.
  
    default: { // Arguments must be generic. I'm hoping this will usually be doable.
        while (srcp<srcc) {
          RDTOKEN
          if ((err=compile_argument(dst+dstc,dsta-dstc,opcode,token,tokenc))<0) {
            if (err!=-2) fprintf(stderr,"Error processing argument '%.*s' to opcode 0x%02x '%.*s'\n",tokenc,token,opcode,kwc,kw);
            return -2;
          }
          dstc+=err;
        }
      }
  }
  
  // Generically validate output length and full consumption of input.
  #define PAYLEN(expect) { \
    int paylen=dstc-1; \
    if (paylen!=expect) { \
      fprintf(stderr,"Expected %d-byte payload for opcode 0x%02x ('%.*s'), but compiler produced %d.\n",expect,opcode,kwc,kw,paylen); \
      return -2; \
    } \
  }
       if (opcode<0x20) PAYLEN(0)
  else if (opcode<0x40) PAYLEN(2)
  else if (opcode<0x60) PAYLEN(4)
  else if (opcode<0x80) PAYLEN(6)
  else if (opcode<0xa0) PAYLEN(8)
  else if (opcode<0xc0) PAYLEN(12)
  else if (opcode<0xe0) PAYLEN(16)
  else if (opcode<0xf0) {
    int paylen=dstc-2;
    if ((paylen<0)||(paylen>0xff)) return -1;
    dst[1]=paylen;
  } else {
    // 0xf0..0xff are "known length". Accept whatever the opcode-specific compiler produced.
  }
  #undef PAYLEN
  if (srcp<srcc) {
    fprintf(stderr,"Unexpected tokens at end of line: ...'%.*s'\n",srcc-srcp,src+srcp);
    return -2;
  }
  #undef RDTOKEN
  #undef APPEND8
  return dstc;
}

/* Compile one row of map data.
 * Caller validates length: COLC at (dst), and COLC*2 at (src).
 */
 
static int compile_cells(uint8_t *dst,const char *src) {
  int i=COLC;
  for (;i-->0;dst++,src+=2) {
    int hi=sr_digit_eval(src[0]);
    int lo=sr_digit_eval(src[1]);
    if ((hi<0)||(hi>0xf)||(lo<0)||(lo>0xf)) {
      fprintf(stderr,"Invalid hexadecimal byte '%c%c'\n",src[0],src[1]);
      return -2;
    }
    *dst=(hi<<4)|lo;
  }
  return 0;
}

/* Compile map, main entry point.
 */
 
int builder_compile_map() {
  struct map map={0};
  uint8_t *vp=map.v;
  struct sr_decoder decoder={.v=builder.src,.c=builder.srcc};
  int lineno=1,linec,i=ROWC,err;
  const char *line;
  for (;i-->0;lineno++,vp+=COLC) {
    linec=sr_decode_line(&line,&decoder);
    while ((linec>0)&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while ((linec>0)&&((unsigned char)line[0]<=0x20)) { linec--; line++; } // shouldn't be leading space, but we'll tolerate.
    if (linec<1) {
      fprintf(stderr,"%s:%d: Unexpected EOF or empty line, expecting cell data.\n",builder.srcpath,lineno);
      return -2;
    }
    if (linec!=COLC*2) {
      fprintf(stderr,"%s:%d: Cell data lines must be exactly %d hex digits (found %d)\n",builder.srcpath,lineno,COLC*2,linec);
      return -2;
    }
    if ((err=compile_cells(vp,line))<0) {
      // Log even if -2, to provide context.
      fprintf(stderr,"%s:%d: Failed to decode cells.\n",builder.srcpath,lineno);
      return -2;
    }
  }
  int cmdc=0;
  for (;(linec=sr_decode_line(&line,&decoder))>0;lineno++) {
    for (i=0;i<linec;i++) if (line[i]=='#') linec=i;
    while ((linec>0)&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while ((linec>0)&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    if (!linec) continue;
    if ((err=compile_command(map.commands+cmdc,sizeof(map.commands)-cmdc,line,linec))<0) {
      // Log even if -2, to provide context.
      fprintf(stderr,"%s:%d: Failed to decode command.\n",builder.srcpath,lineno);
      return -2;
    }
    cmdc+=err;
  }
  // At runtime, we read maps directly off the resource.
  // They are not dependent on byte order.
  // So... easy peasy!
  int len=sizeof(map.v)+cmdc;
  if (sr_encode_raw(&builder.dst,&map,len)<0) return -1;
  return 0;
}
