#include <egg/egg.h>
#include "menu.h"
#include "util/tile_renderer.h"
#include "stdlib/egg-stdlib.h"

extern double total_game_time;
extern double start_real_time;

struct menu_misc {
  struct menu hdr;
  char langs[32];
};

#define MENU ((struct menu_misc*)menu)

static int misc_repr_time(char *dst,int dsta,const char *label,double time) {
  int ms=(int)(time*1000.0);
  int sec=ms/1000; ms%=1000;
  int min=sec/60; sec%=60;
  int hour=min/60; min%=60;
  if (hour>99) { hour=min=sec=99; ms=999; }
  int dstc=0;
  for (;*label;label++) {
    if (dstc<dsta) dst[dstc++]=*label;
  }
  if (dstc<dsta) dst[dstc++]=':';
  if (dstc<dsta) dst[dstc++]=' ';
  if (dstc<dsta) dst[dstc++]='0'+hour/10;
  if (dstc<dsta) dst[dstc++]='0'+hour%10;
  if (dstc<dsta) dst[dstc++]=':';
  if (dstc<dsta) dst[dstc++]='0'+min/10;
  if (dstc<dsta) dst[dstc++]='0'+min%10;
  if (dstc<dsta) dst[dstc++]=':';
  if (dstc<dsta) dst[dstc++]='0'+sec/10;
  if (dstc<dsta) dst[dstc++]='0'+sec%10;
  if (dstc<dsta) dst[dstc++]='.';
  if (dstc<dsta) dst[dstc++]='0'+ms/100;
  if (dstc<dsta) dst[dstc++]='0'+(ms/10)%10;
  if (dstc<dsta) dst[dstc++]='0'+ms%10;
  return dstc;
}

static void _misc_render(struct menu *menu) {
  char tmp[256];
  int tmpc,y=16;
  tile_renderer_begin(menu->tile_renderer,menu->tilesheet,0xffffffff,0xff);
  
  if (((tmpc=misc_repr_time(tmp,sizeof(tmp),"Real time",egg_time_real()-start_real_time))>0)&&(tmpc<=sizeof(tmp))) {
    tile_renderer_string(menu->tile_renderer,8,y,tmp,tmpc);
    y+=8;
  }
  if (((tmpc=misc_repr_time(tmp,sizeof(tmp),"Game time",egg_time_real()-start_real_time))>0)&&(tmpc<=sizeof(tmp))) {
    tile_renderer_string(menu->tile_renderer,8,y,tmp,tmpc);
    y+=8;
  }
  
  int date[7]={0};
  egg_time_local(date,7);
  char datestr[]={
    '0'+(date[0]/1000)%10,
    '0'+(date[0]/ 100)%10,
    '0'+(date[0]/  10)%10,
    '0'+(date[0]     )%10,
    '-',
    '0'+(date[1]/  10)%10,
    '0'+(date[1]     )%10,
    '-',
    '0'+(date[2]/  10)%10,
    '0'+(date[2]     )%10,
    'T',
    '0'+(date[3]/  10)%10,
    '0'+(date[3]     )%10,
    ':',
    '0'+(date[4]/  10)%10,
    '0'+(date[4]     )%10,
    ':',
    '0'+(date[5]/  10)%10,
    '0'+(date[5]     )%10,
    '.',
    '0'+(date[6]/ 100)%10,
    '0'+(date[6]/  10)%10,
    '0'+(date[6]     )%10,
  };
  tile_renderer_string(menu->tile_renderer,8,y,datestr,sizeof(datestr));
  y+=8;
  
  tile_renderer_string(menu->tile_renderer,8,y,"Langs:",6);
  tile_renderer_string(menu->tile_renderer,8+7*8,y,MENU->langs,-1);
  y+=8;
  
  tile_renderer_end(menu->tile_renderer);
}
 
struct menu *menu_new_misc(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu_misc),parent);
  if (!menu) return 0;
  menu->render=_misc_render;
  
  int langv[16];
  int langc=egg_get_user_languages(langv,16);
  if (langc<=0) {
    memcpy(MENU->langs,"None!",6);
  } else if (langc>16) {
    memcpy(MENU->langs,"Too many!",10);
  } else {
    int dstc=0;
    int i=0; for (;i<langc;i++) {
      if (dstc) MENU->langs[dstc++]=',';
      MENU->langs[dstc++]="012345abcdefghijklmnopqrstuwxyz"[(langv[i]>>5)&0x1f];
      MENU->langs[dstc++]="012345abcdefghijklmnopqrstuwxyz"[langv[i]&0x1f];
      if (dstc>=sizeof(MENU->langs)-3) break;
    }
    MENU->langs[dstc]=0;
  }
  
  return menu;
}
