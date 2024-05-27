# Eggsamples

Games for Egg that you can use as a starting point or reference.

Everything in this repository -- assets as well as code -- is in the public domain.
Except `stdlib/math.c`, copied from newlib (https://sourceware.org/git/newlib-cygwin.git) and modified.

## How to Build

- Clone and build Egg [https://github.com/aksommerville/egg].
- Update eggsamples/Makefile with the location of Egg and your WebAssembly compiler.
- Get WABT for the WAT demo: https://github.com/WebAssembly/wabt or `sudo apt install wabt`
- Finally, from the top of this repo, just: `make`

## The Games

ts, minigolf, wat, cfood: Written against an earlier and incompatible version of Egg.
Keeping for now as a reference, while I build out the new ones.

### lightson

Egg's official test ROM.
It exposes pretty much all of the platform's features, in a fashion easy to validate.
I use this to discover differences between ports of the platform.

### shmup

Like Space Invaders.
Intended to demonstrate the barest minimum of everything.
At just a hair over 10 kB, this is about as small as an Egg game can be.
Also it's not much fun.
Write something better!

### hardboiled

Point-and-click detective game, culminates in a pretty simple elimination puzzle.
Using this as the guinea pig for a modal input manager.

## TODO

- [ ] General utilities.
- - [x] tile_renderer
- - [x] font
- - [x] menu
- - [x] strings
- - [ ] input coersion (fake keyboard, fake pointer, keyboard and touch as joystick, etc)
- [x] libc
- [ ] Lights-on test ROM. Hit every entry point.
- - [ ] Sound effects on UI activity. (it's not just fluff, it helps validate audio)
- - [ ] Report high water mark and final state (memory leaks) from malloc.
- [ ] A game in C.
- [ ] A game in WAT
- [ ] A game in C++. Not going to bother with C++-conventional libs, but do at least demonstrate it can be done.
- [ ] Other Wasmable languages: Rust, Go, Zig... Pascal? Fortran?
- [ ] Client-side rendering in a laughably small framebuffer.
- [ ] 3d graphics with triangles. I'm really curious how far one can take that. Can I write a StarFox clone?
- [ ] inkeep: Hide cursor until first event. It's misleading, I catch myself clicking out of the window because the cursor is there in the middle.
- [ ] malloc.c produces a 16-MB intermediate file. No harm, but can we prevent that?

## Game Ideas

What kind of demo games should I write? Looks like we'll need half a dozen or so.

- Rhythm.
- ~Space Invaders. Maybe in WAT, since it's not complicated.~
- ~Visual Novel. The important thing being input: Mouse, Touch, Joystick, and Keyboard should all be useable.~
- - ~Ooh ooh! Make it a gritty detective story like Deja Vu so we can call it "Hard Boiled"!~
- Platformer.
- Formal RPG.
- At least one game with nice high-resolution graphics, so people don't get the wrong idea!
