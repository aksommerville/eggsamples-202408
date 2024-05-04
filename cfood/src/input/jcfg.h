/* jcfg.h
 * Interactive joystick configuration.
 * We take input from joy2 and present a modal UI "Press button A", etc.
 */
 
#ifndef JCFG_H
#define JCFG_H

struct jcfg;
struct bus;
struct font;

void jcfg_del(struct jcfg *jcfg);

struct jcfg *jcfg_new(struct bus *bus);

/* We'll use only the last one you set, of font and tilesheet.
 */
void jcfg_set_font(struct jcfg *jcfg,struct font *font);
void jcfg_set_tilesheet(struct jcfg *jcfg,int texid);

void jcfg_set_strings(struct jcfg *jcfg,
  int stringid_device, // "Device", will be followed by device name.
  int stringid_fault, // eg "Fault! Please press %."
  int stringid_press, // eg "Please press %."
  int stringid_again // eg "Press % again."
);

int jcfg_is_complete(const struct jcfg *jcfg);

void jcfg_update(struct jcfg *jcfg,double elapsed);
void jcfg_render(struct jcfg *jcfg);

// devid zero to disable
void jcfg_enable(struct jcfg *jcfg,int devid);

#endif
