#include <egg/egg.h>
#include "text.h"
#if USE_REAL_STDLIB
  #include <stdlib.h>
  #include <string.h>
  #include <limits.h>
#else
  #include "stdlib/egg-stdlib.h"
#endif

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

/* Globals.
 */
 
static struct {
  int lang;
  int *langv;
  int langc,langa;
  struct text_string {
    int rid;
    int c;
    char *v;
  } *stringv;
  int stringc,stringa;
  char *metav;
  int metac;
} text={0};

/* Test language present.
 */
 
static int text_language_exists(int lang) {
  int lo=0,hi=text.langc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    int q=text.langv[ck];
         if (lang<q) hi=ck;
    else if (lang>q) lo=ck+1;
    else return 1;
  }
  return 0;
}

/* Language codes and resource qualifiers.
 */
 
static void qual_repr(char *dst/*2*/,int qual) {
  dst[0]="012345abcdefghijklmnopqrstuvwxyz"[(qual>>5)&31];
  dst[1]="012345abcdefghijklmnopqrstuvwxyz"[qual&31];
}

static inline int qual_eval_1(char src) {
  if ((src>='0')&&(src<='5')) return src-'0';
  if ((src>='a')&&(src<='z')) return src-'a'+6;
  return -1;
}

static int qual_eval(const char *src,int srcc) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (srcc!=2) return 0;
  int hi=qual_eval_1(src[0]);
  if (hi<0) return 0;
  int lo=qual_eval_1(src[1]);
  if (lo<0) return 0;
  return (hi<<5)|lo;
}

/* Look for string resources and record languages.
 */
 
static int text_cb_for_res(int tid,int qual,int rid,int len,void *userdata) {
  if (tid<EGG_RESTYPE_string) return 0;
  if (tid>EGG_RESTYPE_string) return 1;
  if (text.langc&&(text.langv[text.langc-1]==qual)) return 0;
  char qstr[2];
  qual_repr(qstr,qual);
  if ((qstr[0]<'a')||(qstr[0]>'z')||(qstr[1]<'a')||(qstr[1]>'z')) return 0;
  if (text.langc>=text.langa) {
    int na=text.langa+8;
    void *nv=realloc(text.langv,sizeof(int)*na);
    if (!nv) return -1;
    text.langv=nv;
    text.langa=na;
  }
  text.langv[text.langc++]=qual;
  return 0;
}

/* Init.
 */

int text_init() {

  // Populate list of available languages, from the string resources.
  egg_res_for_each(text_cb_for_res,0);
  
  // Acquire the user's preferred languages.
  int userpref[16];
  int userprefc=egg_get_user_languages(userpref,sizeof(userpref)/sizeof(int));
  if (userprefc<0) userprefc=0;
  else if (userprefc>sizeof(userpref)/sizeof(int)) userprefc=sizeof(userpref)/sizeof(int);
  
  // Best case: Walk the user's languages in order, and take the first one supported.
  int userprefi=0;
  for (;userprefi<userprefc;userprefi++) {
    if (text_language_exists(userpref[userprefi])) {
      text.lang=userpref[userprefi];
      break;
    }
  }
  
  // Suboptimal: Take the first of the game's declared languages to actually exist.
  // In a sane universe, that means the first declared language.
  if (!text.lang) {
    char gamepref[256];
    int gameprefc=text_get_metadata(gamepref,sizeof(gamepref),"language",8);
    if ((gameprefc>0)&&(gameprefc<=sizeof(gamepref))) {
      int p=0;
      while (p<gameprefc) {
        const char *token=gamepref+p;
        int tokenc=0;
        while ((p<gameprefc)&&(gamepref[p++]!=',')) tokenc++;
        while (tokenc&&((unsigned char)token[tokenc-1]<=0x20)) tokenc--;
        while (tokenc&&((unsigned char)token[0]<=0x20)) { tokenc--; token++; }
        int lang=qual_eval(token,tokenc);
        if (text_language_exists(lang)) {
          text.lang=lang;
          break;
        }
      }
    }
  }
  
  // Even worse: Take the first language we discovered in any string resource.
  if (!text.lang&&text.langc) {
    text.lang=text.langv[0];
  }
  
  // And the very worst: Call it English. Can only land here if there are no strings, so I guess it doesn't matter.
  if (!text.lang) {
    text.lang=339; // en
  }

  return text.lang;
}

/* Get string.
 */
 
static int text_stringv_search(int rid) {
  int lo=0,hi=text.stringc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct text_string *q=text.stringv+ck;
         if (rid<q->rid) hi=ck;
    else if (rid>q->rid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}
 
static struct text_string *text_string_insert(int p) {
  if ((p<0)||(p>text.stringc)) return 0;
  if (text.stringc>=text.stringa) {
    int na=text.stringa+32;
    if (na>INT_MAX/sizeof(struct text_string)) return 0;
    void *nv=realloc(text.stringv,sizeof(struct text_string)*na);
    if (!nv) return 0;
    text.stringv=nv;
    text.stringa=na;
  }
  struct text_string *string=text.stringv+p;
  memmove(string+1,string,sizeof(struct text_string)*(text.stringc-p));
  text.stringc++;
  memset(string,0,sizeof(struct text_string));
  return string;
}
 
int text_get_string(const char **v,int stringid) {
  if (stringid<1) {
    *v="";
    return 0;
  }
  int p=text_stringv_search(stringid);
  if (p>=0) {
    const struct text_string *string=text.stringv+p;
    *v=string->v;
    return string->c;
  }
  p=-p-1;
  int tmpa=32,tmpc=0;
  char *tmpv=malloc(tmpa);
  if (!tmpv) { *v=""; return 0; }
  for (;;) {
    tmpc=egg_res_get(tmpv,tmpa,EGG_RESTYPE_string,text.lang,stringid);
    if (tmpc<0) tmpc=0;
    if (tmpc<tmpa) {
      tmpv[tmpc]=0;
      break;
    }
    char *nv=realloc(tmpv,tmpa=tmpc+1);
    if (!nv) { free(tmpv); *v=""; return 0; }
    tmpv=nv;
  }
  struct text_string *string=text_string_insert(p);
  if (!string) { free(tmpv); *v=""; return 0; }
  string->rid=stringid;
  string->v=tmpv;
  string->c=tmpc;
  *v=tmpv;
  return tmpc;
}

/* For each language.
 */

int text_for_each_language(int (*cb)(int lang,void *userdata),void *userdata) {
  const int *v=text.langv;
  int i=text.langc,err;
  for (;i-->0;v++) if (err=cb(*v,userdata)) return err;
  return 0;
}

/* Current language.
 */

int text_get_language() {
  return text.lang;
}

void text_set_language(int lang) {
  if (lang==text.lang) return;
  if (lang&~0x03ff) return;
  text.lang=lang;
  while (text.stringc>0) {
    struct text_string *string=text.stringv+(--(text.stringc));
    if (string->v) free(string->v);
  }
}

/* Acquire metadata:0:1 if we don't have it yet.
 */
 
static int text_require_metadata() {
  if (text.metav) return 0;
  text.metac=0;
  int a=1024,c=0;
  if (!(text.metav=malloc(a))) return -1;
  for (;;) {
    c=egg_res_get(text.metav,a,EGG_RESTYPE_metadata,0,1);
    if (c<0) c=0;
    if (c<=a) {
      text.metac=c;
      return 0;
    }
    void *nv=realloc(text.metav,a=c);
    if (!nv) return -1;
    text.metav=nv;
  }
}

/* Get metadata field.
 */
 
int text_get_metadata(char *dst,int dsta,const char *k,int kc) {
  if (!k) kc=0; else if (kc<0) { kc=0; while (k[kc]) kc++; }
  text_require_metadata();
  const uint8_t *src=(uint8_t*)text.metav;
  int srcc=text.metac;
  int srcp=0;
  for (;;) {
    if (srcp>srcc-2) break;
    int qkc=src[srcp++];
    int qvc=src[srcp++];
    if (srcp>srcc-qkc-qvc) break;
    const char *qk=(char*)(src+srcp);
    srcp+=qkc;
    const char *qv=(char*)(src+srcp);
    srcp+=qvc;
    if (qkc!=kc) continue;
    if (memcmp(qk,k,kc)) continue;
    if (qvc<=dsta) {
      memcpy(dst,qv,qvc);
      if (qvc<dsta) dst[qvc]=0;
    }
    return qvc;
  }
  if (dsta>=1) dst[0]=0;
  return 0;
}
