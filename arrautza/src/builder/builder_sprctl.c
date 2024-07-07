#include "builder.h"

const struct sprctl *sprctl_by_id(int id);

/* Read the header and compose the code in two encoders.
 * We don't require the sprctl and stobus blocks to be in any order, in fact they can be interleaved.
 * But the order of declarations within those types is hugely significant.
 */
 
static int compile_sprctl_stobus_inner(struct sr_encoder *sprctl,struct sr_encoder *stobus) {

  // Preambles.
  if (sr_encode_raw(sprctl,
    "static const struct sprctl *sprctl_by_idv[]={\n"
      "0,\n" // ID zero is always invalid.
  ,-1)<0) return -1;
  if (sr_encode_raw(stobus,
    "int define_stobus_fields() {\n"
  ,-1)<0) return -1;
  
  // Read header linewise.
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
      if (sr_encode_fmt(sprctl,"&%.*s,\n",idc,id)<0) return -1;
      continue;
    }
    
    if ((linec>=12)&&!memcmp(line,"#define FLD_",12)) {
      int linep=12;
      const char *name=line+linep;
      int namec=0;
      while ((linep<linec)&&((unsigned char)line[linep++]>0x20)) namec++;
      while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
      const char *idstr=line+linep;
      int idstrc=0;
      while ((linep<linec)&&((unsigned char)line[linep++]>0x20)) idstrc++;
      while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
      if ((linep>linec-2)||memcmp(line+linep,"/*",2)) {
        fprintf(stderr,"%s:%d: Malformed declaration line for field '%.*s'. Expected ' /* SIZE'\n",builder.srcpath,lineno,namec,name);
        return -2;
      }
      linep+=2;
      while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
      const char *sizestr=line+linep;
      int sizestrc=0;
      while ((linep<linec)&&((unsigned char)line[linep++]>0x20)) sizestrc++;
      int id,size;
      if ((sr_int_eval(&id,idstr,idstrc)<2)||(id<1)) {
        fprintf(stderr,"%s:%d: Invalid id '%.*s' for field '%.*s'\n",builder.srcpath,lineno,idstrc,idstr,namec,name);
        return -2;
      }
      if ((sr_int_eval(&size,sizestr,sizestrc)<2)||(size<0)||(size>31)) {
        fprintf(stderr,"%s:%d: Invalid size '%.*s' for field '%.*s'\n",builder.srcpath,lineno,sizestrc,sizestr,namec,name);
        return -2;
      }
      if (sr_encode_fmt(stobus,"  if (stobus_define(&g.stobus,%d,%d)<0) return -1;\n",id,size)<0) return -1;
      continue;
    }
  }
  
  // Postambles.
  if (sr_encode_raw(sprctl,
    "};\n"
    "const struct sprctl *sprctl_by_id(int id) {\n"
      "if (id<0) return 0;\n"
      "int c=sizeof(sprctl_by_idv)/sizeof(void*);\n"
      "if (id>=c) return 0;\n"
      "return sprctl_by_idv[id];\n"
    "}\n"
  ,-1)<0) return -1;
  if (sr_encode_raw(stobus,
    "  return 0;\n"
    "}\n"
  ,-1)<0) return -1;
  
  return 0;
}

/* Generate sprctl_by_id() from arrautza.h.
 * Also generate define_stobus_fields(), same idea.
 */
 
int builder_compile_sprctl() {
  struct sr_encoder sprctl={0},stobus={0};
  int err;
  if (
    ((err=sr_encode_raw(&builder.dst,"#include \"arrautza.h\"\n",-1))<0)||
    ((err=compile_sprctl_stobus_inner(&sprctl,&stobus))<0)||
    ((err=sr_encode_raw(&builder.dst,sprctl.v,sprctl.c))<0)||
    ((err=sr_encode_raw(&builder.dst,stobus.v,stobus.c))<0)||
  0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error generating code from main header.\n",builder.srcpath);
    err=-2;
  }
  sr_encoder_cleanup(&sprctl);
  sr_encoder_cleanup(&stobus);
  return err;
}
