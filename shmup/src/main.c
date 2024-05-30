#include "shmup.h"

#define HERO_SPEED 100.0 /* px/s */
#define SHOT_SPEED 250.0 /* px/s */
#define SHOT_LIMIT 16
#define MONSTER_LIMIT 24
#define MONSTER_ANIM_TIME 0.250
#define MONSTER_MOVE_TIME 0.100
#define FIREWORK_LIMIT 24
#define SCORE_ATROPHY_TIME 0.100

int texid_sprites;
static int texid_tiles;
static int herodx=0;
static double herox=64.0;
static double monster_anim_clock=0.0;
static double monster_move_clock=0.0;
static int16_t monsterdx=1;
static int score=0;
static double score_atrophy_clock=0.0;
double alarm_clock=0.0;
static int pause=1;
static int perfect=1;
static int wave=0;
static double show_perfect=0.0;
static double show_wave=0.0;
static int hiscore=0;
static int new_hiscore=0;

static struct shot {
  int16_t x;
  double y; // lower edge; <=0 means defunct
} shotv[SHOT_LIMIT]={0};

// Really pushing it, and using the vertex type as our sprites.
// I thiiiink we can get away with this? Since they only move in formation.
static struct egg_draw_tile monsterv[MONSTER_LIMIT]={0};
static int monsterc=0;

static struct firework {
  int x,y;
  double ttl;
} fireworkv[FIREWORK_LIMIT];
static int fireworkc=0; // but may be sparse

void egg_client_quit() {
}

int egg_client_init() {
  egg_texture_load_image(texid_sprites=egg_texture_new(),0,1);
  
  // texid_tiles is a 16x16 tilesheet, but we don't want to store that whole thing in the ROM.
  // (since we're only using just a couple tiles).
  texid_tiles=egg_texture_new();
  egg_texture_upload(texid_tiles,128,128,128<<2,EGG_TEX_FMT_RGBA,0,0);
  egg_draw_decal(texid_tiles,texid_sprites,0,0,19,0,48,8,0);
  
  // Load high score. It's not allowed to exceed 4 digits.
  char tmp[4];
  int tmpc=egg_store_get(tmp,sizeof(tmp),"hiscore",7);
  if ((tmpc>0)&&(tmpc<=4)) {
    int i=0; for (;i<tmpc;i++) {
      int digit=tmp[i]-'0';
      if ((digit<0)||(digit>9)) { hiscore=0; break; }
      hiscore*=10;
      hiscore+=digit;
    }
  }
  
  egg_audio_play_song(0,1,0,1);
  egg_event_enable(EGG_EVENT_MMOTION,1);
  egg_event_enable(EGG_EVENT_MBUTTON,1);
  egg_lock_cursor(1);
  srand_auto();
  stars_init();
  return 0;
}

static void generate_monsters() { // ...or end game

  if (wave==3) {
    pause=1;
    fireworkc=0;
    struct shot *shot=shotv;
    int i=SHOT_LIMIT;
    for (;i-->0;shot++) shot->y=0.0;
    if (score>hiscore) {
      if ((hiscore=score)>9999) hiscore=9999;
      new_hiscore=1;
      char tmp[4]={
        '0'+hiscore/1000,
        '0'+(hiscore/100)%10,
        '0'+(hiscore/10)%10,
        '0'+hiscore%10,
      };
      egg_store_set("hiscore",7,tmp,4);
    }
    return;
  }
  if (wave&&perfect) {
    score+=1000;
    show_perfect=3.000;
  }
  wave++;
  perfect=1;
  show_wave=3.000;
  monsterdx=1;

  monsterc=0;
  struct egg_draw_tile *monster=monsterv;
  int16_t x=8,y=8;
  for (;monsterc<MONSTER_LIMIT;monsterc++,monster++) {
    monster->x=x;
    monster->y=y;
    monster->tileid=0x00;
    monster->xform=0;
    x+=16;
    if (x>100) {
      x=8;
      y+=16;
    }
  }
}

static void move_monsters() {
  if (monsterc<1) return;
  int16_t xlo=monsterv[0].x,xhi=monsterv[0].y;
  struct egg_draw_tile *monster=monsterv;
  int i=monsterc;
  for (;i-->0;monster++) {
    if (monster->x<xlo) xlo=monster->x;
    else if (monster->x>xhi) xhi=monster->x;
  }
  if (monsterdx>0) {
    if (xhi>=120) {
      monsterdx=-1;
      perfect=0;
    } else {
      for (monster=monsterv,i=monsterc;i-->0;monster++) monster->x++;
    }
  } else {
    if (xlo<8) {
      monsterdx=1;
      perfect=0;
    } else {
      for (monster=monsterv,i=monsterc;i-->0;monster++) monster->x--;
    }
  }
}

static void animate_monsters() {
  struct egg_draw_tile *monster=monsterv;
  int i=monsterc;
  for (;i-->0;monster++) {
    monster->tileid^=1;
  }
}

static void fire_laser() { // ...or start game, if we're paused

  if (pause) {
    herox=64.0;
    pause=0;
    score=0;
    perfect=1;
    wave=0;
    new_hiscore=0;
    return;
  }

  struct shot *shot=0;
  struct shot *q=shotv;
  int i=SHOT_LIMIT;
  for (;i-->0;q++) {
    if (q->y<=0.0) {
      shot=q;
      break;
    }
  }
  if (!shot) return;
  egg_audio_play_sound(0,1,0x00010000,0);
  shot->x=(int16_t)herox;
  shot->y=115.0;
}

static void add_firework(int x,int y) {
  if (fireworkc>=FIREWORK_LIMIT) return;
  struct firework *firework=fireworkv+fireworkc++;
  firework->x=x;
  firework->y=y;
  firework->ttl=1.0;
}

// Shots are sparse and monsters are packed.
// If there's a collision, kill them both.
static void check_monster_shot(struct shot *shot) {
  int i=monsterc;
  struct egg_draw_tile *monster=monsterv+i-1;
  for (;i-->0;monster--) {
    int16_t dx=shot->x-monster->x;
    if (dx<-4) continue;
    if (dx>4) continue;
    int16_t shoty=(int16_t)shot->y-5;
    int16_t dy=shoty-monster->y;
    if (dy<-4) continue;
    if (dy>4) continue;
    add_firework(monster->x,monster->y);
    monsterc--;
    int j=i; for (;j<monsterc;j++) monsterv[j]=monsterv[j+1];
    shot->y=0.0;
    score+=100;
    egg_audio_play_sound(0,2,0x00010000,0);
    return;
  }
}

void egg_client_update(double elapsed) {
  union egg_event eventv[16];
  int eventc;
  while ((eventc=egg_event_get(eventv,16))>0) {
    const union egg_event *event=eventv;
    int i=eventc; for (;i-->0;event++) switch (event->type) {
      case EGG_EVENT_KEY: switch (event->key.keycode) {
          case 0x00070029: if (event->key.value) egg_request_termination(); break; // Esc
          case 0x0007002c: if (event->key.value==1) fire_laser(); break; // Space
          case 0x0007004f: if (event->key.value) herodx=1; else if (herodx>0) herodx=0; break; // Right
          case 0x00070050: if (event->key.value) herodx=-1; else if (herodx<0) herodx=0; break; // Left
        } break;
      case EGG_EVENT_JOY: switch (event->joy.btnid) {
          case EGG_JOYBTN_SOUTH: if (event->joy.value) fire_laser(); break;
          case EGG_JOYBTN_RIGHT: if (event->joy.value) herodx=1; else if (herodx>0) herodx=0; break;
          case EGG_JOYBTN_LEFT: if (event->joy.value) herodx=-1; else if (herodx<0) herodx=0; break;
          case EGG_JOYBTN_RP: if (event->joy.value) egg_request_termination(); break;
        } break;
      case EGG_EVENT_MMOTION: {
          herox+=event->mmotion.x;
          if (herox<0.0) herox+=128.0;
          else if (herox>=128.0) herox-=128.0;
        } break;
      case EGG_EVENT_MBUTTON: if ((event->mbutton.btnid==1)&&(event->mbutton.value==1)) fire_laser(); break;
    }
    if (eventc<16) break;
  }
  
  if ((show_perfect-=elapsed)<=0.0) show_perfect=0.0;
  if ((show_wave-=elapsed)<=0.0) show_wave=0.0;
  
  if (pause) return;
  
  if (!monsterc) generate_monsters();
  
  if ((monster_anim_clock-=elapsed)<=0.0) {
    monster_anim_clock+=MONSTER_ANIM_TIME;
    animate_monsters();
  }
  
  if ((monster_move_clock-=elapsed)<=0.0) {
    monster_move_clock+=MONSTER_MOVE_TIME;
    move_monsters();
  }
  
  if (herodx) {
    herox+=herodx*HERO_SPEED*elapsed;
    if (herox<0.0) herox+=128.0;
    else if (herox>=128.0) herox-=128.0;
  }
  
  struct shot *shot=shotv;
  int i=SHOT_LIMIT;
  for (;i-->0;shot++) {
    if (shot->y<=0.0) continue;
    if ((shot->y-=SHOT_SPEED*elapsed)<=0.0) {
      score>>=1;
      alarm_clock=0.500;
      perfect=0;
      egg_audio_play_sound(0,3,0x00010000,0);
    } else {
      check_monster_shot(shot);
    }
  }
  
  struct firework *firework=fireworkv;
  for (i=fireworkc;i-->0;firework++) {
    firework->ttl-=elapsed;
  }
  while (fireworkc&&(fireworkv[fireworkc-1].ttl<=0.0)) fireworkc--;
  
  if ((score_atrophy_clock-=elapsed)<=0) {
    score_atrophy_clock+=SCORE_ATROPHY_TIME;
    if (score>0) score--;
  }
  
  if ((alarm_clock-=elapsed)<=0.0) alarm_clock=0.0;
}

static void render_game() {
  
  egg_draw_tile(1,texid_tiles,monsterv,monsterc);

  struct shot *shot=shotv;
  int i=SHOT_LIMIT;
  for (;i-->0;shot++) {
    if (shot->y<=0.0) continue;
    int16_t top=(int16_t)shot->y-5;
    int16_t left=shot->x-1;
    egg_draw_decal(1,texid_sprites,left,top,16,0,3,5,0);
  }
    
  if (herodx<0) {
    egg_draw_decal(1,texid_sprites,(int)herox-3,110,9,0,7,8,0);
  } else if (herodx>0) {
    egg_draw_decal(1,texid_sprites,(int)herox-3,110,9,0,7,8,EGG_XFORM_XREV);
  } else {
    egg_draw_decal(1,texid_sprites,(int)herox-4,110,0,0,9,8,0);
  }
  
  struct firework *firework=fireworkv;
  for (i=fireworkc;i-->0;firework++) {
    if (firework->ttl<=0.0) continue;
    int frame=3-(firework->ttl*4.0);
    if (frame<0) frame=0; else if (frame>3) frame=3;
    egg_draw_decal(1,texid_sprites,firework->x-4,firework->y-4,35+frame*8,0,8,8,0);
    egg_draw_decal(1,texid_sprites,firework->x-4,firework->y-12+(int)(firework->ttl*10.0),67,0,11,5,0);
  }
}

void egg_client_render() {

  stars_render();
  
  if (!pause) render_game();
  
  /* (100 per kill * 24 monsters per wave + 1000 bonus) * 3 waves = 10200
   * I believe a human player can't reach 10k. Might not even be mathematically possible, I haven't run all the numbers.
   * (because you lose a point every 100 ms and it takes a nonzero amount of time to shoot all the monsters).
   * Well at any rate, if the score reaches 10k, display "9999".
   * ...I got 8914 for a perfect game. Of course the timing could improve.
   */
  int scorex=100,scorey=122;
  int dscore=(score<0)?0:(score>9999)?9999:score;
  if ((score>=hiscore)||new_hiscore) egg_render_tint(0x00ff00ff);
  egg_draw_decal(1,texid_sprites,scorex,scorey,((dscore/   1000)%10)*5,8,5,5,0); scorex+=6;
  egg_draw_decal(1,texid_sprites,scorex,scorey,((dscore/    100)%10)*5,8,5,5,0); scorex+=6;
  egg_draw_decal(1,texid_sprites,scorex,scorey,((dscore/     10)%10)*5,8,5,5,0); scorex+=6;
  egg_draw_decal(1,texid_sprites,scorex,scorey,((dscore        )%10)*5,8,5,5,0);
  egg_render_tint(0);
  
  scorex=10;
  egg_draw_decal(1,texid_sprites,scorex,scorey,((hiscore/   1000)%10)*5,8,5,5,0); scorex+=6;
  egg_draw_decal(1,texid_sprites,scorex,scorey,((hiscore/    100)%10)*5,8,5,5,0); scorex+=6;
  egg_draw_decal(1,texid_sprites,scorex,scorey,((hiscore/     10)%10)*5,8,5,5,0); scorex+=6;
  egg_draw_decal(1,texid_sprites,scorex,scorey,((hiscore        )%10)*5,8,5,5,0);
  
  if (show_wave>0.0) {
    egg_draw_decal(1,texid_sprites,42,62,50,8,25,5,0);
    egg_draw_decal(1,texid_sprites,72,62,wave*5,8,5,5,0);
    egg_draw_decal(1,texid_sprites,78,62,75,8,5,5,0);
    egg_draw_decal(1,texid_sprites,84,62,3*5,8,5,5,0);
  }
  
  if (show_perfect>0.0) {
    egg_render_tint(((int)(show_perfect*10.0)&1)?0xffff00ff:0x00ff00ff);
    egg_draw_decal(1,texid_sprites,43,69,0,13,45,5,0);
    egg_render_tint(0);
  }
}
