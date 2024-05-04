/* text.h
 * Helpers for text processing.
 */
 
#ifndef TEXT_H
#define TEXT_H

/* If a valid UTF-8 sequence starts at (src), decode it into (codepoint) and return the encoded length.
 * Returns zero if empty or malformed.
 * We don't enforce the artificial upper limit U+10ffff, and we don't check for over-encoding.
 */
int text_utf8_decode(int *codepoint,const void *src,int srcc);

int text_utf8_encode(void *dst,int dsta,int codepoint);

#endif
