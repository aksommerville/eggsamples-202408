#include "stobus.h"
#include <stdint.h>
#include <egg/egg.h>
#include <stdlib/egg-stdlib.h>

/* Cleanup.
 */
 
void stobus_cleanup(struct stobus *stobus) {
  if (stobus->listenerc) {
    // It's not a big deal for listeners to go down with the ship, but it might indicate a flaw in your code, hence the warning.
    egg_log("!!!WARNING!!! Deleting stobus with %d listeners stll attached.",stobus->listenerc);
  }
  if (stobus->fieldv) free(stobus->fieldv);
  if (stobus->listenerv) free(stobus->listenerv);
  memset(stobus,0,sizeof(struct stobus));
}

/* Define field.
 */
 
int stobus_define(struct stobus *stobus,int id,int size_bits) {
  if (id<1) return -1;
  if ((size_bits<0)||(size_bits>31)) return -1;
  int p=stobus_fieldv_search(stobus,id);
  if (p>=0) {
    if (stobus->fieldv[p].c==size_bits) return 0;
    return -1;
  }
  p=-p-1;
  struct stobus_field *field=stobus_fieldv_insert(stobus,p,id);
  if (!field) return -1;
  field->c=size_bits;
  return 0;
}

/* Add listener.
 */
 
int stobus_listen(struct stobus *stobus,int id,void (*cb)(int id,int v,void *userdata),void *userdata) {
  if ((id<1)||!cb) return -1;
  int listenerid=1;
  if (stobus->listenerc>0) {
    listenerid=stobus->listenerv[stobus->listenerc-1].listenerid+1;
    if (listenerid<1) return -1; // We'll fail after 2G listens.
  }
  if (stobus->listenerc>=stobus->listenera) {
    int na=stobus->listenera+16;
    if (na>INT_MAX/sizeof(struct stobus_listener)) return -1;
    void *nv=realloc(stobus->listenerv,sizeof(struct stobus_listener)*na);
    if (!nv) return -1;
    stobus->listenerv=nv;
    stobus->listenera=na;
  }
  struct stobus_listener *listener=stobus->listenerv+stobus->listenerc++;
  listener->listenerid=listenerid;
  listener->fldid=id;
  listener->cb=cb;
  listener->userdata=userdata;
  return listenerid;
}

/* Remove listener.
 */
 
void stobus_unlisten(struct stobus *stobus,int listenerid) {
  int p=stobus_listenerv_search(stobus,listenerid);
  if (p<0) return;
  struct stobus_listener *listener=stobus->listenerv+p;
  stobus->listenerc--;
  memmove(listener,listener+1,sizeof(struct stobus_listener)*(stobus->listenerc-p));
}

/* Get field.
 */
 
int stobus_get(struct stobus *stobus,int id) {
  int p=stobus_fieldv_search(stobus,id);
  if (p<0) return 0;
  return stobus->fieldv[p].v;
}

/* Set field.
 */
 
void stobus_set(struct stobus *stobus,int id,int v) {
  int p=stobus_fieldv_search(stobus,id);
  if (p<0) return;
  struct stobus_field *field=stobus->fieldv+p;
  if (field->c) { // stateless fields, we preserve the provided value for callbacks.
    if (v<0) v=0;
    unsigned int limit=(1u<<field->c)-1;
    if ((unsigned int)v>limit) v=limit;
    if (v==field->v) return;
    field->v=v;
    stobus->dirty=1;
  }
  int i=stobus->listenerc;
  while (i-->0) {
    const struct stobus_listener *listener=stobus->listenerv+i;
    if (listener->fldid!=id) continue;
    listener->cb(id,v,listener->userdata);
  }
}

/* Encode.
 */
 
int stobus_encode(char *dst,int dsta,const struct stobus *stobus) {
  /* Three passes:
   *  1. Calculate output size in bits, then bytes.
   *  2. Pack fields little-endianly bitwise, 6 bits per output byte.
   *  3. Apply the base64 alphabet to each byte of output.
   */
  int bitc=0,i;
  const struct stobus_field *field=stobus->fieldv;
  for (i=stobus->fieldc;i-->0;field++) bitc+=field->c;
  int dstc=(bitc+7)>>3;
  if (dstc>dsta) return dstc;
  
  memset(dst,0,dstc);
  int dstp=0;
  uint8_t dstmask=1;
  for (field=stobus->fieldv,i=stobus->fieldc;i-->0;field++) {
    int bi=field->c;
    int vmask=1;
    // Copy one bit at a time. This could be done more efficiently, but it would be complicated.
    for (;bi-->0;vmask<<=1) {
      if (field->v&vmask) dst[dstp]|=dstmask;
      if (dstmask==0x20) { dstmask=1; dstp++; }
      else dstmask<<=1;
    }
  }
  
  const char *alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  uint8_t *p=(uint8_t*)dst;
  for (i=dstc;i-->0;p++) (*p)=alphabet[*p];
  
  if (dstc<dsta) dst[dstc]=0;
  return dstc;
}

/* Decode.
 */

int stobus_decode(struct stobus *stobus,const char *src,int srcc) {
  if ((srcc<0)||(srcc&!src)) return -1;
  while (srcc&&(src[srcc-1]=='=')) srcc--;
  
  // Take one pass over (src) just to validate format, before we touch anything.
  int i=srcc; for (;i-->0;) {
    if ((src[i]>='A')&&(src[i]<='Z')) continue;
    if ((src[i]>='a')&&(src[i]<='z')) continue;
    if ((src[i]>='0')&&(src[i]<='9')) continue;
    if (src[i]=='+') continue;
    if (src[i]=='/') continue;
    return -1;
  }
  
  // Read six bits at a time from (src), and step fields responsively.
  if (stobus->fieldc>0) {
    struct stobus_field *field=stobus->fieldv;
    field->v=0;
    int fieldi=0;
    int fieldmask=1;
    int fieldpending=field->c;
    while (!fieldpending) {
      if (++fieldi>=stobus->fieldc) break;
      field++;
      fieldpending=field->c;
    }
    if (fieldi<stobus->fieldc) {
      for (;srcc-->0;src++) {
        uint8_t inbyte=*src;
             if ((inbyte>='A')&&(inbyte<='Z')) inbyte=inbyte-'A';
        else if ((inbyte>='a')&&(inbyte<='z')) inbyte=inbyte-'a'+26;
        else if ((inbyte>='0')&&(inbyte<='9')) inbyte=inbyte-'0'+52;
        else if (inbyte=='+') inbyte=62;
        else if (inbyte=='/') inbyte=63;
        else return -1;
        uint8_t inmask=1;
        for (;inmask<0x40;inmask<<=1) {
          if (inbyte&inmask) field->v|=fieldmask;
          if (fieldpending--) {
            fieldmask<<=1;
          } else {
            if (++fieldi>=stobus->fieldc) break;
            field++;
            field->v=0;
            fieldmask=1;
            fieldpending=field->c;
            while (!fieldpending) {
              if (++fieldi>=stobus->fieldc) break;
              field++;
              fieldpending=field->c;
            }
          }
        }
        if (fieldi>=stobus->fieldc) break;
      }
    }
    // Any fields we haven't reached get value zero.
    for (;;) {
      fieldi++;
      if (fieldi>=stobus->fieldc) break;
      field++;
      field->v=0;
    }
  }
  
  stobus->dirty=0;
  return 0;
}

/* Field list.
 */

int stobus_fieldv_search(const struct stobus *stobus,int id) {
  int lo=0,hi=stobus->fieldc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct stobus_field *field=stobus->fieldv+ck;
         if (id<field->id) hi=ck;
    else if (id>field->id) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

struct stobus_field *stobus_fieldv_insert(struct stobus *stobus,int p,int id) {
  if (id<1) return 0;
  if ((p<0)||(p>stobus->fieldc)) return 0;
  if (p&&(id<=stobus->fieldv[p-1].id)) return 0;
  if ((p<stobus->fieldc)&&(id>=stobus->fieldv[p].id)) return 0;
  if (stobus->fieldc>=stobus->fielda) {
    int na=stobus->fielda+128;
    if (na>INT_MAX/sizeof(struct stobus_field)) return 0;
    void *nv=realloc(stobus->fieldv,sizeof(struct stobus_field)*na);
    if (!nv) return 0;
    stobus->fieldv=nv;
    stobus->fielda=na;
  }
  struct stobus_field *field=stobus->fieldv+p;
  memmove(field+1,field,sizeof(struct stobus_field)*(stobus->fieldc-p));
  stobus->fieldc++;
  field->id=id;
  field->c=0;
  field->v=0;
  return field;
}

/* Listener list.
 */

int stobus_listenerv_search(const struct stobus *stobus,int id) {
  int lo=0,hi=stobus->listenerc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct stobus_listener *listener=stobus->listenerv+ck;
         if (id<listener->listenerid) hi=ck;
    else if (id>listener->listenerid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}
