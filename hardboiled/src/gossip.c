/* gossip.c
 * This is a special "talk" handler for nouns.
 * Shoeshine Jimmy uses it, to give you a relevant hint (what item to give to whom).
 */
 
#include "hardboiled.h"

/* Each item is a "topic", ie a thing we can talk about.
 * Items already given away are disqualified, and items in current inventory are prioritized.
 */
#define GOSSIP_TOPIC_booze 0
#define GOSSIP_TOPIC_pen 1
#define GOSSIP_TOPIC_letter 2
#define GOSSIP_TOPIC_teacup 3
#define GOSSIP_TOPIC_bottle 4
#define GOSSIP_TOPIC_gab 5
#define GOSSIP_TOPIC_glam 6
#define GOSSIP_TOPIC_apple 7
#define GOSSIP_TOPIC_COUNT 8

#define GOSSIP_TOPIC_FOR_EACH \
  _(booze) \
  _(pen) \
  _(letter) \
  _(teacup) \
  _(bottle) \
  _(gab) \
  _(glam) \
  _(apple)

static int gossip_stringid_for_item(int item) {
  switch (item) {
    #define _(tag) case RID_string_item_##tag: return RID_string_gossip_##tag;
    GOSSIP_TOPIC_FOR_EACH
    #undef _
  }
  return 0;
}

static int gossip_index_for_item(int item) {
  switch (item) {
    #define _(tag) case RID_string_item_##tag: return GOSSIP_TOPIC_##tag;
    GOSSIP_TOPIC_FOR_EACH
    #undef _
  }
  return -1;
}

static int gossip_item_for_index(int index) {
  switch (index) {
    #define _(tag) case GOSSIP_TOPIC_##tag: return RID_string_item_##tag;
    GOSSIP_TOPIC_FOR_EACH
    #undef _
  }
  return -1;
}

void talk_gossip() {
  
  /* If we have three clues, the puzzle is solved.
   * Special message then, and it's all you'll get.
   */
  int cluec=clues_count();
  if (cluec>=3) {
    log_add_string(RID_string_gossip_done);
    return;
  }
  
  /* Consider the available topics.
   * First, if there's anything currently in inventory, select from among those.
   */
  int topicv[GOSSIP_TOPIC_COUNT]={0};
  int topicc=0;
  while (topicc<GOSSIP_TOPIC_COUNT) {
    if (!(topicv[topicc]=inv_get_present(topicc))) break;
    topicc++;
  }
  if (topicc) {
    int choice=rand()%topicc;
    log_add_string(gossip_stringid_for_item(topicv[choice]));
    return;
  }
  
  /* Mark all topics nonzero, ie eligible, then zero those already completed.
   * Then replace the nonzero values with their item id.
   */
  memset(topicv,1,sizeof(topicv));
  int i=0; for (;;i++) {
    int item=inv_get_given(i);
    if (!item) break;
    int topicp=gossip_index_for_item(item);
    if (topicp<0) continue;
    topicv[topicp]=0;
  }
  for (i=GOSSIP_TOPIC_COUNT;i-->0;) {
    if (!topicv[i]) continue;
    topicv[i]=gossip_item_for_index(i);
  }
  
  /* Eliminate zeroes from topicv.
   * It shouldn't be possible for the list to go empty, but if so let's figure the puzzle is solvable.
   */
  topicc=GOSSIP_TOPIC_COUNT;
  for (i=topicc;i-->0;) {
    if (topicv[i]) continue;
    topicc--;
    memmove(topicv+i,topicv+i+1,sizeof(int)*(topicc-i));
  }
  if (!topicc) {
    log_add_string(RID_string_gossip_done);
    return;
  }
  
  /* If every topic is eligible, deliver the special "start" string.
   * User will only see this if no items have been collected or given.
   */
  if (topicc==GOSSIP_TOPIC_COUNT) {
    log_add_string(RID_string_gossip_start);
    return;
  }
  
  /* Select randomly among the eligible topics.
   */
  int choice=rand()%topicc;
  log_add_string(gossip_stringid_for_item(topicv[choice]));
}
