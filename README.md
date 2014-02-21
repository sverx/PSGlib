PSGPlay
=======

Z80 ASM library (and C conversion/compression tools) to allow replay of VGMs as background music/SFX in SEGA 8 bit homebrew programs

Typical workflow:

1) You (or a friend of yours) track a/some module(s) and SFX(s) using either Mod2PSG2 or DefleMask (or whatever you prefer as long as it supports exporting in VGM format).

2) Convert the VGM(s) to PSG(s) file(s) using the vgm2psg tool.

3) Compress the PSG(s) file(s) using the psgcomp tool (it's an optional step) - the psguncomp tool can be used to check that the compression was right.

4) 'incbin' the file to your Z80 ASM source

5) Set up a steady interrupt (vBlank for instance) to call PSGFrame at a constant pace

6) Start/stop tunes when needed using PSGPlay and PSGStop calls, start/cancel SFXs when needed using PSGSFXPlay and PSGSFXStop calls. Looping SFXs are supported too, fire them using a PSGSFXPlayLoop call.

