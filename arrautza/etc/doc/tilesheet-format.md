# Arrautza Tilesheet Format

Tilesheets are parallel to images, for 256-tile images.
These describe the properties of each tile.

Input format is text.
First line is the name of the table, followed by 16 lines of 32 hex digits each.
Repeat for each table.
No comments, and blank lines are only permitted between tables.

Output format is binary, and consists only of the "physics" table, verbatim, ie 256 bytes.
If smaller than 256, assume the missing slots are zero.
If you're extending Arrautza and need additional properties at runtime, you can add more tables after.
Only they need to be in a constant order and only the last one can be truncated.

## Tables

Runtime only uses the `physics` table. A bunch of others exist only for the editor.

`physics`: Describes how the tile should behave.
See src/general/map.h.
```
0x00 Vacant
0x01 Solid
0x02 Water
0x03 Hole
```

`family`: Arbitrary nonzero identifier indicating that tiles are related.
If zero, assume it's an orphan.

`neighbors`: 8 bits, 1 for each neighbor.
If set, this tile will only be selected when that neighbor belongs to the same family.
(0x80..0x01) = (NW, N, NE, W, E, SW, S, SE).

`weight`: Tile's prominence for randomization. 0..254 = likely..unlikely, 255 = appointment only.
A tile with weight 255 will never be selected automatically by the editor.
