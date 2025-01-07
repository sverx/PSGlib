PSGlib
======

Z80 ASM library (and C conversion/compression tools) to allow replay of SN76489 VGMs as background music/SFX in SEGA 8 bit homebrew programs

Typical workflow:

1) You (or a friend of yours) track one or more SN76489 module(s) and SFX(s) using either [Furnace](https://github.com/tildearrow/furnace), [DefleMask](https://www.deflemask.com), [Mod2PSG2](https://www.smspower.org/Music/Mod2PSG2) or VGM Music Maker or whatever other tracker/tool you prefer as long as it supports exporting in VGM format. SFXs can be also tracked using the abovementioned tools or generated using the amazing [sn_sfxr](https://harmlesslion.com/sn_sfxr/) sound effects generator, for instance.

2) Use Calindro's [PSGTool](http://www.smspower.org/forums/16925-PSGToolAVGMToPSGConvertor) to convert each VGM file into a PSG file. This actually saves you these steps:

  * Optimize your VGM using Maxim's VGMTool

  * Convert the VGM to PSG file(s) using PSGlib's vgm2psg tool.

  * Compress the PSG file(s) using PSGlib's psgcomp tool (PSGlib's psgdecomp tool can be used to verify that the compression was right)

3) include PSGlib.inc and 'incbin' all the PSG file(s) to your Z80 ASM source.

4) call PSGInit once somewhere near the beginning of your code.

5) Set up a steady interrupt (vertical blanking for instance) so to call PSGFrame and PSGSFXFrame at a constant pace (very important!). The two calls are separated so you can switch banks when done processing background music and need to process SFX.

6) Start and stop (pause) tunes when needed using PSGPlay and PSGStop calls, start and stop SFXs when needed using PSGSFXPlay and PSGSFXStop calls.

 * Stopped (paused) tunes can be unpaused using a PSGResume call.

 * Tunes can be set to run just once instead of endlessly using PSGPlayNoRepeat call, or to loop a given number of times only using PSGPlayLoops call.

 * Any looping tune can be told to have no more loops using PSGCancelLoop call at any time and it will stop when its end is reached.

 * Looping SFXs are supported too: fire them using a PSGSFXPlayLoop call, cancel their loop using a PSGSFXCancelLoop call.

 * To check if a tune is still playing use PSGGetStatus call, to check if a SFX is still playing use PSGSFXGetStatus call.

+) You can set the music 'master' volume using PSGSetMusicVolumeAttenuation, even when a tune it's playing (and this won't affect SFX volumes).

+) If you need to completely suspend all the audio, use PSGSilenceChannels and stop calling PSGFrame and PSGSFXFrame. When ready to resume, use PSGRestoreVolumes and start calling PSGFrame and PSGSFXFrame again at a constant pace.

[PSGlib complete function reference](src/README.md)

