#include <egg/egg.h>
#include "menu.h"
#include "util/font.h"
#include "util/tile_renderer.h"
#include "stdlib/egg-stdlib.h"

#define VOICEC 10
#define SONGID_LIMIT 20
#define SOUNDID_LIMIT 20

static const uint8_t noteidv[VOICEC]={
  0x38,0x3a,0x3b,0x3d,0x3f,0x40,0x42,0x44,0x4b,0x50,
};

struct menu_audio {
  struct menu hdr;
  int disclaimer;
  int voicev[VOICEC];
  int songidv[SONGID_LIMIT];
  int soundidv[SOUNDID_LIMIT];
  int songidc,soundidc;
};

#define MENU ((struct menu_audio*)menu)

static void _audio_del(struct menu *menu) {
  if (MENU->disclaimer) egg_texture_del(MENU->disclaimer);
}

static void audio_play_note(struct menu *menu,int p,int v) {
  if ((p<0)||(p>=VOICEC)) return;
  if (v) {
    if (MENU->voicev[p]) return;
    MENU->voicev[p]=1;
  } else {
    if (!MENU->voicev[p]) return;
    MENU->voicev[p]=0;
  }
  egg_audio_event(0xf,v?0x90:0x80,noteidv[p],0x40);
}

static void _audio_adjust(struct menu *menu,int d) {
  switch (menu->optionp) {
    case 0: { // songid
        if (MENU->songidc<1) return;
        int songid=menu_option_read_tail_int(menu,0);
        int p=-1,i=MENU->songidc;
        while (i-->0) {
          if (MENU->songidv[i]==songid) {
            p=i;
            break;
          }
        }
        p+=d;
        if (p<0) p=MENU->songidc-1;
        else if (p>=MENU->songidc) p=0;
        menu_option_rewrite_tail_int(menu,0,MENU->songidv[p]);
      } break;
    case 1: { // force
        char tmp[8];
        int tmpc=menu_option_read_tail_string(tmp,sizeof(tmp),menu,1);
        if ((tmpc==4)&&!memcmp(tmp,"true",4)) menu_option_rewrite_tail_string(menu,1,"false",5);
        else menu_option_rewrite_tail_string(menu,1,"true",4);
      } break;
    case 2: { // repeat
        char tmp[8];
        int tmpc=menu_option_read_tail_string(tmp,sizeof(tmp),menu,2);
        if ((tmpc==4)&&!memcmp(tmp,"true",4)) menu_option_rewrite_tail_string(menu,2,"false",5);
        else menu_option_rewrite_tail_string(menu,2,"true",4);
      } break;
    case 3: { // soundid
        if (MENU->soundidc<1) return;
        int soundid=menu_option_read_tail_int(menu,3);
        int p=-1,i=MENU->soundidc;
        while (i-->0) {
          if (MENU->soundidv[i]==soundid) {
            p=i;
            break;
          }
        }
        p+=d;
        if (p<0) p=MENU->soundidc-1;
        else if (p>=MENU->soundidc) p=0;
        menu_option_rewrite_tail_int(menu,3,MENU->soundidv[p]);
      } break;
    case 4: { // trim
        int v=menu_option_read_tail_int(menu,4);
        v+=d*10;
        if (v<0) v=0;
        else if (v>99) v=99;
        menu_option_rewrite_tail_int(menu,4,v);
      } break;
    case 5: { // pan
        int v=menu_option_read_tail_int(menu,5);
        v+=d*20;
        if (v<-99) v=-99;
        else if (v>99) v=99;
        menu_option_rewrite_tail_int(menu,5,v);
      } break;
    case 6: { // playhead
        double v=menu_option_read_tail_double(menu,6);
        v+=d*1.0;
        if (v<0.0) v=0.0;
        menu_option_rewrite_tail_double(menu,6,v);
      } break;
  }
}

static void _audio_option(struct menu *menu) {
  switch (menu->optionp) {
    case 0: { // Play song
        int songid=menu_option_read_tail_int(menu,0);
        int force=menu_option_read_tail_int(menu,1);
        int repeat=menu_option_read_tail_int(menu,2);
        egg_audio_play_song(0,songid,force,repeat);
      } break;
    case 1: _audio_adjust(menu,1); break;
    case 2: _audio_adjust(menu,1); break;
    case 3: { // Play sound
        int soundid=menu_option_read_tail_int(menu,3);
        int trim99=menu_option_read_tail_int(menu,4);
        int pan99=menu_option_read_tail_int(menu,5);
        if (soundid>0) egg_audio_play_sound(0,soundid,(trim99*0x10000)/100,(pan99*0x8000)/100);
      } break;
    case 4: { // Trim
        int trim99=menu_option_read_tail_int(menu,4);
        if (trim99>=99) trim99=25;
        else trim99=99;
        menu_option_rewrite_tail_int(menu,4,trim99);
      } break;
    case 5: { // Pan
        int pan99=menu_option_read_tail_int(menu,5);
        if (pan99>=99) pan99=-99;
        else if (pan99<=-99) pan99=0;
        else if (!pan99) pan99=99;
        else pan99=0;
        menu_option_rewrite_tail_int(menu,5,pan99);
      } break;
    case 6: { // Set playhead
        double beat=menu_option_read_tail_double(menu,6);
        egg_audio_set_playhead(beat);
      } break;
  }
}

static void _audio_event(struct menu *menu,const union egg_event *event) {
  switch (event->type) {
    case EGG_EVENT_JOY: switch (event->joy.btnid) {
        case EGG_JOYBTN_NORTH: audio_play_note(menu,0,event->joy.value); break;
        case EGG_JOYBTN_EAST: audio_play_note(menu,1,event->joy.value); break;
        case EGG_JOYBTN_L1: audio_play_note(menu,2,event->joy.value); break;
        case EGG_JOYBTN_R1: audio_play_note(menu,3,event->joy.value); break;
        case EGG_JOYBTN_L2: audio_play_note(menu,4,event->joy.value); break;
        case EGG_JOYBTN_R2: audio_play_note(menu,5,event->joy.value); break;
      } break;
    case EGG_EVENT_KEY: switch (event->key.keycode) {
        case 0x00070014: audio_play_note(menu,0,event->key.value); break;
        case 0x0007001a: audio_play_note(menu,1,event->key.value); break;
        case 0x00070008: audio_play_note(menu,2,event->key.value); break;
        case 0x00070015: audio_play_note(menu,3,event->key.value); break;
        case 0x00070017: audio_play_note(menu,4,event->key.value); break;
        case 0x0007001c: audio_play_note(menu,5,event->key.value); break;
        case 0x00070018: audio_play_note(menu,6,event->key.value); break;
        case 0x0007000c: audio_play_note(menu,7,event->key.value); break;
        case 0x00070012: audio_play_note(menu,8,event->key.value); break;
        case 0x00070013: audio_play_note(menu,9,event->key.value); break;
      } break;
  }
  menu_option_event(menu,event);
}

static void _audio_render(struct menu *menu) {
  menu_option_render(menu);
  if (!MENU->disclaimer) {
    if (menu->font) {
      MENU->disclaimer=font_render_new_texture(menu->font,"Joy or Q..P to play notes.",-1,0,0x808080);
    }
  } else {
    int w=0,h=0;
    egg_texture_get_header(&w,&h,0,MENU->disclaimer);
    egg_draw_decal(1,MENU->disclaimer,4,menu->screenh-4-h,0,0,w,h,0);
  }
  if (menu->tile_renderer&&menu->tilesheet) {
    double playhead=egg_audio_get_playhead();
    double whole,fract;
    fract=modf(playhead,&whole);
    uint8_t alpha=(fract<=0.0)?0xff:(fract>=1.0)?0:((1.0-fract)*0xff);
    int beat=(int)whole;
    if (beat<0) beat=0;
    char tmp[4]={
      '0'+(beat/1000)%10,
      '0'+(beat/ 100)%10,
      '0'+(beat/  10)%10,
      '0'+(beat     )%10,
    };
    tile_renderer_begin(menu->tile_renderer,menu->tilesheet,0xffffffff,alpha);
    tile_renderer_string(menu->tile_renderer,menu->screenw-40,menu->screenh-8,tmp,4);
    tile_renderer_end(menu->tile_renderer);
  }
}

static int _audio_cb_res(int tid,int qual,int rid,int len,void *userdata) {
  struct menu *menu=userdata;
  if (tid==EGG_RESTYPE_song) {
    if (!qual) {
      if (MENU->songidc<SONGID_LIMIT) {
        MENU->songidv[MENU->songidc++]=rid;
      }
    }
  } else if (tid==EGG_RESTYPE_sound) {
    if (!qual) {
      if (MENU->soundidc<SOUNDID_LIMIT) {
        MENU->soundidv[MENU->soundidc++]=rid;
      }
    }
  } else if ((tid>EGG_RESTYPE_song)&&(tid>EGG_RESTYPE_sound)) return 1;
  return 0;
}
 
struct menu *menu_new_audio(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu_audio),parent);
  if (!menu) return 0;
  menu->del=_audio_del;
  menu->event=_audio_event;
  menu->update=menu_option_update;
  menu->render=_audio_render;
  menu->on_option=_audio_option;
  menu->on_option_adjust=_audio_adjust;
  
  MENU->songidv[MENU->songidc++]=0; // Keep zero in the song list, "no song" is a valid thing to ask for.
  egg_res_for_each(_audio_cb_res,menu);
  
  if (
    (menu_option_add(menu,"Play Song 0",-1)<0)||
    (menu_option_add(menu,"  Force: false",-1)<0)||
    (menu_option_add(menu,"  Repeat: true",-1)<0)||
    (menu_option_add(menu,"Play Sound 0",-1)<0)||
    (menu_option_add(menu,"  Trim: 99",-1)<0)||
    (menu_option_add(menu,"  Pan: 0",-1)<0)||
    (menu_option_add(menu,"Force Playhead to 1.0",-1)<0)||
  0) {
    menu_del(menu);
    return 0;
  }
  
  menu_option_rewrite_tail_int(menu,0,MENU->songidv[0]);
  menu_option_rewrite_tail_int(menu,3,MENU->soundidv[0]);
  
  egg_audio_event(0xf,0xc0,0x10,0x00);
  egg_audio_event(0xf,0xb0,0x07,0x7f);
  
  return menu;
}
