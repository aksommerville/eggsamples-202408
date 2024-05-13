#include <egg/egg.h>
#include "menu.h"
#include "util/font.h"
#include "util/tile_renderer.h"
#include "stdlib/egg-stdlib.h"

// We have screen space for 25.
// Lots of systems won't go above 4 devices. We have no such limit, though more than 4 is pretty weird.
#define DEVICE_LIMIT 32

// The first 17 state bits match the order of EGG_JOYBTN_ from 0x80 thru 0x90.
#define STATE_SOUTH    0x00000001
#define STATE_EAST     0x00000002
#define STATE_WEST     0x00000004
#define STATE_NORTH    0x00000008
#define STATE_L1       0x00000010
#define STATE_R1       0x00000020
#define STATE_L2       0x00000040
#define STATE_R2       0x00000080
#define STATE_AUX2     0x00000100
#define STATE_AUX1     0x00000200
#define STATE_LP       0x00000400
#define STATE_RP       0x00000800
#define STATE_UP       0x00001000
#define STATE_DOWN     0x00002000
#define STATE_LEFT     0x00004000
#define STATE_RIGHT    0x00008000
#define STATE_AUX3     0x00010000
// Then 8 more bits for the analogue sticks.
#define STATE_LXLO     0x00020000
#define STATE_LXHI     0x00040000
#define STATE_LYLO     0x00080000
#define STATE_LYHI     0x00100000
#define STATE_RXLO     0x00200000
#define STATE_RXHI     0x00400000
#define STATE_RYLO     0x00800000
#define STATE_RYHI     0x01000000

struct menu_joystick {
  struct menu hdr;
  struct device {
    int devid;
    int state;
  } devicev[DEVICE_LIMIT];
  int devicec;
  int colc,rowc;
  int colw,rowh;
  int xmargin,ymargin;
  int tilesize;
};

#define MENU ((struct menu_joystick*)menu)

static void _joystick_del(struct menu *menu) {
}

static void joystick_on_joy(struct menu *menu,const struct egg_event_joy *event) {
  struct device *device=0;
  struct device *q=MENU->devicev;
  int i=MENU->devicec;
  for (;i-->0;q++) {
    if (q->devid==event->devid) {
      device=q;
      break;
    }
  }
  if (!device) {
    if ((event->btnid==0)&&event->value) {
      if (MENU->devicec<DEVICE_LIMIT) {
        device=MENU->devicev+MENU->devicec++;
        memset(device,0,sizeof(struct device));
        device->devid=event->devid;
      }
    }
    return;
  }
  if ((event->btnid==0)&&!event->value) {
    MENU->devicec--;
    memmove(device,device+1,sizeof(struct device)*(MENU->devicec*(device-MENU->devicev)));
    return;
  }
  int mask=0,value=event->value;
  if ((event->btnid>=0x80)&&(event->btnid<=0x90)) {
    mask=1<<(event->btnid-0x80);
    if (value) device->state|=mask;
    else device->state&=~mask;
  } else switch (event->btnid) {
    #define ANALOG(lobit,hibit) { \
      device->state&=~(lobit|hibit); \
      if (event->value<=-32) device->state|=lobit; \
      else if (event->value>=32) device->state|=hibit; \
    }
    case EGG_JOYBTN_LX: ANALOG(STATE_LXLO,STATE_LXHI) break;
    case EGG_JOYBTN_LY: ANALOG(STATE_LYLO,STATE_LYHI) break;
    case EGG_JOYBTN_RX: ANALOG(STATE_RXLO,STATE_RXHI) break;
    case EGG_JOYBTN_RY: ANALOG(STATE_RYLO,STATE_RYHI) break;
    #undef ANALOG
  }
}

static void _joystick_event(struct menu *menu,const union egg_event *event) {
  switch (event->type) {
    case EGG_EVENT_JOY: joystick_on_joy(menu,&event->joy); break;
  }
}

static void _joystick_update(struct menu *menu,double elapsed) {
}

/* Render.
 */
 
static void joystick_render_device(struct menu *menu,int x0,int y0,const struct device *device) {
  if (!menu->tile_renderer) return;
  tile_renderer_begin(menu->tile_renderer,menu->tilesheet,0x808080ff,0xff);
  x0=x0+(MENU->colw>>1)-((MENU->tilesize*7)>>1);
  y0=y0+(MENU->rowh>>1)-((MENU->tilesize*4)>>1);
  
  // Outline.
  int row=0,y=y0+(MENU->tilesize>>1); for (;row<4;row++,y+=MENU->tilesize) {
    int col=0,x=x0+(MENU->tilesize>>1); for (;col<7;col++,x+=MENU->tilesize) {
      uint8_t tileid,xform;
      if (col<4) {
        tileid=0x80+col;
        xform=0;
      } else {
        tileid=0x80-col+6;
        xform=EGG_XFORM_XREV;
      }
      tileid+=row<<4;
      tile_renderer_tile(menu->tile_renderer,x,y,tileid,xform);
    }
  }
  
  // Button outlines.
  #define SETVTX(p,xx,yy,ti,xf) tile_renderer_tile(menu->tile_renderer,x0+(xx),y0+(yy),ti,xf);
  const int colw=MENU->tilesize;
  SETVTX(0,2+colw*5,colw*3-3,0x86,0) // South
  SETVTX(1,2+colw*6-3,colw*2,0x86,0) // East
  SETVTX(2,2+colw*4+3,colw*2,0x86,0) // West
  SETVTX(3,2+colw*5,colw+3,0x86,0) // North
  SETVTX(4,colw+(colw>>1),colw>>1,0x94,0) // L1 (left)
  SETVTX(5,colw*2+(colw>>1),colw>>1,0x95,0) // L1 (right)
  SETVTX(6,colw*4+(colw>>1),colw>>1,0x95,EGG_XFORM_XREV) // R1 (left)
  SETVTX(7,colw*5+(colw>>1),colw>>1,0x94,EGG_XFORM_XREV) // R1 (right)
  SETVTX(8,colw,colw>>1,0x98,0) // L2
  SETVTX(9,colw*6,colw>>1,0x98,EGG_XFORM_XREV) // R2
  SETVTX(10,colw*3+(colw>>1),colw*2+(colw>>1),0x8a,0) // Aux2 (upper)
  SETVTX(11,colw*3+(colw>>1),colw*2+(colw>>1),0x8a,EGG_XFORM_XREV) // Aux1 (lower)
  SETVTX(12,colw*2+(colw>>1),colw*3+(colw>>1),0x86,0) // LP
  SETVTX(13,colw*4+(colw>>1),colw*3+(colw>>1),0x86,0) // RP
  SETVTX(14,colw*2-2,colw+(colw>>1),0x84,EGG_XFORM_SWAP) // Up
  SETVTX(15,colw*2-2,colw*2+(colw>>1),0x84,EGG_XFORM_XREV|EGG_XFORM_SWAP) // Down
  SETVTX(16,colw+(colw>>1)-2,colw*2,0x84,0) // Left
  SETVTX(17,colw*2+(colw>>1)-2,colw*2,0x84,EGG_XFORM_XREV) // Right
  SETVTX(18,colw*3+(colw>>1),colw+(colw>>1),0x86,0) // Aux3
  
  tile_renderer_end(menu->tile_renderer);
  
  // Bright state for anything "on".
  if (device->state) {
    tile_renderer_begin(menu->tile_renderer,menu->tilesheet,0xffff00ff,0xff);
    int mask=1; for (;mask<0x10000000;mask<<=1) {
      if (!(device->state&mask)) continue;
      switch (mask) {
        case STATE_SOUTH: SETVTX(0,2+colw*5,colw*3-3,0x87,0) break;
        case STATE_EAST: SETVTX(1,2+colw*6-3,colw*2,0x87,0) break;
        case STATE_WEST: SETVTX(2,2+colw*4+3,colw*2,0x87,0) break;
        case STATE_NORTH: SETVTX(3,2+colw*5,colw+3,0x87,0) break;
        case STATE_L1: SETVTX(4,colw+(colw>>1),colw>>1,0x96,0) SETVTX(5,colw*2+(colw>>1),colw>>1,0x97,0) break;
        case STATE_R1: SETVTX(6,colw*4+(colw>>1),colw>>1,0x97,EGG_XFORM_XREV) SETVTX(7,colw*5+(colw>>1),colw>>1,0x96,EGG_XFORM_XREV) break;
        case STATE_L2: SETVTX(8,colw,colw>>1,0x99,0) break;
        case STATE_R2: SETVTX(9,colw*6,colw>>1,0x99,EGG_XFORM_XREV) break;
        case STATE_AUX2: SETVTX(10,colw*3+(colw>>1),colw*2+(colw>>1),0x8b,0) break;
        case STATE_AUX1: SETVTX(11,colw*3+(colw>>1),colw*2+(colw>>1),0x8b,EGG_XFORM_XREV) break;
        case STATE_LP: SETVTX(12,colw*2+(colw>>1),colw*3+(colw>>1),0x89,0) break;
        case STATE_RP: SETVTX(13,colw*4+(colw>>1),colw*3+(colw>>1),0x89,0) break;
        case STATE_UP: SETVTX(14,colw*2-2,colw+(colw>>1),0x85,EGG_XFORM_SWAP) break;
        case STATE_DOWN: SETVTX(15,colw*2-2,colw*2+(colw>>1),0x85,EGG_XFORM_XREV|EGG_XFORM_SWAP) break;
        case STATE_LEFT: SETVTX(16,colw+(colw>>1)-2,colw*2,0x85,0) break;
        case STATE_RIGHT: SETVTX(17,colw*2+(colw>>1)-2,colw*2,0x85,EGG_XFORM_XREV) break;
        case STATE_AUX3: SETVTX(18,colw*3+(colw>>1),colw+(colw>>1),0x87,0) break;
        case STATE_LXLO: SETVTX(12,colw*2+(colw>>1),colw*3+(colw>>1),0x88,0) break;
        case STATE_LXHI: SETVTX(12,colw*2+(colw>>1),colw*3+(colw>>1),0x88,EGG_XFORM_XREV) break;
        case STATE_LYLO: SETVTX(12,colw*2+(colw>>1),colw*3+(colw>>1),0x88,EGG_XFORM_SWAP) break;
        case STATE_LYHI: SETVTX(12,colw*2+(colw>>1),colw*3+(colw>>1),0x88,EGG_XFORM_SWAP|EGG_XFORM_XREV) break;
        case STATE_RXLO: SETVTX(13,colw*4+(colw>>1),colw*3+(colw>>1),0x88,0) break;
        case STATE_RXHI: SETVTX(13,colw*4+(colw>>1),colw*3+(colw>>1),0x88,EGG_XFORM_XREV) break;
        case STATE_RYLO: SETVTX(13,colw*4+(colw>>1),colw*3+(colw>>1),0x88,EGG_XFORM_SWAP) break;
        case STATE_RYHI: SETVTX(13,colw*4+(colw>>1),colw*3+(colw>>1),0x88,EGG_XFORM_SWAP|EGG_XFORM_XREV) break;
      }
    }
    tile_renderer_end(menu->tile_renderer);
  }
  #undef SETVTX
}

static void _joystick_render(struct menu *menu) {
  int ymajor=MENU->ymargin,devicep=0,row=0;
  for (;row<MENU->rowc;row++,ymajor+=MENU->rowh) {
    int xmajor=MENU->xmargin,col=0;
    for (;col<MENU->colc;col++,xmajor+=MENU->colw,devicep++) {
      if (devicep>=MENU->devicec) break;
      joystick_render_device(menu,xmajor,ymajor,MENU->devicev+devicep);
    }
    if (devicep>=MENU->devicec) break;
  }
}

/* New.
 */
 
struct menu *menu_new_joystick(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu_joystick),parent);
  if (!menu) return 0;
  menu->del=_joystick_del;
  menu->event=_joystick_event;
  menu->update=_joystick_update;
  menu->render=_joystick_render;
  
  MENU->tilesize=8;
  // Each joystick uses 7x4 tiles. Measuring against 8x5 to build a little padding into each.
  MENU->colc=menu->screenw/(MENU->tilesize*8);
  MENU->rowc=menu->screenh/(MENU->tilesize*5);
  if (MENU->colc<1) MENU->colc=1;
  if (MENU->rowc<1) MENU->rowc=1;
  if ((MENU->colw=menu->screenw/MENU->colc)<1) MENU->colw=1;
  if ((MENU->rowh=menu->screenh/MENU->rowc)<1) MENU->rowh=1;
  MENU->xmargin=(menu->screenw-MENU->colw*MENU->colc)>>1;
  MENU->ymargin=(menu->screenh-MENU->rowh*MENU->rowc)>>1;
  
  int p=0; for (;;p++) {
    int devid=egg_joystick_devid_by_index(p);
    if (devid<1) break;
    struct device *device=MENU->devicev+MENU->devicec++;
    device->devid=devid;
    if (MENU->devicec>=DEVICE_LIMIT) break;
  }
  
  return menu;
}
