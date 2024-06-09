#ifndef GRAVEDIGGER_H
#define GRAVEDIGGER_H

#include <stdint.h>
#include <egg/egg.h>

// Must agree with metdata.
#define SCREENW 240
#define SCREENH 134

// Dimensions of the full RGBA spritesheet. (image:0:2)
#define SPRITESW 70
#define SPRITESH 21

// Colors are 0xAABBGGRR. (Wasm is always little-endian).
#define COLOR_SKY 0xffe0b060
#define COLOR_DIRT 0xff102040
#define COLOR_IS_DIRT(px) (!((px)&0x00c00000))

#define DAY_LENGTH 60.0 /* seconds */

#define SND_DIG 1
#define SND_DIG_REJECT 2
#define SND_CARRY 3
#define SND_CARRY_REJECT 4
#define SND_UNCARRY 5
#define SND_JUMP 6
#define SND_VICTORY 7
#define SND_FAILURE 8

// main.c
int choose_and_move_dirt(int x,int y,int dx); // => nonzero if changed
void repair_terrain(int x,int w);

// gd_render.c
void redraw_terrain(uint32_t *dst,int dirth);
void copy_framebuffer(uint32_t *dst,const uint32_t *src);
void blit_sprite(uint32_t *dst,int dstx,int dsty,const uint32_t *src,int srcx,int srcy,int w,int h,int flop);
int find_highest_dirt(const uint32_t *fb,int x,int y,int w,int h); // => 0..h, position of highest dirt pixel in this box

// gd_hero.c
struct hero {
  double x; // center
  double y; // bottom
  int dx;
  int facex;
  int frame;
  double clock;
  int carrying;
  int seated;
  int effort; // Nonzero if we ascended on the last frame; slow down the next motion.
  int jump;
  double jumppower;
  double gravity;
  double coyote; // Counts down after unseating, a brief interval when jumping is still allowed.
  double digclock; // Counts down during dig animation; no other activity permitted until it reaches zero.
};
void hero_reset(struct hero *hero);
void hero_render(uint32_t *dst,const struct hero *hero);
void hero_check_seated(struct hero *hero,int bubble_up);
void hero_update(struct hero *hero,double elapsed);
void hero_update_noninteractive(struct hero *hero,double elapsed);
void hero_walk_begin(struct hero *hero,int dx);
void hero_walk_end(struct hero *hero);
void hero_jump_begin(struct hero *hero);
void hero_jump_end(struct hero *hero);
void hero_toggle_carry(struct hero *hero);
void hero_dig(struct hero *hero);

// gd_coffin.c
#define COFFIN_LIMIT 8
#define COFFIN_WIDTH 11 /* Must match image. */
#define COFFIN_HEIGHT 4
struct coffin {
  int x,y; // Top left corner.
  int buried;
};
int generate_coffins(struct coffin *dst,int dsta); // => dstc (0..dsta)
int coffin_is_buried(const struct coffin *coffin);
void coffins_render(uint32_t *dst,struct coffin *coffinv,int coffinc);
void coffins_force_valid(struct coffin *coffinv,int coffinc); // Move coffins down or dirt up to ensure no gaps or overlap against terrain.
int test_coffin_pixel(int x,int y);
int coffin_find(const struct coffin *coffinv,int coffinc,int x,int y);
struct coffin *coffin_add();
void coffin_remove(int p);

extern struct globals {
  uint32_t fb[SCREENW*SCREENH]; // Redrawn each frame.
  uint32_t terrain[SCREENW*SCREENH]; // Background image, changes only on user actions.
  uint32_t spritebits[SPRITESW*SPRITESH];
  struct hero hero;
  struct coffin coffinv[COFFIN_LIMIT];
  int coffinc;
  double dayclock; // Counts down, game over at zero.
  int vampire_flop_clock;
} g;

#endif
