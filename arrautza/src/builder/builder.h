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

/* 1..63 on success, 0 on error. Type IDs are 6 bits and zero is forbidden.
 */
int builder_restype_eval(const char *src,int srcc);

/* 1..65535 on success, 0 on error.
 */
int builder_rid_eval(int tid,const char *src,int srcc);

#endif
