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

/* Helpers for accessing string resources.
 * We employ some global state to track the language, and to cache content.
 ************************************************************************************/

/* Prepare globals, and select a default language.
 * Returns the selected language.
 */
int text_init();
 
/* Get a string resource in the global language.
 * Content is cached, so we return it WEAK.
 * Don't free or modify, and stop using if you change language.
 * Safely returns the empty string for any errors, never negative.
 */
int text_get_string(const char **v,int stringid);

/* Call (cb) for each language used by any string resource.
 * This is cheaper than you think; we gather that list at init and cache it.
 */
int text_for_each_language(int (*cb)(int lang,void *userdata),void *userdata);

/* Current language for returned string resources.
 * These are ISO 639 language codes, big-endianly, eg English "en" is 0x656e.
 * We reject languages that aren't two lowercase letters.
 * But we don't require that it actually have strings present.
 */
int text_get_language();
void text_set_language(int lang);

/* Get a field from resource "metadata:0:1".
 * Keys and values are both limited to 256 bytes.
 */
int text_get_metadata(char *dst,int dsta,const char *k,int kc);

#endif
