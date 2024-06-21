/* map.h
 * One rectangle of terrain data, and extra attached data.
 * We assume a fixed size, and a fixed limit for extra data.
 * Would not be difficult to make both of those dynamic, if you need to.
 */
 
#ifndef MAP_H
#define MAP_H

#define MAP_COMMANDS_LIMIT 512

struct map {
  uint8_t v[COLC*ROWC];
  uint8_t commands[MAP_COMMANDS_LIMIT];
};

/* Populate map from a resource.
 * In this fixed-size regime, it's literally just `egg_res_get(map,sizeof(*map),EGG_RESTYPE_map,qual,rid)`.
 * But you might need dynamic decoding in the future.
 */
int map_from_res(struct map *map,int qual,int rid);

/* Trigger (cb) for each command, in order.
 * Stops if you return nonzero, and returns the same.
 * (cmdc) is always at least one, and for most commands, the length is knowable from (cmd[0]).
 */
int map_for_each_command(const struct map *map,int (*cb)(const uint8_t *cmd,int cmdc,void *userdata),void *userdata);

/* Length of one command, assuming its opcode is at (src[0]).
 * Errors are possible; we return <0.
 * The high bits of the opcode tell us length or how to measure:
 *   0x00..0x1f: No data, opcode only. 
 *   0x20..0x3f: 2 bytes.
 *   0x40..0x5f: 4 bytes.
 *   0x60..0x7f: 6 bytes.
 *   0x80..0x9f: 8 bytes.
 *   0xa0..0xbf: 12 bytes.
 *   0xc0..0xdf: 16 bytes.
 *   0xe0..0xef: Next byte is length.
 *   0xf0..0xff: Reserved, must be known explicitly. (and none are defined)
 * Opcode zero is reserved as a terminator, and we always report its length as zero.
 */
int map_command_measure(const uint8_t *src,int srcc);

/* Many opcodes only make sense to appear once, and have a payload length of 4 bytes or less.
 * For these, you can call map_get_command() to read the payload as a big-endian integer.
 * Opcodes <0x20 which have no payload return either 0=absent or 1=present.
 * Opcodes 0x20..0x5f return zero if absent, indistinguishable from an actual zero.
 * Opcodes >=0x60 always return zero.
 */
int map_get_command(const struct map *map,uint8_t opcode);

#endif
