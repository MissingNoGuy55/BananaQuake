# BANANAQUAKE

BananaQuake is a port of Quake (and eventually QuakeWorld, QCC, etc.) to modern compilers and C++.

## What it has

Many global-scope systems ported to classes and allocated at launch.

Source-like memory system. Admittedly some of it was copied, so it will need to be replaced/reworked.

Many QoL fixes from QuakeSpasm.

## What it doesn't have

It is not very clean, and some of it is a total mess (looking at you, sound system)!

There are still many bugs, particularly with lighting in a select few levels like E3M1 on x64 and x86_64. There are also some crashes here and there.