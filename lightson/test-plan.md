# Egg Lights-On Test Plan

After major changes to the platform, or porting to a new platform, or whenever,
run the lightson ROM and go thru the checklist below.

Ensure that web works in both the load-rom and embedded cases, in all browsers and operating systems.

Test true-native, fake-native, and external-load on all native platforms.

## Global
- [ ] ROM launches. You should see white text on a dark blue background, with a red square upper left.
- [ ] Keyboard (arrows, enter, escape) navigates menus.
- [ ] System cursor should be visible.
- [ ] Mouse navigates menus. Must highlight options on hover. Click the red square to go back.
- [ ] Joysticks navigate menus (dpad, south, west).
- [ ] Going Back from the main menu terminates.
- [ ] UI motion plays a click sound, activate a "thunk", and back a bird tweet.
- [ ] Native: After quitting, check the logged frame rate and CPU load.
- - Frame rate should be close to 60.
- - Load is typically under 0.030 on my PC. Anything over 0.100 would be worrisome.
- [ ] Check malloc stats on quit. "waterline" can increase over time. "allocated" should always be small, otherwise we must have a leak.
- - (A leak would be a problem with lights-on itself, not with Egg).
- - `GAME: malloc: limit=16777216 waterline=229156 allocated=4108`

## Input
- [ ] "Joystick": Confirm standard-mappable devices report sensibly.
- - In browsers, a joystick is not visible until it first sends some event. That's by design, we can't change it.
- [ ] "Raw Joystick": Confirm sensible events arrive, with unique IDs for each button.
- - The actual button IDs will vary across runtimes, even from the same device, that's normal.
- [ ] "Keyboard": Hold multiple keys; they should stay on while held.
- [ ] "Keyboard": Confirm Shift+Letter does *not* change the reported keycode.
- [ ] "Keyboard": Confirm aux and modifier keys report just like any others.
- [ ] "Keyboard": Validate text on the bottom of screen. These *must* react to Shift etc.
- [ ] "Mouse": Coordinates must be for the framebuffer. NW=(0,0) SE=(320,180)
- [ ] "Mouse": Validate buttons left, middle, and right. If you have others, they should report too, as numbers.
- [ ] "Mouse": Natural wheel is +Y for down. Hold shift for X (left=up right=down).
- [ ] "Mouse Lock": Dragging mouse moves the camera. Confirm you can go forever in any direction.
- - This is currently broken sometimes, I need to investigate.
- [ ] "Mouse Lock": Cursor must hide.
- [ ] "Mouse Lock": Click to relinquish lock, return to prior menu, and show cursor again.
- [ ] "Touch": Draw lines with finger, if hardware supports it.
- [ ] "Accelerometer": TODO

## Video
- [ ] "Primitives": Red and Green squares overlap with some blending.
- [ ] "Primitives": Squiqqly line goes from white to black to red to transparent.
- [ ] "Primitives": Trapezoid's NW corner is extended leftward. NW=white NE=red SW=green SE=blue
- [ ] "Primitives": The third witch should be twice as large as the others, and rotating clockwise.
- [ ] "Primitives": The other three witches are identical.
- [ ] "Primitives": All figures must react to tint and alpha.
- [ ] "Transforms": All four figures in each column must be identical.
- [ ] "Too Many Sprites": Press right to increase sprite count. Should go well into the thousands without any trouble.
- [ ] "Intermediate Framebuffer": The two images are identical.

## Audio
- [ ] Play song zero to terminate current music.
- [ ] Song 4 has drums, the other two do not.
- [ ] Confirm song 4 repeats at the 96th beat, and the counter bottom-right returns to zero.
- [ ] Play the same song without "force", it must do nothing and proceed playing.
- [ ] Play the same song with "force", it must start over. Validate beat counter too.
- [ ] Play a song without "repeat", you must get silence at the end and the beat counter returns to zero.
- [ ] Confirm that "trim" reduces the level of sound effects. Trim zero must be silent.
- [ ] Pan should go from left (negative) to right (positive).
- - Currently not implemented.
- [ ] Force playhead changes the song in progress, to the specified beat.
- - Currently not implemented.

## Store
- [ ] "List Resources" shows the hundred or so resources in this archive, with believable sizes.
- [ ] "Strings by Language": Press left/right to view strings in English, French, and Russian.
- [ ] "Persistence": "List" logs the stored keys. Should be either empty or ["myStoredData"].
- [ ] "Persistence": Write, then relaunch and Read. Watch the log.

## Misc
- [ ] Real time and Game time are usually in sync. It's OK if they slip a little.
- [ ] The third line must show current local time, "YYYY-MM-DDTHH:MM:SS.mmm"
- [ ] "Langs" shows the languages your system is configured for, usually just one.
- [ ] Launch with "LANG=ru" or something in environment, confirm Langs shows up different.

## Regression
- [ ] Run each test. Flashes green for PASS, red for FAIL, gray for CHECK LOG.
- - Currently nothing useful here.
