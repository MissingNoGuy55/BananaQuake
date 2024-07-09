# BANANAQUAKE

BananaQuake is a port of Quake (and eventually QuakeWorld, QCC, etc.) to modern compilers and C++.

## What it has

Many global-scope systems ported to classes and allocated at launch.

Source-like memory system. Admittedly some of it was copied, so it will need to be replaced/reworked.

Many QoL fixes from QuakeSpasm.

## What it doesn't have

It is not very clean, and some of it is a total mess (looking at you, sound system)!

There are still many bugs. There are also some crashes here and there.

## CLONING

Just do `git clone --recurse-submodules https://github.com/MissingNoGuy55/BananaQuake` into the directory of your choice and run `make`.

## PREREQUISITES FOR BUILDING

For Windows, there are no prerequisites that I can think of to get it to build, besides having CMake.

For Linux, you will need the following if you are using the Makefile: `automake` for the configuration of the SDL submodules, `libxxf86dga-dev`, `libgl-dev` and `libglx-dev` (for X11); and `libmad0-dev` if you are to compile MP3 support which is there by default. You can edit the Makefile to suit your needs.

Otherwise, use CMake.

## Special thanks

Huge thanks to QuakeSpasm team for a ton of optimizations and useful code this source port uses that is under GPL.
