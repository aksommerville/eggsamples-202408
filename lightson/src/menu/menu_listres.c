#include <egg/egg.h>
#include "menu.h"
#include "util/tile_renderer.h"
#include "util/font.h"
#include "stdlib/egg-stdlib.h"

/* Object definition.
 */
 
struct menu_listres {
  struct menu hdr;
  struct res { int tid,qual,rid,len; } *resv;
  int resc,resa;
  int resp;
  int rowc;
  struct egg_draw_tile *tilev;
  int tilec,tilea;
};

#define MENU ((struct menu_listres*)menu)

/* Cleanup.
 */
 
static void _listres_del(struct menu *menu) {
  if (MENU->resv) free(MENU->resv);
  if (MENU->tilev) free(MENU->tilev);
}

/* Add one tile.
 */
 
static void listres_add_tile(struct menu *menu,int x,int y,uint8_t tileid) {
  if (MENU->tilec>=MENU->tilea) {
    int na=MENU->tilea+128;
    if (na>INT_MAX/sizeof(struct egg_draw_tile)) return;
    void *nv=realloc(MENU->tilev,sizeof(struct egg_draw_tile)*na);
    if (!nv) return;
    MENU->tilev=nv;
    MENU->tilea=na;
  }
  struct egg_draw_tile *tile=MENU->tilev+MENU->tilec++;
  tile->x=x;
  tile->y=y;
  tile->tileid=tileid;
  tile->xform=0;
}

/* Represent scalar components.
 * All of these return 0..dsta, no errors or excess.
 */
 
static int decuint_repr(char *dst,int dsta,unsigned int src) {
  int dstc=1;
  unsigned int limit=10;
  while (src>=limit) { dstc++; if (limit>0xffffffffu/10) break; limit*=10; }
  if (dstc>dsta) dstc=dsta;
  int i=dstc; for (;i-->0;src/=10) dst[i]='0'+src%10;
  return dstc;
}

static int tid_repr(char *dst,int dsta,int tid) {
  const char *src=0;
  switch (tid) {
    #define _(tag) case EGG_RESTYPE_##tag: src=#tag; break;
    EGG_RESTYPE_FOR_EACH
    #undef _
  }
  if (!src) return decuint_repr(dst,dsta,tid);
  int dstc=0;
  for (;*src;src++) {
    if (dstc>=dsta) return dstc;
    dst[dstc++]=*src;
  }
  return dstc;
}

static int qual_repr(char *dst,int dsta,int src) {
  if (src>0x3ff) return decuint_repr(dst,dsta,src);
  if (dsta<2) return 0;
  if (!src) {
    dst[0]=dst[1]='-';
  } else {
    dst[0]="012345abcdefghijklmnopqrstuvwxyz"[src>>5];
    dst[1]="012345abcdefghijklmnopqrstuvwxyz"[src&0x1f];
  }
  return 2;
}

static int size_repr(char *dst,int dsta,int src) {
  if (dsta<6) return 0;
  if (src<0) src=0;
  char unit=' ';
       if (src>1<<30) { unit='G'; src>>=30; }
  else if (src>1<<20) { unit='M'; src>>=20; }
  else if (src>1<<10) { unit='k'; src>>=10; }
  if (src>999) src=999;
  if (src>=100) dst[0]='0'+(src/100)%10; else dst[0]=' ';
  if (src>=10) dst[1]='0'+(src/10)%10; else dst[1]=' ';
  dst[2]='0'+src%10;
  dst[3]=' ';
  dst[4]=unit;
  dst[5]='B';
  return 6;
}

/* Rebuild tiles for current view.
 */
 
static void listres_add_tiles_for_res(struct menu *menu,const struct res *res,int y) {
  int x=12;
  char tmp[32];
  int tmpc=tid_repr(tmp,sizeof(tmp),res->tid);
  int i=0;
  for (;i<tmpc;i++,x+=8) listres_add_tile(menu,x,y,tmp[i]);
  listres_add_tile(menu,x,y,':'); x+=8;
  tmpc=qual_repr(tmp,sizeof(tmp),res->qual);
  for (i=0;i<tmpc;i++,x+=8) listres_add_tile(menu,x,y,tmp[i]);
  listres_add_tile(menu,x,y,':'); x+=8;
  tmpc=decuint_repr(tmp,sizeof(tmp),res->rid);
  for (i=0;i<tmpc;i++,x+=8) listres_add_tile(menu,x,y,tmp[i]);
  if (x>=122) x+=8; else x=130;
  tmpc=size_repr(tmp,sizeof(tmp),res->len);
  for (i=0;i<tmpc;i++,x+=8) listres_add_tile(menu,x,y,tmp[i]);
}
 
static void listres_rebuild_tiles(struct menu *menu) {
  MENU->tilec=0;
  const struct res *res=MENU->resv+MENU->resp;
  int i=MENU->resc-MENU->resp;
  if (i>MENU->rowc) i=MENU->rowc;
  int y=6;
  for (;i-->0;res++,y+=8) {
    listres_add_tiles_for_res(menu,res,y);
  }
}

/* Motion.
 */
 
static void listres_motion1(struct menu *menu,int d) {
  int np=MENU->resp+d;
  if (np>MENU->resc-MENU->rowc) np=MENU->resc-MENU->rowc;
  if (np<0) np=0;
  if (np==MENU->resp) return;
  MENU->resp=np;
  listres_rebuild_tiles(menu);
}
 
static void listres_motion(struct menu *menu,int dx,int dy) {
  listres_motion1(menu,dx*MENU->rowc+dy);
}

/* Event.
 */
 
static void _listres_event(struct menu *menu,const union egg_event *event) {
  switch (event->type) {
    case EGG_EVENT_JOY: if (event->joy.value) switch (event->joy.btnid) {
        case EGG_JOYBTN_UP: listres_motion(menu,0,-1); break;
        case EGG_JOYBTN_DOWN: listres_motion(menu,0,1); break;
        case EGG_JOYBTN_LEFT: listres_motion(menu,-1,0); break;
        case EGG_JOYBTN_RIGHT: listres_motion(menu,1,0); break;
        case EGG_JOYBTN_L1: case EGG_JOYBTN_L2: listres_motion(menu,-1,0); break;
        case EGG_JOYBTN_R1: case EGG_JOYBTN_R2: listres_motion(menu,1,0); break;
      } break;
    case EGG_EVENT_KEY: if (event->key.value) switch (event->key.keycode) {
        case 0x0007004f: listres_motion(menu,1,0); break;
        case 0x00070050: listres_motion(menu,-1,0); break;
        case 0x00070051: listres_motion(menu,0,1); break;
        case 0x00070052: listres_motion(menu,0,-1); break;
      } break;
  }
}

/* Update.
 */
 
static void _listres_update(struct menu *menu,double elapsed) {
}

/* Render.
 */
 
static void _listres_render(struct menu *menu) {
  egg_render_tint(0xffffffff);
  egg_draw_tile(1,menu->tilesheet,MENU->tilev,MENU->tilec);
  egg_render_tint(0);
}

/* Add resource.
 */
 
static int listres_add_res(int tid,int qual,int rid,int len,void *userdata) {
  struct menu *menu=userdata;
  if (MENU->resc>=MENU->resa) {
    int na=MENU->resa+256;
    if (na>INT_MAX/sizeof(struct res)) return -1;
    void *nv=realloc(MENU->resv,sizeof(struct res)*na);
    if (!nv) return -1;
    MENU->resv=nv;
    MENU->resa=na;
  }
  struct res *res=MENU->resv+MENU->resc++;
  res->tid=tid;
  res->qual=qual;
  res->rid=rid;
  res->len=len;
  return 0;
}

/* New.
 */
 
struct menu *menu_new_listres(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu_listres),parent);
  if (!menu) return 0;
  menu->del=_listres_del;
  menu->event=_listres_event;
  menu->update=_listres_update;
  menu->render=_listres_render;
  MENU->rowc=menu->screenh/8;
  egg_res_for_each(listres_add_res,menu);
  listres_rebuild_tiles(menu);
  return menu;
}
