/* puzzle.c
 * Manages the logic puzzle at the heart of the game.
 * There are seven suspects each with three properties.
 * Each property has three possible values.
 * There are eight witnesses, each with a clue that establishes one property value.
 * So it's a very simple puzzle. If you pick the right witnesses, you'll solve it with three clues.
 *
 * Properties:
 *  Hair: Brown, Blond, Bald
 *  Shirt: Blue, Green, Red
 *  Tie: None, Neck, Bow
 */

#include "hardboiled.h"

#define CLUE_COUNT 8

#define FIELD_HAIR 0
#define FIELD_SHIRT 1
#define FIELD_TIE 2
#define FIELD_COUNT 3
 
struct suspect { // all values are 0,1,2
  uint8_t v[FIELD_COUNT];
};

struct clue {
  uint8_t field; // 0,1,2
  uint8_t value; // 0,1,2
};

/* Globals.
 */
 
static struct suspect suspectv[SUSPECT_COUNT];
static struct clue cluev[CLUE_COUNT];
// Separate list of revealed clues instead of a property on struct clue, so order of revelation is preserved.
static struct clue *clue_revealedv[CLUE_COUNT];
static int clue_revealedc=0;
static int guiltyp=0; // 0..SUSPECT_COUNT-1

/* Randomizing init helpers.
 */
 
static struct suspect *random_suspect_with_invalid_field(int fieldid) {
  int candidatec=0;
  int i=SUSPECT_COUNT;
  struct suspect *suspect=suspectv;
  for (;i-->0;suspect++) {
    if (suspect->v[fieldid]>2) candidatec++;
  }
  if (candidatec<1) return suspectv; // oops, overwrite [0]
  int choice=rand()%candidatec;
  for (suspect=suspectv,i=SUSPECT_COUNT;i-->0;suspect++) {
    if (suspect->v[fieldid]<3) continue;
    if (!choice--) return suspect;
  }
  return suspectv; // oops, overwrite [0]
}

static void clue_init_random(int fieldid) {
  int candidatec=0;
  int i=CLUE_COUNT;
  struct clue *clue=cluev;
  for (;i-->0;clue++) {
    if (clue->field>2) candidatec++;
  }
  if (candidatec<1) return;
  int choice=rand()%candidatec;
  for (clue=cluev,i=CLUE_COUNT;i-->0;clue++) {
    if (clue->field<3) continue;
    if (choice--) continue;
    clue->field=fieldid;
    clue->value=suspectv[guiltyp].v[fieldid];
    return;
  }
}
 
/* Init.
 */
 
void puzzle_init() {
  clue_revealedc=0;
  
  /* Use every possible suspect field value twice.
   */
  memset(suspectv,0xff,sizeof(suspectv));
  int fieldid;
  for (fieldid=FIELD_COUNT;fieldid-->0;) random_suspect_with_invalid_field(fieldid)->v[fieldid]=0;
  for (fieldid=FIELD_COUNT;fieldid-->0;) random_suspect_with_invalid_field(fieldid)->v[fieldid]=0;
  for (fieldid=FIELD_COUNT;fieldid-->0;) random_suspect_with_invalid_field(fieldid)->v[fieldid]=1;
  for (fieldid=FIELD_COUNT;fieldid-->0;) random_suspect_with_invalid_field(fieldid)->v[fieldid]=1;
  for (fieldid=FIELD_COUNT;fieldid-->0;) random_suspect_with_invalid_field(fieldid)->v[fieldid]=2;
  for (fieldid=FIELD_COUNT;fieldid-->0;) random_suspect_with_invalid_field(fieldid)->v[fieldid]=2;
  
  /* There are now three fields unassigned. Assign random values to those.
   */
  struct suspect *suspect=suspectv;
  int i=SUSPECT_COUNT;
  for (;i-->0;suspect++) {
    uint8_t *v=suspect->v;
    int ii=FIELD_COUNT;
    for (;ii-->0;v++) {
      if (*v>2) *v=rand()%3;
    }
  }
  
  /* Who done it?
   */
  guiltyp=rand()%SUSPECT_COUNT;
  
  /* Make two clues for each field, and two random clues.
   */
  memset(cluev,0xff,sizeof(cluev));
  clue_init_random(0);
  clue_init_random(0);
  clue_init_random(1);
  clue_init_random(1);
  clue_init_random(2);
  clue_init_random(2);
  struct clue *clue=cluev;
  for (i=CLUE_COUNT;i-->0;clue++) {
    if (clue->field<3) continue;
    clue->field=rand()%3;
    clue->value=suspectv[guiltyp].v[clue->field];
  }
  
  /**
  egg_log("---- initialized puzzle [%s:%d] ----",__FILE__,__LINE__);
  for (suspect=suspectv,i=0;i<SUSPECT_COUNT;i++,suspect++) {
    egg_log("suspect: [%d,%d,%d] %s",suspect->v[0],suspect->v[1],suspect->v[2],(i==guiltyp)?"*":"");
  }
  for (clue=cluev,i=CLUE_COUNT;i-->0;clue++) {
    egg_log("clue: %d=%d",clue->field,clue->value);
  }
  /**/
}

/* Test clue reveal.
 */
 
static int clue_is_revealed(const struct clue *clue) {
  int i=clue_revealedc;
  struct clue **q=clue_revealedv;
  for (;i-->0;q++) if (clue==*q) return 1;
  return 0;
}

/* Generate text for clue.
 * There's only nine possible clues, so I'm writing them all out as individual strings.
 * If we wanted to compose the text programmatically, could do that here instead.
 */
 
static int text_for_clue(char *dst,int dsta,const struct clue *clue) {
  int stringid=10+clue->field*3+clue->value;
  const char *src=0;
  int srcc=text_get_string(&src,stringid);
  if (srcc<0) return 0;
  if (srcc<=dsta) memcpy(dst,src,srcc);
  return srcc;
}

/* Reveal a random clue.
 */
 
int puzzle_reveal_clue(char *dst,int dsta) {
  if (clue_revealedc>=CLUE_COUNT) return 0;
  struct clue *candidatev[CLUE_COUNT];
  int candidatec=0;
  struct clue *q=cluev;
  int i=CLUE_COUNT;
  for (;i-->0;q++) {
    if (clue_is_revealed(q)) continue;
    candidatev[candidatec++]=q;
  }
  if (candidatec<1) return 0; // oops? this can't be possible
  int choice=rand()%candidatec;
  struct clue *clue=candidatev[choice];
  clue_revealedv[clue_revealedc++]=clue;
  inventory_add_clue(clue->field,clue->value);
  return text_for_clue(dst,dsta,clue);
}

/* Public accessors.
 */
 
void puzzle_get_suspect(int *hair,int *shirt,int *tie,int suspectp) {
  if ((suspectp<0)||(suspectp>=SUSPECT_COUNT)) {
    *hair=*shirt=*tie=0;
  } else {
    const struct suspect *suspect=suspectv+suspectp;
    *hair=suspect->v[0];
    *shirt=suspect->v[1];
    *tie=suspect->v[2];
  }
}
