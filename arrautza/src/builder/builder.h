#ifndef BUILDER_H
#define BUILDER_H

#include "serial.h"
#include "arrautza.h" /* Not linking against any of this, but we will borrow some macros. */
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>

extern struct builder {
  const char *exename;
  const char *dstpath;
  const char *srcpath;
  const char *type;
  const char *hdrpath;
  void *src;
  int srcc;
  struct sr_encoder dst;
  struct restoc { // (restoca) will be nonzero if TOC load was attempted at all.
    int tid,rid;
    char *name;
    int namec;
  } *restocv;
  int restocc,restoca;
} builder;

/* Type-specific entry points.
 * (builder.src) will be populated already; these populate (builder.dst).
 */
int builder_compile_map();
int builder_compile_tilesheet();
int builder_compile_sprctl();
int builder_compile_sprite();

/* 1..63 on success, 0 on error. Type IDs are 6 bits and zero is forbidden.
 */
int builder_restype_eval(const char *src,int srcc);

/* 1..65535 on success, 0 on error.
 */
int builder_rid_eval(int tid,const char *src,int srcc);

/* 0..255 on success, -1 on error.
 */
int builder_item_eval(const char *src,int srcc);

/* 1..65535 on success, 0 on error.
 */
int builder_sprctl_eval(const char *src,int srcc);

/* >=0 on success. "FLD_" prefix optional.
 */
int builder_field_eval(int *size,const char *src,int srcc);

#endif
