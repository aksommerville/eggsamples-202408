#include "builder.h"

const struct sprctl *sprctl_by_id(int id);

/* Generate sprctl_by_id() from arrautza.h, main entry point.
 */
 
int builder_compile_sprctl() {
  
  if (sr_encode_raw(&builder.dst,
    "#include \"arrautza.h\"\n"
    "static const struct sprctl *sprctl_by_idv[]={\n"
      "0,\n" // ID zero is always invalid.
  ,-1)<0) return -1;
  
  struct sr_decoder decoder={.v=builder.src,.c=builder.srcc};
  int lineno=1,linec;
  const char *line;
  for (;(linec=sr_decode_line(&line,&decoder))>0;lineno++) {
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { line++; linec--; }
    if (!linec) continue;

    // It is important that we assign these sequentially from 1; builder_text.c:builder_sprctl_eval() depends on it.
    if ((linec>=27)&&!memcmp(line,"extern const struct sprctl ",27)) {
      int idp=27;
      while ((idp<linec)&&((unsigned char)line[idp]<=0x20)) idp++;
      const char *id=line+idp;
      int idc=0;
      while (idp+idc<linec) {
        char ch=id[idc];
             if ((ch>='a')&&(ch<='z')) ;
        else if ((ch>='A')&&(ch<='Z')) ;
        else if ((ch>='0')&&(ch<='9')) ;
        else if (ch=='_') ;
        else break;
        idc++;
      }
      if (!idc) {
        fprintf(stderr,"%s:%d: Malformed sprctl declaration line.\n",builder.srcpath,lineno);
        return -2;
      }
      if (sr_encode_fmt(&builder.dst,"&%.*s,\n",idc,id)<0) return -1;
    }
  }
  
  if (sr_encode_raw(&builder.dst,
    "};\n"
    "const struct sprctl *sprctl_by_id(int id) {\n"
      "if (id<0) return 0;\n"
      "int c=sizeof(sprctl_by_idv)/sizeof(void*);\n"
      "if (id>=c) return 0;\n"
      "return sprctl_by_idv[id];\n"
    "}\n"
  ,-1)<0) return -1;
  
  return 0;
}
