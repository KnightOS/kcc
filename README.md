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

## Help, Bugs, Feedback

If you need help with KnightOS, want to keep up with progress, chat with
developers, or ask any other questions about KnightOS, you can hang out in the
IRC channel: [#knightos on irc.freenode.net](http://webchat.freenode.net/?channels=knightos).
 
To report bugs, please create [a GitHub issue](https://github.com/KnightOS/KnightOS/issues/new) or contact us on IRC.
 
If you'd like to contribute to the project, please see the [contribution guidelines](http://www.knightos.org/contributing).
