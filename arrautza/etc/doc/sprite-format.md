# Arrautza Sprite Format

Sprites work almost the same way as maps, except obviously there's no image data.

## Text Format

Each non-empty line is a command.
'#' starts a line comment, permitted after real lines in addition to full line.

Command starts with its opcode, which is a `SPRITECMD_*` symbol from `sprite.h`, or an integer in 0..255.

Must be followed by arguments that produce the expected output length (see "Commands" below).

- Plain integer, decimal or "0x" + hexadecimal: One byte.
- Suffix 'u16', 'u24', or 'u32' for multibyte integers, eg "0x123456u24". Will emit big-endianly.
- `FLD_*` as defined in arrautza.h => 1 byte.
- `sprctl:NAME` produces 2-byte sprctl id.
- `groups:NAME1,NAME2,...` produces a 4-byte group mask.
- `TYPE:NAME` emits a resource's rid in 2 bytes. "TYPE" is only for lookup purposes at edit and build time.
- Quoted strings emit verbatim bytewise. See `sr_string_eval()`. Quote `"` only.

## Binary Format

Loose commands, with a hard-coded limit in `src/generic/sprite.h`, 128 bytes currently.

Leading byte of a command describes its length, usually.
Zero is reserved as commands terminator.

We expose the original resource at runtime, but also perform some decode as we load it.
So do not `egg_res_get()` directly on sprite resources; use `sprdef_get()`. Qualifier is always zero.

## Commands

Generic command sizing works exactly the same as maps:
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
