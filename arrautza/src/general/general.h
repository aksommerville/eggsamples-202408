/* general.h
 * These are utilities for adventure games that aren't really specific to Arrautza.
 */
 
#ifndef GENERAL_H
#define GENERAL_H

#include "map.h"
#include "sprite.h"
#include "menu.h"
#include "stobus.h"

/* Enumerated cardinal and diagonal directions.
 * These are selected so you can also use for 8-bit neighbor masks.
 */
#define DIR_NW  0x80
#define DIR_N   0x40
#define DIR_NE  0x20
#define DIR_W   0x10
#define DIR_MID 0x00
#define DIR_E   0x08
#define DIR_SW  0x04
#define DIR_S   0x02
#define DIR_SE  0x01

uint8_t dir_reverse(uint8_t dir);

#endif
