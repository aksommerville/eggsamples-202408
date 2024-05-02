/* fkbd.h
 * Fake keyboard.
 */
 
#ifndef FKBD_H
#define FKBD_H

struct fkbd;
struct bus;
struct font;

void fkbd_del(struct fkbd *fkbd);

struct fkbd *fkbd_new(struct bus *bus);

/* Set the characters to show.
 * We are a set of pages, each of uniform size.
 * Each cell is a Unicode codepoint.
 * Two special codepoints that we consume (we won't report them as events):
 *   U+0001 Next page.
 *   U+0002 Previous page.
 *
 * The content you provide is ordered column, row, page. eg:
 *   [ 1, 2, 3, 4, 5, 6, 7, 8 ]
 *   If (colc,rowc,pagec) are all 2.
 *   First page is:
 *      1 2
 *      3 4
 *   Second page is:
 *      5 6
 *      7 8
 *
 * If you don't call this, at the first enable you'll get 
 * a set of pages containing U+20..U+7e, U+08, and U+0a.
 */
int fkbd_set_keycaps(struct fkbd *fkbd,int colc,int rowc,int pagec,const int *codepointv);

/* Give us either a font object, or a 256-character tilesheet.
 * Using a tilesheet, only codepoints U+0..U+ff are printable.
 * Whichever you provide last wins, we'll forget the other.
 * We borrow both things. You must keep them alive forever.
 */
void fkbd_set_font(struct fkbd *fkbd,struct font *font);
void fkbd_set_tilesheet(struct fkbd *fkbd,int texid);

void fkbd_event(struct fkbd *fkbd,const struct egg_event *event);
void fkbd_update(struct fkbd *fkbd,double elapsed);
void fkbd_render(struct fkbd *fkbd);

void fkbd_enable(struct fkbd *fkbd,int enable);

#endif
