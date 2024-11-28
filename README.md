# WL22MID
Wario Land 2/3 (GB/GBC) to MIDI converter

This tool converts music (and sound effects) from Game Boy and Game Boy Color games using Kozue Ishikawa's sound engine to MIDI format, which is used in Wario Land 2 and 3 and other first-party titles including Tetris DX.

It works with ROM images. To use it, you must specify the name of the ROM followed by the number of the main bank containing the sound data (in hex).

Examples:
* WL22MID "Wario Land II (U) [C][!].gbc" 19
* WL22MID "Wario Land 3 (JU) (M2) [C][!].gbc" D
* WL22MID "Tetris DX (JU) [C][!].gbc" D

This tool was based on my own reverse-engineering of the sound engine, with little disassembly involved. Also included is the "prototype" TXT converter tool WL22TXT.

Supported games:
  * Game Boy Color Promotional Demo
  * Mobile Golf
  * Mobile Trainer
  * Nintendo Power Menu Program
  * Tetris DX
  * Wario Land II
  * Wario Land 3

## To do:
  * GBS file support
