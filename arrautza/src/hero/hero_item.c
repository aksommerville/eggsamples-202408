#include "hero_internal.h"

/* Set (aitem) or (bitem).
 */

static void setitem(struct sprite *sprite,int slot,uint8_t itemid) {
  switch (slot) {
    case 0: SPRITE->aitem=itemid; break;
    case 1: SPRITE->bitem=itemid; break;
  }
}

static void unsetitem(struct sprite *sprite,uint8_t itemid) {
  if (SPRITE->aitem==itemid) SPRITE->aitem=0;
  if (SPRITE->bitem==itemid) SPRITE->bitem=0;
}

/* Sword.
 */
 
#define SWORD_TIME_MIN 0.125
#define SWORD_TIME_MAX 0.750
 
static void hero_sword_begin(struct sprite *sprite,int slot) {
  if (hero_item_in_use(sprite,ITEM_SHIELD)) return;
  if (hero_item_in_use(sprite,ITEM_CLOAK)) return;
  //TODO Check for impulse actions that only happen when the sword is first drawn.
  // The initial check applies across a quarter-turn arc from the hero's right side to front.
  //TODO Sound effect.
  SPRITE->swordtime=0.0;
  SPRITE->sword_drop_pending=0;
  setitem(sprite,slot,ITEM_SWORD);
}

static void hero_sword_end(struct sprite *sprite) {
  if (SPRITE->swordtime>=SWORD_TIME_MIN) {
    unsetitem(sprite,ITEM_SWORD);
  } else {
    SPRITE->sword_drop_pending=1;
  }
}

static void hero_sword_update(struct sprite *sprite,double elapsed) {
  SPRITE->swordtime+=elapsed;
  if (SPRITE->swordtime>SWORD_TIME_MAX) {
    unsetitem(sprite,ITEM_SWORD);
  } else if (SPRITE->sword_drop_pending&&(SPRITE->swordtime>=SWORD_TIME_MIN)) {
    unsetitem(sprite,ITEM_SWORD);
  } else {
    //TODO Check for damage to others. At this point, the sword is stationary, only facing front.
  }
}

/* Shield.
 */
 
#define SHIELD_TIME_MIN 0.125
 
static void hero_shield_begin(struct sprite *sprite,int slot) {
  if (hero_item_in_use(sprite,ITEM_SWORD)) return;
  if (hero_item_in_use(sprite,ITEM_CLOAK)) return;
  SPRITE->shieldtime=0.0;
  SPRITE->shield_drop_pending=0;
  setitem(sprite,slot,ITEM_SHIELD);
  //TODO Sound effect.
}

static void hero_shield_end(struct sprite *sprite) {
  if (SPRITE->shieldtime>=SHIELD_TIME_MIN) {
    unsetitem(sprite,ITEM_SHIELD);
  } else {
    SPRITE->shield_drop_pending=1;
  }
}

static void hero_shield_update(struct sprite *sprite,double elapsed) {
  SPRITE->shieldtime+=elapsed;
  if (SPRITE->shield_drop_pending&&(SPRITE->shieldtime>=SHIELD_TIME_MIN)) {
    unsetitem(sprite,ITEM_SHIELD);
  }
}

/* Bow.
 */
 
static int hero_bow_valid(struct sprite *sprite) {
  if (g.itemqual[ITEM_BOW]<1) return 0;
  //TODO Check map? Other conditions?
  return 1;
}
 
static void hero_bow_begin(struct sprite *sprite,int slot) {
  if (hero_bow_valid(sprite)) {
    g.itemqual[ITEM_BOW]--;
    uint8_t argv[]={SPRITE->facedir,15,TILESIZE};
    struct sprite *arrow=sprite_spawn_resless(&sprctl_missile,RID_image_hero,0x35,sprite->x,sprite->y,argv,sizeof(argv));
    if (arrow) {
      if ((SPRITE->facedir==DIR_W)||(SPRITE->facedir==DIR_E)) {
        sprite_set_hitbox(arrow,0.75,0.125,0.0,0.0);
      } else {
        sprite_set_hitbox(arrow,0.125,0.75,0.0,0.0);
      }
    }
  } else {
    //TODO Reject sound.
  }
}

/* Bomb.
 */
 
static int hero_bomb_valid(struct sprite *sprite) {
  if (g.itemqual[ITEM_BOMB]<1) return 0;
  //TODO Check map? Other conditions?
  return 1;
}
 
static void hero_bomb_begin(struct sprite *sprite,int slot) {
  if (hero_bomb_valid(sprite)) {
    g.itemqual[ITEM_BOMB]--;
    struct sprite *bomb=sprite_spawn_resless(&sprctl_bomb,RID_image_hero,0x36,sprite->x,sprite->y,0,0);
    if (bomb) {
    }
  } else {
    //TODO Reject sound.
  }
}

/* Raft.
 */
 
static void hero_raft_begin(struct sprite *sprite,int slot) {
  setitem(sprite,slot,ITEM_RAFT);
    egg_log("%s TODO %s:%d",__func__,__FILE__,__LINE__);
  //TODO What does the raft do, how does it work?
}

/* Cloak.
 */
 
static void hero_cloak_begin(struct sprite *sprite,int slot) {
  if (hero_item_in_use(sprite,ITEM_SWORD)) return;
  if (hero_item_in_use(sprite,ITEM_SHIELD)) return;
  setitem(sprite,slot,ITEM_CLOAK);
  //TODO Sound effect.
}

static void hero_cloak_end(struct sprite *sprite) {
  //TODO Sound effect.
}

/* Stopwatch.
 */
 
static void hero_stopwatch_begin(struct sprite *sprite,int slot) {
  setitem(sprite,slot,ITEM_STOPWATCH);
  egg_log("%s TODO %s:%d",__func__,__FILE__,__LINE__);
  //TODO Sound effect.
}

static void hero_stopwatch_end(struct sprite *sprite) {
  //TODO Sound effect.
}

static void hero_stopwatch_update(struct sprite *sprite,double elapsed) {
  //TODO recurring sound effect.
}

/* Gold.
 */
 
static int hero_gold_valid(struct sprite *sprite) {
  if (g.itemqual[ITEM_GOLD]<1) return 0;
  //TODO Check map? Other conditions?
  return 1;
}
 
static void hero_gold_begin(struct sprite *sprite,int slot) {
  if (hero_gold_valid(sprite)) {
    g.itemqual[ITEM_GOLD]--;
    uint8_t argv[]={SPRITE->facedir,10,TILESIZE};
    struct sprite *coin=sprite_spawn_resless(&sprctl_missile,RID_image_hero,0x43,sprite->x,sprite->y,argv,sizeof(argv));
    if (coin) {
      sprite_set_hitbox(coin,0.250,0.250,0.0,0.0);
    }
    //TODO If there's a receptacle directly in front of us, should we skip spawning?
  } else {
    //TODO Reject sound.
  }
}

/* Hammer.
 */
 
static void hero_hammer_begin(struct sprite *sprite,int slot) {
  egg_log("%s TODO %s:%d",__func__,__FILE__,__LINE__);
  //TODO Animation, sound effect, and look for things to break.
}

/* Compass.
 */
 
static void hero_compass_begin(struct sprite *sprite,int slot) {
  setitem(sprite,slot,ITEM_COMPASS);
  egg_log("%s TODO %s:%d",__func__,__FILE__,__LINE__);
  //TODO The important thing the compass does, point you toward some target, that happens elsewhere.
  // Activating the item lets you pick a target from some list.
  // I think we want a menu for this.
}

/* Begin, end, update: Dispatch on itemid.
 */
 
void hero_item_begin(struct sprite *sprite,uint8_t itemid,int slot) {
  if (!itemid) return;
  switch (slot) {
    case 0: if (SPRITE->aitem) hero_item_end(sprite,SPRITE->aitem); if (SPRITE->aitem) return; break;
    case 1: if (SPRITE->bitem) hero_item_end(sprite,SPRITE->bitem); if (SPRITE->bitem) return; break;
    default: return;
  }
  switch (itemid) {
    case ITEM_SWORD: hero_sword_begin(sprite,slot); break;
    case ITEM_SHIELD: hero_shield_begin(sprite,slot); break;
    case ITEM_BOW: hero_bow_begin(sprite,slot); break;
    case ITEM_BOMB: hero_bomb_begin(sprite,slot); break;
    case ITEM_RAFT: hero_raft_begin(sprite,slot); break;
    case ITEM_CLOAK: hero_cloak_begin(sprite,slot); break;
    case ITEM_STOPWATCH: hero_stopwatch_begin(sprite,slot); break;
    case ITEM_GOLD: hero_gold_begin(sprite,slot); break;
    case ITEM_HAMMER: hero_hammer_begin(sprite,slot); break;
    case ITEM_COMPASS: hero_compass_begin(sprite,slot); break;
  }
}

void hero_item_end(struct sprite *sprite,uint8_t itemid) {
  if (!itemid) return;
  
  // Items that interfere with termination must be called out separate, here.
  // eg the sword has a minimum deploy time.
  switch (itemid) {
    case ITEM_SWORD: hero_sword_end(sprite); return;
    case ITEM_SHIELD: hero_shield_end(sprite); return;
  }
  
  unsetitem(sprite,itemid);
  switch (itemid) {
    // Most items shouldn't need an end hook.
    case ITEM_CLOAK: hero_cloak_end(sprite); break;
    case ITEM_STOPWATCH: hero_stopwatch_end(sprite); break;
  }
}

void hero_item_update(struct sprite *sprite,uint8_t itemid,double elapsed) {
  if (!itemid) return;
  switch (itemid) {
    case ITEM_SWORD: hero_sword_update(sprite,elapsed); break;
    case ITEM_SHIELD: hero_shield_update(sprite,elapsed); break;
    case ITEM_STOPWATCH: hero_stopwatch_update(sprite,elapsed); break;
  }
}

/* Public tests for equipeed and active items.
 */

int hero_item_in_use(struct sprite *sprite,uint8_t itemid) {
  if (!itemid) return 0;
  if (SPRITE->aitem==itemid) return 1;
  if (SPRITE->bitem==itemid) return 1;
  return 0;
}

static uint8_t no_walk_items[1+ITEM_COUNT]={
  [ITEM_SWORD]=1,
  [ITEM_SHIELD]=1,
};

int hero_walk_inhibited_by_item(struct sprite *sprite) {
  if ((SPRITE->aitem<sizeof(no_walk_items))&&no_walk_items[SPRITE->aitem]) return 1;
  if ((SPRITE->bitem<sizeof(no_walk_items))&&no_walk_items[SPRITE->bitem]) return 1;
  return 0;
}
