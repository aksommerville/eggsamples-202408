#include "../arrautza.h"

/* Decode map from resource.
 */
 
int map_from_res(struct map *map,int qual,int rid) {
  int c=egg_res_get(map,sizeof(struct map),EGG_RESTYPE_map,qual,rid);
  egg_log("%s (%d:%d:%d) c=%d",__func__,EGG_RESTYPE_map,qual,rid,c);
  if ((c<COLC*ROWC)||(c>sizeof(struct map))) return -1;
  if (c<sizeof(struct map)) { // Force termination if not saturated.
    map->commands[c-sizeof(map->v)]=0;
  }
  return 0;
}

/* Iterate commands.
 */

int map_for_each_command(const struct map *map,int (*cb)(const uint8_t *cmd,int cmdc,void *userdata),void *userdata) {
  int commandsp=0,err;
  while (commandsp<MAP_COMMANDS_LIMIT) {
    int cmdc=map_command_measure(map->commands+commandsp,MAP_COMMANDS_LIMIT-commandsp);
    if (cmdc<1) break;
    if (err=cb(map->commands+commandsp,cmdc,userdata)) return err;
    commandsp+=cmdc;
  }
  return 0;
}

/* Measure command.
 */

int map_command_measure(const uint8_t *src,int srcc) {
  if (srcc<1) return -1;
  switch (src[0]&0xe0) {
    case 0x00: return src[0]?1:0; // Opcode zero is a terminator, report length zero for it.
    case 0x20: if (srcc<3) return -1; return 3;
    case 0x40: if (srcc<5) return -1; return 5;
    case 0x60: if (srcc<7) return -1; return 7;
    case 0x80: if (srcc<9) return -1; return 9;
    case 0xa0: if (srcc<13) return -1; return 13;
    case 0xc0: if (srcc<17) return -1; return 17;
    case 0xe0: {
        if (src[0]>=0xf0) return -1; // Known-length. None defined.
        if (srcc<2) return -1;
        int paylen=src[1];
        if (2+paylen>srcc) return -1;
        return 2+paylen;
      }
  }
  return -1;
}

/* Get first occurrence of scalar command.
 */
 
int map_get_command(const struct map *map,uint8_t opcode) {
  if (opcode>=0x60) return 0;
  int p=0;
  while (p<MAP_COMMANDS_LIMIT) {
    int len=map_command_measure(map->commands+p,MAP_COMMANDS_LIMIT-p);
    if (len<1) break;
    if (opcode==map->commands[p]) {
      const uint8_t *b=map->commands+p+1;
      switch (len) {
        case 1: return 1;
        case 3: return (b[0]<<8)|b[1];
        case 5: return (b[0]<<24)|(b[1]<<16)|(b[2]<<8)|b[3];
      }
      return 0;
    }
    p+=len;
  }
  return 0;
}
