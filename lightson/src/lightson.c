/* lightson.c
 * An Egg ROM that touches the entire public API.
 */
 
#include <egg/egg.h>
#include "resid.h" // Gets built by our Makefile, with eggdev's help.
#include "util/tile_renderer.h"

// We of course know this already, it's in our metadata file.
// I feel it's safer to read back from the API during init.
static int screenw,screenh;

static int texid_font_tiles;
static struct tile_renderer tile_renderer={0};

/* Quit.
 * This usually stays empty.
 */
 
void egg_client_quit() {
}

/* Init.
 */

int egg_client_init() {

  egg_texture_get_header(&screenw,&screenh,0,1);
  
  if (egg_texture_load_image(texid_font_tiles=egg_texture_new(),0,RID_image_font_tiles)<0) return -1;
  
  #define ENEV(tag) if (!egg_event_enable(EGG_EVENT_##tag,1)) egg_log("Failed to enable %s events.",#tag);
  ENEV(JOY) // JOY, KEY, and TEXT are enabled by default anyway.
  ENEV(KEY)
  ENEV(TEXT)
  ENEV(MMOTION) // Enabling any mouse event also makes the system cursor visible.
  ENEV(MBUTTON)
  ENEV(MWHEEL)
  ENEV(TOUCH) // Enabled by default.
  ENEV(ACCEL) // Bad idea to enable, it causes a lot of noise if it actually exists.
  ENEV(RAW) // Prefer JOY.
  #undef ENEV

  return 0;
}

/* Event reception.
 */
 
static void on_joy(const struct egg_event_joy *event) {
  //egg_log("%s devid=%d btnid=%d value=%d",__func__,event->devid,event->btnid,event->value);
}

static void on_key(const struct egg_event_key *event) {
  //egg_log("%s keycode=0x%x value=%d",__func__,event->keycode,event->value);
  if (event->value) switch (event->keycode) {
    case 0x00070029: egg_request_termination(); break; // Escape
  }
}

static void on_text(const struct egg_event_text *event) {
  //egg_log("%s U+%x",__func__,event->codepoint);
}

static void on_mmotion(const struct egg_event_mmotion *event) {
  //egg_log("%s @%d,%d",__func__,event->x,event->y);
}

static void on_mbutton(const struct egg_event_mbutton *event) {
  //egg_log("%s %d=%d @%d,%d",__func__,event->btnid,event->value,event->x,event->y);
}

static void on_mwheel(const struct egg_event_mwheel *event) {
  //egg_log("%s %d,%d @%d,%d",__func__,event->dx,event->dy,event->x,event->y);
}

static void on_touch(const struct egg_event_touch *event) {
  //egg_log("%s touchid=%d state=%d @%d,%d",__func__,event->touchid,event->state,event->x,event->y);
}

static void on_accel(const struct egg_event_accel *event) {
  //egg_log("%s %d,%d,%d",__func__,event->x,event->y,event->z);
}

static void on_raw(const struct egg_event_raw *event) {
  //egg_log("%s devid=%d btnid=%d value=%d",__func__,event->devid,event->btnid,event->value);
}

/* Update.
 */

void egg_client_update(double elapsed) {

  /* It's wise to poll multiple events at once, more efficient.
   * But you could egg_event_get(&event,1) for legibility too, not a big deal.
   * We're calling out every possible event type, since we're a demonstration. That's overkill.
   */
  union egg_event eventv[16];
  int eventc;
  while ((eventc=egg_event_get(eventv,16))>0) {
    const union egg_event *event=eventv;
    int i=eventc; for (;i-->0;event++) switch (event->type) {
      case EGG_EVENT_JOY: on_joy(&event->joy); break;
      case EGG_EVENT_KEY: on_key(&event->key); break;
      case EGG_EVENT_TEXT: on_text(&event->text); break;
      case EGG_EVENT_MMOTION: on_mmotion(&event->mmotion); break;
      case EGG_EVENT_MBUTTON: on_mbutton(&event->mbutton); break;
      case EGG_EVENT_MWHEEL: on_mwheel(&event->mwheel); break;
      case EGG_EVENT_TOUCH: on_touch(&event->touch); break;
      case EGG_EVENT_ACCEL: on_accel(&event->accel); break;
      case EGG_EVENT_RAW: on_raw(&event->raw); break;
      default: egg_log("Unknown event type %d!",event->type);
    }
  }
}

/* Render.
 */

void egg_client_render() {
  egg_draw_rect(1,0,0,screenw,screenh,0x203040ff);
  
  tile_renderer_begin(&tile_renderer,texid_font_tiles,0xffff00ff,0xff);
  tile_renderer_string(&tile_renderer,8,8,"Yello, world!",-1);
  tile_renderer_end(&tile_renderer);
}
