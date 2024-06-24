#include "builder.h"

static inline int hexdigit_eval(char src) {
  if ((src>='0')&&(src<='9')) return src-'0';
  if ((src>='a')&&(src<='f')) return src-'a'+10;
  if ((src>='A')&&(src<='F')) return src-'A'+10;
  return -1;
}

/* Compile tilesheet, main entry point.
 */
 
int builder_compile_tilesheet() {
  struct sr_decoder decoder={.v=builder.src,.c=builder.srcc};
  const char *line;
  int linec,lineno=1;
  char name[32];
  int namec=0;
  uint8_t bin[256];
  int binc=0;
  int have_physics=0;
  for (;(linec=sr_decode_line(&line,&decoder))>0;lineno++) {
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    
    if (namec) { // A table is started. We must receive 16 lines of 16 bytes each.
      if (linec!=32) {
        fprintf(stderr,"%s:%d: Expected 32 bytes, found %d.\n",builder.srcpath,lineno,linec);
        return -2;
      }
      int linep=0;
      while (linep<linec) {
        int hi=hexdigit_eval(line[linep++]);
        int lo=hexdigit_eval(line[linep++]);
        if ((hi<0)||(lo<0)) {
          fprintf(stderr,"%s:%d: Invalid hex byte '%.2s'.\n",builder.srcpath,lineno,line+linep-2);
          return -2;
        }
        bin[binc++]=(hi<<4)|lo;
      }
      if (binc>=256) { // Table complete.
      
        if ((namec==7)&&!memcmp(name,"physics",7)) {
          if (sr_encode_raw(&builder.dst,bin,binc)<0) return -1;
        } // Ignore all other tables.
      
        binc=0;
        namec=0;
      }
      
    } else if (!linec) {
      // Blank lines are permitted between tables.
    } else {
      if (linec>=sizeof(name)) {
        fprintf(stderr,"%s:%d: Invalid table name, must be under %d bytes.\n",builder.srcpath,lineno,(int)sizeof(name));
        return -2;
      }
      memcpy(name,line,linec);
      namec=linec;
      name[namec]=0;
      
      // Ensure no duplicate tables, among ones we emit.
      if ((namec==7)&&!memcmp(name,"physics",7)) {
        if (have_physics) {
          fprintf(stderr,"%s:%d: Duplicate physics table.\n",builder.srcpath,lineno);
          return -2;
        }
        have_physics=1;
      }
    }
  }
  if (namec) {
    fprintf(stderr,"%s: Incomplete table at end.\n",builder.srcpath);
    return -2;
  }
  return 0;
}
