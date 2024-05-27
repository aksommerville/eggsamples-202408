/* log.h
 * Manages the read-only text area at the bottom of the screen.
 */

#include "hardboiled.h"
#include "verblist.h"

#define LINE_COUNT 3
#define DSTX 4
#define DSTY 105
#define DSTW 232
#define DSTH 27
#define LOG_TYPEWRITER_TIME 0.050

/* Globals.
 */
 
static char log_text[1024];
static int log_textc=0; // How much text is there? Updates immediately on sets.
static int log_printc=0; // How much of it are we displaying right now? 0..log_textc
static int log_texid=0;
static int log_texw=0,log_texh=0;
static double log_clock=0.0; // Counts down to next codepoint out.

/* Clear text.
 */
 
void log_clear() {
  log_textc=0;
  log_printc=0;
  egg_texture_clear(log_texid);
}

/* Eliminate head text through the first newline.
 * No newlines? Eliminate all.
 */
 
static void log_drop_head() {
  int nlp=-1,i=0;
  for (;i<log_textc;i++) {
    if (log_text[i]==0x0a) {
      nlp=i;
      break;
    }
  }
  if (nlp<0) {
    log_textc=0;
    return;
  }
  nlp++; // Eliminate the LF itself too.
  log_textc-=nlp;
  if ((log_printc-=nlp)<0) log_printc=0;
  memmove(log_text,log_text+nlp,log_textc);
}
 
/* Append raw text and newline to log, drop any descoped head, and rerender.
 */
 
void log_add_text(const char *src,int srcc) {
  if (!src) return;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (!srcc) return;
  
  if (!log_texid) {
    if ((log_texid=egg_texture_new())<1) {
      log_texid=0;
      log_printc=0;
      return;
    }
  }
  
  // Drop head until it fits, at least bytewise.
  int addc=srcc+1; // We'll be adding an LF too.
  if (addc>sizeof(log_text)) return;
  while (addc>sizeof(log_text)-log_textc) {
    log_drop_head();
  }
  memcpy(log_text+log_textc,src,srcc);
  log_textc+=srcc;
  log_text[log_textc++]=0x0a;
  
  // Have the font split it into lines.
  // If we produce more than 3, drop head to only the last 3.
  // If we produce more than 12, panic.
  int startv[LINE_COUNT*4];
  int startc=font_break_lines(startv,LINE_COUNT,font,log_text,log_textc,DSTW);
  if ((startc<0)||(startc>LINE_COUNT*4)) {
    egg_log("%s:%d: Too many lines of text! (%d)",__FILE__,__LINE__,startc);
    log_textc=0;
    log_printc=0;
    startc=0;
  } else if (startc>LINE_COUNT) {
    int trimc=startv[startc-LINE_COUNT];
    log_textc-=trimc;
    if ((log_printc-=trimc)<0) log_printc=0;
    memmove(log_text,log_text+trimc,log_textc);
    memmove(startv,startv+startc-LINE_COUNT,sizeof(int)*LINE_COUNT);
    startc=LINE_COUNT;
  }
}
 
/* Append string resource to log.
 */
 
void log_add_string_keep_verb(int stringid) {
  const char *src=0;
  int srcc=text_get_string(&src,stringid);
  if (srcc<1) return;
  log_add_text(src,srcc);
}
 
void log_add_string(int stringid) {
  verblist_unselect(); // Arguably not in our writ, but I figure adding a string is always a response to some action and should then reset.
  const char *src=0;
  int srcc=text_get_string(&src,stringid);
  if (srcc<1) return;
  log_add_text(src,srcc);
}

/* Print one more character.
 */
 
static void log_advance_typewriter() {
  if (log_printc>=log_textc) return;
  int codepoint,seqlen;
  if ((seqlen=text_utf8_decode(&codepoint,log_text+log_printc,log_textc-log_printc))<1) {
    seqlen=1;
  }
  log_printc+=seqlen;
  font_render_to_texture(log_texid,font,log_text,log_printc,DSTW,0x000000);
  egg_texture_get_header(&log_texw,&log_texh,0,log_texid);
}

/* Update.
 */
 
void log_update(double elapsed) {
  if (log_printc<log_textc) {
    if ((log_clock-=elapsed)<0.0) {
      log_clock+=LOG_TYPEWRITER_TIME;
      log_advance_typewriter();
    }
  } else {
    log_clock=0.0;
  }
}

/* Render.
 * The background image contains our border and background, we only blit text on top of it.
 */
 
void log_render() {
  if (log_texid) {
    egg_draw_decal(1,log_texid,DSTX,DSTY,0,0,log_texw,log_texh,0);
  }
}
