# PCFX_Blox
Falling Blocks-type game rewritten in 'C' for PC-FX (based on PC Engine game written in Assembler in 1999)

## Background

This is a rewrite of the PCE_Blox game I wrote in assembler for the PC Engine back in 1998/1999, but this
version is written in 'C' for the PC-FX.

Although I have had many thoughts over the years about how to implement this in alternative ways, I have
tried to remain as faithful as possible to the original program's design, data layouts, variable names,
etc.  and make this as straightforward of a port as possible, so that people can compare the two works.

There are several reasons I undertook this rewrite:

 1) There is very little PC-FX homebrew software available, even though some of the tools exist, such as
a good 'C' compiler (gcc), a library (liberis) to interface with the lower-level hardware, and tools to
package the generated executables into bootable discs.

 2) The liberis library - however well-intentioned - wasn't sufficiently tested in real-world usage to
determine whether the provided API is an appropriate API for general-purpose game development work.
Through the creation of other programs, I have notied some weaknesses, and decided to take this opportunity
to explore some of them, with an eye to improving (or possibly replacing) them.

 3) Some of the required "non-compiler" tools (such as graphic data format converters) also didn't exist,
so this was an opportunity to identify and create some of those as well.

This proof-of-concept has concentrated solely on the HuC6270 video processor and its various facilites,
and perhaps touched upon joypad controller limitations. There was no sound in the original proof-of-concept
game.

There are vast opportunities for ports of software created for the PC Engine and SuperGrafx to the PC-FX,
so the logical starting point for that is to ensure that the analogous hardware (i.e. HuC6270, palette,
sound generator, joypad, CDROM, ADPCM) is the initial focus of these tools.

## Current State

 1) The Blox field of play is incomplete, though some aspects are approaching completion.  The code is
somewhat messy, as I have been required to include additional #define statements and other facilities
which should be part of the base liberis functions; this refactoring will take place once the game is
in a fully-working state.

 2) Several tools have been written to support the palette and graphic data conversions required. These can be
found at this (relatively) new repository:
[https://github.com/pcfx-devel/PC-FX_Programming_Tools](https://github.com/pcfx-devel/PC-FX_Programming_Tools)


