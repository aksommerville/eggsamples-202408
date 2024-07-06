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

Everything is playable right here in your browser.
(I'm going to forget to keep those up to date. Reminder: `make playables` on branch `gh-pages`).

### lightson

<a href="https://aksommerville.github.io/eggsamples/playable/lightson.html">Play</a>

Egg's official test ROM.
It exposes pretty much all of the platform's features, in a fashion easy to validate.
I use this to discover differences between ports of the platform.

### shmup

<a href="https://aksommerville.github.io/eggsamples/playable/shmup.html">Play</a>

Like Space Invaders.
Intended to demonstrate the barest minimum of everything.
At just a hair over 10 kB, this is about as small as an Egg game can be.
Also it's not much fun.
Write something better!

### hardboiled

<a href="https://aksommerville.github.io/eggsamples/playable/hardboiled.html">Play</a>

Point-and-click detective game, culminates in a pretty simple elimination puzzle.
Using this as the guinea pig for a modal input manager.

### arrautza

<a href="https://aksommerville.github.io/eggsamples/playable/arrautza.html">Play</a>

The Leggend of Arrautza! That's the Basque word for "egg", and it sounds cool.
A Legend of Zelda clone, more or less.
One screen at a time, no scrolling. Multiplayer support.

### gravedigger

<a href="https://aksommerville.github.io/eggsamples/playable/gravedigger.html">Play</a>

Example of rendering into a pure software framebuffer.
Dig a hole and bury the coffins.
You'll notice there's a pretty substantial CPU cost for rendering client-side, and it's heavier in wasm than native.
Expect that cost to scale with the size of the framebuffer.

## TODO

- [ ] General utilities.
- - [ ] input coersion (fake keyboard, fake pointer, keyboard and touch as joystick, etc)
- - - [ ] Touch=>Joy
- [ ] A game in WAT
- [ ] A game in C++. Not going to bother with C++-conventional libs, but do at least demonstrate it can be done.
- [ ] Other Wasmable languages: Rust, Go, Zig... Pascal? Fortran?
- [ ] Realistic OpenGL demo. Hit as much of GLES2 as we can manage.
- [ ] 3d graphics with triangles. I'm really curious how far one can take that. Can I write a StarFox clone?
- [ ] Touch events get processed twice (presumably being aliased as mouse by the browser)
- [ ] Make a separate "eggzotics" repo for projects that target Egg in addition to some highly-constrained native platform (Playdate, TinyArcade, ...)
- [ ] Wizard for copying a project.
- [ ] arrautza
- - [x] Sprite collisions
- - [ ] Sprite editor
- - - [ ] image
- - - [ ] tileid
- - - [ ] xform
- - - [ ] sprctl
- - - [ ] groups
- - [ ] Map editor: Click to create POI command.
- - [ ] Map editor: Can we show a thumbnail for sprite commands instead of the generic monster?
- - [ ] Map editor: What could we do to provide metadata labels for sprite args, per sprctl?
- - [ ] Global state store
- - [ ] Persistence
- - [ ] Dialogue
- - [ ] Cutscenes
- - [ ] Hazards
- - [ ] Combat
- - - [ ] Sword
- - - [ ] Bow
- - - [ ] Shield
- - [ ] Sprite controllers
- - - [ ] Block that slides to nearest grid cell
- - - [ ] Treadle
- - - [ ] Animate forever, animate once
- - [x] Neighbor transitions
- - [x] Defer map load
- - [x] Index doors on map load
- - [ ] Possible to enter some invalid state and get stuck, by walking NE thru two transitions (even after deferred load and transitions).

## Game Ideas

What kind of demo games should I write? Looks like we'll need half a dozen or so.

- Rhythm.
- ~Space Invaders. Maybe in WAT, since it's not complicated.~
- ~Visual Novel. The important thing being input: Mouse, Touch, Joystick, and Keyboard should all be useable.~
- - ~Ooh ooh! Make it a gritty detective story like Deja Vu so we can call it "Hard Boiled"!~
- Platformer.
- Formal RPG.
- At least one game with nice high-resolution graphics, so people don't get the wrong idea!
