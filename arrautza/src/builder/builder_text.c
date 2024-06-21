#include "builder.h"
#include "fs.h"

/* Require resource TOC, from "mid/resid.h".
 * Will only attempt once, and return >=0 if loaded.
 */
 
static int builder_restoc_require() {
  if (builder.restoca>0) return (builder.restocc>0)?0:-1;
  if (!(builder.restocv=malloc(sizeof(struct restoc)*256))) return -1;
  builder.restoca=256;
  char *src=0;
  int srcc=file_read(&src,"mid/resid.h");
  if (srcc<0) return -1;
  struct sr_decoder decoder={.v=src,.c=srcc};
  const char *line;
  int linec;
  while ((linec=sr_decode_line(&line,&decoder))>0) {
    // Lines we're interested in look like this: #define RID_image_meadow 2
    if ((linec<12)||memcmp(line,"#define RID_",12)) continue;
    int linep=12;
    const char *tname=line+linep;
    int tnamec=0;
    while ((linep<linec)&&(line[linep]!='_')&&((unsigned char)line[linep]>0x20)) { linep++; tnamec++; }
    int tid=builder_restype_eval(tname,tnamec);
    if (!tid) continue;
    const char *rname=0;
    int rnamec=0;
    if ((linep<linec)&&(line[linep]=='_')) {
      linep++;
      rname=line+linep;
      while ((linep<linec)&&((unsigned char)line[linep]>0x20)) { linep++; rnamec++; }
    }
    // No sense keeping it if there isn't a name.
    if (!rnamec) continue;
    while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
    const char *ridsrc=line+linep;
    int ridsrcc=0;
    while ((linep<linec)&&((unsigned char)line[linep]>0x20)) { linep++; ridsrcc++; }
    int rid;
    if ((sr_int_eval(&rid,ridsrc,ridsrcc)<2)||(rid<1)||(rid>0xffff)) continue;
    // OK, add it to the list.
    if (builder.restocc>=builder.restoca) {
      int na=builder.restoca+256;
      if (na>INT_MAX/sizeof(struct restoc)) {
        free(src);
        return -1;
      }
      void *nv=realloc(builder.restocv,sizeof(struct restoc)*na);
      if (!nv) {
        free(src);
        return -1;
      }
      builder.restocv=nv;
      builder.restoca=na;
    }
    struct restoc *restoc=builder.restocv+builder.restocc++;
    restoc->tid=tid;
    restoc->rid=rid;
    if (!(restoc->name=malloc(rnamec+1))) {
      free(src);
      return -1;
    }
    memcpy(restoc->name,rname,rnamec);
    restoc->name[rnamec]=0;
    restoc->namec=rnamec;
  }
  free(src);
  return 0;
}

/* Search resource TOC.
 * We keep them in the order we found them, which should be sorted (tid,rid) but don't count on it.
 */
 
static int builder_restoc_search_id(int tid,int rid) {
  const struct restoc *restoc=builder.restocv;
  int i=0;
  for (;i<builder.restocc;i++,restoc++) {
    if (restoc->tid!=tid) continue;
    if (restoc->rid!=rid) continue;
    return i;
  }
  return -1;
}
 
static int builder_restoc_search_name(int tid,const char *name,int namec) {
  const struct restoc *restoc=builder.restocv;
  int i=0;
  for (;i<builder.restocc;i++,restoc++) {
    if (restoc->tid!=tid) continue;
    if (restoc->namec!=namec) continue;
    if (memcmp(restoc->name,name,namec)) continue;
    return i;
  }
  return -1;
}

/* Resource type.
 */
 
int builder_restype_eval(const char *src,int srcc) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  #define _(tag) if ((srcc==sizeof(#tag)-1)&&!memcmp(src,#tag,srcc)) return EGG_RESTYPE_##tag;
  EGG_RESTYPE_FOR_EACH
  CUSTOM_RESTYPE_FOR_EACH
  #undef _
  int v;
  if ((sr_int_eval(&v,src,srcc)>=2)&&(v>0)&&(v<0x40)) return v;
  return 0;
}

/* Resource ID.
 */
 
int builder_rid_eval(int tid,const char *src,int srcc) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int v;
  if ((sr_int_eval(&v,src,srcc)>=2)&&(v>0)&&(v<=0xffff)) return v;
  if (builder_restoc_require()>=0) {
    int p=builder_restoc_search_name(tid,src,srcc);
    if (p>=0) return builder.restocv[p].rid;
  }
  return 0;
}
