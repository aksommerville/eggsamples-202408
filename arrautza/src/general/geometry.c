#include "arrautza.h"
#include "general.h"

/* Reverse direction.
 */
 
uint8_t dir_reverse(uint8_t dir) {
  switch (dir) {
    case DIR_NW: return DIR_SE;
    case DIR_N: return DIR_S;
    case DIR_NE: return DIR_SW;
    case DIR_W: return DIR_E;
    case DIR_E: return DIR_W;
    case DIR_SW: return DIR_NE;
    case DIR_S: return DIR_N;
    case DIR_SE: return DIR_NW;
    case 0: return 0;
    default: {
        uint8_t dst=0;
        uint8_t mask=0x80;
        for (;mask;mask>>=1) if (dir&mask) dst|=dir_reverse(mask);
        return dst;
      }
  }
}
