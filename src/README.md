PSGlib functions reference
==========================

engine initializer function
---------------------------

**PSGInit**: initializes the PSG engine
- no required parameters
- no return values
- destroys AF

functions for music
-------------------

**PSGFrame**: processes a music frame
- no required parameters
- no return values
- destroys AF,HL,BC

**PSGPlay** / **PSGPlayNoRepeat**: starts a tune (playing it repeatedly or only once)
- *needs* the address of the PSG to start playing in HL
- no return values
- destroys AF

**PSGStop**: stops (pauses) the music (leaving the SFX on, if one is playing)
- no required parameters
- no return values
- destroys AF

**PSGResume**: resume the previously stopped music
- no required parameters
- no return values
- destroys AF

**PSGCancelLoop**: sets the currently looping music to no more loops after the current
- no required parameters
- no return values
- destroys AF

**PSGGetStatus**: gets the current status of music into register A
- no required parameters
- *returns* `PSG_PLAYING` in register A the engine is playing music, `PSG_STOPPED` otherwise.

functions for SFX
-----------------

**PSGSFXFrame**: processes a SFX frame
- no required parameters
- no return values
- destroys AF,HL,BC

**PSGSFXPlay** / **PSGSFXPlayLoop**: starts a SFX (playing it once or repeatedly)
- *needs* the address of the SFX to start playing in HL
- *needs* a mask indicating which channels to be used by the SFX in C. The only possible values are `SFX_CHANNEL2`,`SFX_CHANNEL3` and `SFX_CHANNELS2AND3`.
- destroys AF

**PSGSFXStop**: stops the SFX (leaving the music on, if music is playing)
- no required parameters
- no return values
- destroys AF

**PSGSFXCancelLoop**: sets the currently looping SFX to no more loops after the current
- no required parameters
- no return values
- destroys AF

**PSGSFXGetStatus**: gets the current status of SFX into register A
- no required parameters
- *returns* `PSG_PLAYING` in register A if the engine is playing a SFX, `PSG_STOPPED` otherwise.

functions for music volume and hardware channels handling
---------------------------------------------------------

**PSGSetMusicVolumeAttenuation**: sets the volume attenuation for the music
- *needs* the volume attenuation value in L (valid value are 0-15 where 0 means no attenuation and 15 is complete silence)
- no return values
- destroys AF

**PSGSilenceChannels**: sets all hardware channels to volume ZERO (useful if you need to pause all audio)
- no required parameters
- no return values
- destroys AF

**PSGRestoreVolumes**: resets silenced hardware channels to previous volume
- no required parameters
- no return values
- destroys AF,HL

value of the defined constants
------------------------------

- `PSG_STOPPED`   0
- `PSG_PLAYING`   1

- `SFX_CHANNEL2`        $01
- `SFX_CHANNEL3`        $02
- `SFX_CHANNELS2AND3`   $03

