# Eggsamples

Games for Egg that you can use as a starting point or reference.

Everything in this repository -- assets as well as code -- is in the public domain.

## How to Build

- Clone and build Egg [https://github.com/aksommerville/egg].
- Update eggsamples/Makefile with the location of Egg and your WebAssembly compiler.
- Get WABT for the WAT demo: https://github.com/WebAssembly/wabt or `sudo apt install wabt`
- Finally, from the top of this repo, just: `make`

## The Games

ts, minigolf, wat, cfood: Written against an earlier and incompatible version of Egg.
Keeping for now as a reference, while I build out the new ones.

## TODO

- [ ] General utilities.
- - [x] tile_renderer
- - [ ] font
- - [ ] menu
- - [ ] strings
- - [ ] input coersion (fake keyboard, fake pointer, keyboard and touch as joystick, etc)
- [ ] libc
- [ ] Lights-on test ROM. Hit every entry point.
- [ ] A game in C.
- [ ] A game in WAT
- [ ] A game in C++. Not going to bother with C++-conventional libs, but do at least demonstrate it can be done.
- [ ] Other Wasmable languages: Rust, Go, Zig... Pascal? Fortran?
- [ ] Client-side rendering in a laughably small framebuffer.
- [ ] 3d graphics with triangles. I'm really curious how far one can take that. Can I write a StarFox clone?

## Game Ideas

What kind of demo games should I write? Looks like we'll need half a dozen or so.

- Rhythm.
- Space Invaders. Maybe in WAT, since it's not complicated.
- Visual Novel. The important thing being input: Mouse, Touch, Joystick, and Keyboard should all be useable.
- - Ooh ooh! Make it a gritty detective story like Deja Vu so we can call it "Hard Boiled"!
- Platformer.
- Formal RPG.
- At least one game with nice high-resolution graphics, so people don't get the wrong idea!
