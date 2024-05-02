# Eggsamples

Games for Egg that you can use as a starting point or reference.

Everything in this repository -- assets as well as code -- is in the public domain.

## How to Build

- Clone and build Egg [https://github.com/aksommerville/egg].
- Update eggsamples/Makefile with the location of Egg and WASI-SDK.
- Ensure `tsc` is installed globally (`npm i -g typescript`).
- Get WABT for the WAT demo: https://github.com/WebAssembly/wabt
- Finally, from the top of this repo, just: `make`

## The Games

### ts

Troll Slayer!

It's not really a game. Just fooling around with helper libraries for Typescript.
Contains some useful input mapping and text helpers, generic and ready to copy out.

### minigolf

Use mouse to knock the ball into the hole.
Not much fun as a game, since I didn't include barriers or hazards. Takes some imagination i guess.

Demonstrates the bare-minimum build system: A 20-line Makefile, no other config, and no intermediate artifacts.
It can only be done this way if you code in Javascript.

### wat

Experimenting with writing a game in straight WebAssembly Text.
As I start this, I don't actually know WebAssembly Text. So that's the first challenge.

https://webassembly.github.io/spec/core/text/index.html

Wrote enough to prove the concept. It's totally doable. But why would you.

### cfood

Similar to `ts`, building out some helpers in C.

## TODO

- [ ] ts: Do something useful.
- [x] Flesh out Typescript helpers. TileRenderer is a good start.
- [ ] Then similar things in C.
- [ ] A game in C.
- [x] A game in WAT, am I up to it?
- [ ] A game in C++ because surely somebody is going to want that. Yuck. Make sure the C header can include ok.
- [x] "Least possible amount of build tooling". Code in JS, and build with a single call to `eggrom`.
- [ ] Other Wasmable languages: Rust, Go, Zig... Pascal? Fortran?
- [ ] Client-side rendering in a laughably small framebuffer.

## Game Ideas

What kind of demo games should I write? Looks like we'll need half a dozen or so.

- ~Mini Golf. A nice fit for the minimal one.~
- Rhythm.
- Multiplayer Snake with WebSockets.
- Space Invaders. Maybe in WAT, since it's not complicated.
- Visual Novel. The important thing being input: Mouse, Touch, Joystick, and Keyboard should all be useable.
- Platformer.
- Formal RPG.
- At least one game with nice high-resolution graphics, so people don't get the wrong idea!
