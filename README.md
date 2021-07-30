# Sprite Examples for Spectrum Next

Uses code from Stefan Bylund's [ZX Next Sprites library](https://github.com/stefanbylund/zxnext_sprite)

## Requirements

1. Make sure you have Z88dk installed and working
2. You'll need a working emulator / real hardware
3. The Makefile is set up to compile "main.c" only

## How to use

If it all worked, typing `make` in the source directory should at least compile
the code specified in `main.c` for you.

To compile the examples, go into the `src/` directory and *copy* one of the `.c` files to `main.c` and run `make`.

The resulting `.nex` file is available in the `build/` directory. If you do
a `make install` it will get moved into a `bin/` directory.

Copy the `.nex` file onto your SD card / emulator along with any data files needed and run.

## Examples

* src/animation.c - How to do very rudimentary animation
* src/composite.c - Composite sprite example
* src/main.c - Copy an example to this file to compile it
* src/multiple-sprites.c - How to have multiple sprites
* src/rotation.c - Rotation example
* src/scaling.c - 2x 4x and 8x scaling example
* src/simple-sprites.c - How to get a sprite on the screen
