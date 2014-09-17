# KnightOS C Compiler

The KnightOS C Compiler (kcc) is a fork of [sdcc](http://sdcc.sourceforge.net/).

Yes, we know it doesn't compile. Still working on gutting all the sdcc cruft.

## Compiling

kcc depends on flex/bison.

    cmake .
    make
    make install # as root

## Differences from SDCC

* KnightOS support
* Switched from autotools to cmake
* Dropped all targets but z80
* Removed unneccessary subsystems (like simulators)
* General clean up

## Why a fork?

We presented our needs upstream and they were not interested in them.
