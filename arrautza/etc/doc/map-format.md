# Arrautza Map Format

This is intended to be generic enough that future projects can use it as-is (or with mild adjustment).

## Text Format

Starts with the map image. ROWC (22) lines of COLC*2 (80) hexadecimal digits.
The map image's shape must be exact, and must be the very first thing in the file.
(Let the compiler be dumb about it).
Map image must be terminated by EOF or a blank line.

After the map image, each non-empty line is a command.
'#' starts a line comment, permitted after real lines in addition to full line.

Command starts with its opcode, which is a `MAPCMD_*` symbol from `arrautza.h`, or an integer in 0..255.

Must be followed by arguments that produce the expected output length (see "Commands" below).

- Plain integer, decimal or "0x" + hexadecimal: One byte.
- Suffix 'u16', 'u24', or 'u32' for multibyte integers, eg "0x123456u24". Will emit big-endianly.
- `@X,Y` produces two bytes. (editor will look for these tokens, and assume it's a position in the map).
- `@X,Y,W,H` produces four bytes.
- `TYPE:NAME` emits a resource's rid in 2 bytes. "TYPE" is only for lookup purposes at edit and build time.
- Quoted strings emit verbatim bytewise. See `sr_string_eval()`. Quote `"` only.

## Binary Format

Starts with the map data, exactly COLC*ROWC (40*22==880) bytes.

Followed by loose commands, with a hard-coded limit in `src/generic/map.h`, 512 currently.

Leading byte of a command describes its length, usually.
Zero is reserved as commands terminator.

Everything in the binary format should be consumed bytewise, so there is no need for a dedicated decode step.
Just `egg_res_get()` onto a `struct map` and validate length.

## Commands

Generic command sizing works exactly the same as sprites:
```
00     : Terminator.
01..1f : No payload.
20..3f : 2 bytes payload.
40..5f : 4 bytes payload.
60..7f : 6 bytes payload.
80..9f : 8 bytes payload.
a0..bf : 12 bytes payload.
c0..df : 16 bytes payload.
e0..ef : Next byte is payload length.
f0..ff : Reserved, length must be known explicitly.
```
