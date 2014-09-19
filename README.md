# KnightOS C Compiler

The KnightOS C Compiler (kcc) is a fork of [sdcc](http://sdcc.sourceforge.net/). It's a work in progress.

## Compiling

kcc depends on flex/bison and boost.

    cmake .
    make
    make install # as root

## Differences from SDCC

* KnightOS support
* kcc includes man pages
* Switched from autotools to cmake
* Dropped all targets but z80
* Removed unneccessary subsystems (like simulators)
* General clean up

## Why a fork?

We presented our needs upstream and they were not interested in them.
