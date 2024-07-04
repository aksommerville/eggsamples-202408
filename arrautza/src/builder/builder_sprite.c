#include "builder.h"

/* Evaluate opcode.
 */
 
static int builder_spritecmd_eval(const char *src,int srcc) {
  #define _(tag) if ((srcc==sizeof(#tag)-1)&&!memcmp(src,#tag,srcc)) return SPRITECMD_##tag;
  SPRITECMD_FOR_EACH
  #undef _
  int n;
  if ((sr_int_eval(&n,src,srcc)>=2)&&(n>=0)&&(n<=0xff)) return n;
  return -1;
}

/* Evaluate one group name.
 * Returns mask, not index.
 */
 
static int builder_grpmask_eval(const char *src,int srcc) {
  #define _(tag) if ((srcc==sizeof(#tag)-1)&&!memcmp(src,#tag,srcc)) return 1<<SPRGRP_##tag;
  SPRGRP_FOR_EACH
  #undef _
  int v;
  if (sr_int_eval(&v,src,srcc)>=1) return v;
  return 0;
}

/* Compile one argument.
 * All arguments are generic, we don't care what the opcode is. (except for logging)
 */
 
static int builder_compile_sprite_arg(const char *src,int srcc,int lineno,const char *kw,int kwc) {

  /* Quoted strings evaluate and emit verbatim.
   */
  if ((srcc>=2)&&(src[0]=='"')&&(src[srcc-1]=='"')) {
    char dst[256];
    int dstc=sr_string_eval(dst,sizeof(dst),src,srcc);
    if ((dstc<0)||(dstc>sizeof(dst))) {
      fprintf(stderr,"%s:%d: Failed to evaluate string.\n",builder.srcpath,lineno);
      return -2;
    }
    return sr_encode_raw(&builder.dst,dst,dstc);
  }
  
  /* "groups:NAME1,NAME2,..." => 4-byte group mask.
   */
  int colonp=-1;
  int i=0; for (;i<srcc;i++) if (src[i]==':') { colonp=i; break; }
  if ((colonp==6)&&!memcmp(src,"groups",6)) {
    uint32_t grpmask=0;
    int srcp=7;
    while (srcp<srcc) {
      if (src[srcp]==',') { srcp++; continue; }
      const char *token=src+srcp;
      int tokenc=0;
      while ((srcp<srcc)&&(src[srcp++]!=',')) tokenc++;
      int tmp=builder_grpmask_eval(token,tokenc);
      if (!tmp) {
        fprintf(stderr,"%s:%d: Expected sprite group name, found '%.*s'\n",builder.srcpath,lineno,tokenc,token);
        return -2;
      }
      grpmask|=tmp;
    }
    if (grpmask&((1<<SPRGRP_KEEPALIVE)|(1<<SPRGRP_DEATHROW))) {
      fprintf(stderr,"%s:%d: Sprites must not join the KEEPALIVE or DEATHROW groups via sprdef.\n",builder.srcpath,lineno);
      return -2;
    }
    return sr_encode_intbe(&builder.dst,grpmask,4);
  }
  
  /* "TYPE:NAME" => 2-byte rid
   * Nothing below this can allow a colon.
   */
  if (colonp>=0) {
    int tid=builder_restype_eval(src,colonp);
    if (tid<1) {
      fprintf(stderr,"%s:%d: Expected resource type, found '%.*s'\n",builder.srcpath,lineno,colonp,src);
      return -2;
    }
    int rid=builder_rid_eval(tid,src+colonp+1,srcc-colonp-1);
    if (rid<1) {
      fprintf(stderr,"%s:%d: Resource '%.*s' not found\n",builder.srcpath,lineno,srcc,src);
      return -2;
    }
    return sr_encode_intbe(&builder.dst,rid,2);
  }
  
  /* Anything else must be a literal integer.
   * Length is 1 if not specified by a suffix.
   */
  int len=1,lo,hi,v;
       if ((srcc>2)&&!memcmp(src+srcc-2, "u8",2)) { srcc-=2; len=1; lo=-0x80; hi=0xff; }
  else if ((srcc>3)&&!memcmp(src+srcc-3,"u16",3)) { srcc-=3; len=2; lo=-0x8000; hi=0xffff; }
  else if ((srcc>3)&&!memcmp(src+srcc-3,"u24",3)) { srcc-=3; len=3; lo=-0x800000; hi=0xffffff; }
  else if ((srcc>3)&&!memcmp(src+srcc-3,"u32",3)) { srcc-=3; len=4; lo=INT_MIN; hi=INT_MAX; }
  if ((sr_int_eval(&v,src,srcc)<1)||(v<lo)||(v>hi)) {
    fprintf(stderr,"%s:%d: Failed to evaluate '%.*s' as %d-byte integer.\n",builder.srcpath,lineno,srcc,src,len);
    return -2;
  }
  return sr_encode_intbe(&builder.dst,v,len);
}

/* Compile one command.
 */
 
static int builder_compile_sprite_command(const char *src,int srcc,int lineno) {
  int srcp=0;
  
  /* First token is the opcode.
   */
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *kw=src+srcp;
  int kwc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; kwc++; }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  int opcode=builder_spritecmd_eval(kw,kwc);
  if (opcode<0) {
    fprintf(stderr,"%s:%d: Unknown sprite command '%.*s'\n",builder.srcpath,lineno,kwc,kw);
    return -2;
  }
  if (sr_encode_u8(&builder.dst,opcode)<0) return -1;
  int payloadp=builder.dst.c;
  
  /* (0xe0..0xef) take a one-byte payload length; emit a placeholder.
   * (0xf0..0xff) are reserved and currently illegal.
   */
  if (opcode>=0xf0) {
    fprintf(stderr,"%s:%d: Illegal sprite opcode 0x%02x\n",builder.srcpath,lineno,opcode);
    return -2;
  } else if (opcode>=0xe0) {
    if (sr_encode_u8(&builder.dst,0)<0) return -1;
  }
  
  /* Remaining tokens compile generically.
   */
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    const char *token=src+srcp;
    int tokenc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; tokenc++; }
    int err=builder_compile_sprite_arg(token,tokenc,lineno,kw,kwc);
    if (err<0) {
      if (err!=-2) fprintf(stderr,"%s:%d: Failed to compile argument '%.*s' to command '%.*s'\n",builder.srcpath,lineno,tokenc,token,kwc,kw);
      return -2;
    }
  }
  
  /* Confirm that we emitted the correct length.
   */
  if (opcode>=0xe0) { // Explicit length. (payloadp) points to the length byte.
    int paylen=builder.dst.c-payloadp-1;
    if ((paylen<0)||(paylen>0xff)) {
      fprintf(stderr,"%s:%d: Invalid payload length %d for command '%.*s'\n",builder.srcpath,lineno,paylen,kwc,kw);
      return -2;
    }
    ((uint8_t*)builder.dst.v)[payloadp]=paylen;
  } else { // Everything else has an fixed length per opcode, knowable from the top 3 bits.
    int paylen=builder.dst.c-payloadp;
    #define REQLEN(c) if (paylen!=c) { \
      fprintf(stderr,"%s:%d: Payload for sprite command '%.*s' must be %d bytes, found %d\n",builder.srcpath,lineno,kwc,kw,c,paylen); \
      return -2; \
    }
    switch (opcode&0xe0) {
      case 0x00: REQLEN(0) break;
      case 0x20: REQLEN(2) break;
      case 0x40: REQLEN(4) break;
      case 0x60: REQLEN(6) break;
      case 0x80: REQLEN(8) break;
      case 0xa0: REQLEN(12) break;
      case 0xc0: REQLEN(16) break;
      default: return -1;
    }
  }
  
  return 0;
}

/* Compile sprite, main entry point.
 */
 
int builder_compile_sprite() {
  struct sr_decoder decoder={.v=builder.src,.c=builder.srcc};
  int lineno=1,linec;
  const char *line;
  for (;(linec=sr_decode_line(&line,&decoder))>0;lineno++) {
    int i=0; for (;i<linec;i++) if (line[i]=='#') linec=i;
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    if (!linec) continue;
    int err=builder_compile_sprite_command(line,linec,lineno);
    if (err<0) {
      if (err!=-2) fprintf(stderr,"%s:%d: Unspecified error compiling sprite.\n",builder.srcpath,lineno);
      return -2;
    }
  }
  return 0;
}
