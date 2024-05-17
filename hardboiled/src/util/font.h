/* font.h
 * Generate images of text with fixed row height but variable glyph width.
 * Input text must be encoded UTF-8, and all codepoints are supported, if you supply glyphs for them.
 * Page images contain glyphs for some contiguous range of codepoints.
 * These images' height must be a multiple of the font's row height.
 * They must be 1-bit.
 * You can split images into rows any way you like; my examples generally split at multiples of 16 for no particular reason.
 * Empty glyphs are not permitted, and glyphs with horizontal separation (eg ") are not permitted.
 * We artificially produce empty glyphs for U+09, U+20, and U+a0.
 */
 
#ifndef FONT_H
#define FONT_H

struct font;

void font_del(struct font *font);

/* You must know the row height, and for each image its ID and first codepoint.
 * During font_add_page(), we fetch the image and slice it into glyphs.
 * Adding a page will fail if it overlaps any existing page.
 */
struct font *font_new(int rowh);
int font_add_page(struct font *font,int imageid,int codepoint);

int font_get_rowh(const struct font *font);
int font_count_glyphs(const struct font *font);
int font_count_pages(const struct font *font);

/* Drop any existing content attached to this texture, and replace with an RGBA image of some text.
 * Use "_new_" to allocate a new texture first; caller must eventually delete it with egg_texture_del().
 * If (wlimit>0), break lines first. Otherwise it's a single line (even if LF present).
 * These are usually what you want. Everything below is for a more piecemeal approach.
 */
int font_render_to_texture(int texid,const struct font *font,const char *src,int srcc,int wlimit,int rgb);
int font_render_new_texture(const struct font *font,const char *src,int srcc,int wlimit,int rgb);

/* Width of this string in pixels, if it were rendered in one line.
 */
int font_measure_glyph(const struct font *font,int codepoint);
int font_measure(const struct font *font,const char *src,int srcc);

/* Populate (startv) with positions in (src) where each line of text should begin.
 * The length of each line is (startv[n+1]-startv[n]), but the last is (srcc-startv[n]).
 * You'll want to trim whitespace from the end of each -- trailing whitespace is allowed to exceed (wlimit).
 * Nonwhitespace content only exceeds (wlimit) if a single glyph is too wide.
 * If we return (>starta), all positions up to (starta) are faithfully recorded. Your call whether to try again with a larger buffer.
 */
int font_break_lines(int *startv,int starta,const struct font *font,const char *src,int srcc,int wlimit);

/* Render one line of text onto an RGBA or A1 image.
 * (dstx,dsty) are the top-left corner of the first glyph.
 * Returns the rendered width in pixels.
 * May be helpful if you need multi-color text, or other custom features.
 */
int font_render_string_rgba(
  void *dst,int dstw,int dsth,int dststride,
  int dstx,int dsty,
  const struct font *font,
  const char *src,int srcc,
  int rgb
);
int font_render_string_a1(
  void *dst,int dstw,int dsth,int dststride,
  int dstx,int dsty,
  const struct font *font,
  const char *src,int srcc
);

/* Render a single glyph on an existing image.
 * (dstx,dsty) is the *center* of the output glyph (not top-left, as most of our other functions expect).
 * I use this for rendering keycaps on the onscreen keyboard.
 */
int font_render_char_rgba(
  void *dst,int dstw,int dsth,int dststride,
  int dstx,int dsty,
  const struct font *font,
  int codepoint,
  int rgb
);
int font_render_char_a1(
  void *dst,int dstw,int dsth,int dststride,
  int dstx,int dsty,
  const struct font *font,
  int codepoint
);

#endif
