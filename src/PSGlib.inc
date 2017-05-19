;================================================================
; PSGlib - Programmable Sound Generator audio library - by sverx
;          https://github.com/sverx/PSGlib
;================================================================

; NOTE: this uses a WLA-DX 'ramsection' at slot 3
;       If you want to change or remove that,
;       see the note at the end of this file

.define      PSG_STOPPED         0
.define      PSG_PLAYING         1

.define      PSGDataPort         $7f

.define      PSGLatch            $80
.define      PSGData             $40

.define      PSGChannel0         %00000000
.define      PSGChannel1         %00100000
.define      PSGChannel2         %01000000
.define      PSGChannel3         %01100000
.define      PSGVolumeData       %00010000

.define      PSGWait             $38
.define      PSGSubString        $08
.define      PSGLoop             $01
.define      PSGEnd              $00

.define      SFX_CHANNEL2        $01
.define      SFX_CHANNEL3        $02
.define      SFX_CHANNELS2AND3   SFX_CHANNEL2|SFX_CHANNEL3

.section "PSGInit" free
; ************************************************************************************
; initializes the PSG 'engine'
; destroys AF
PSGInit:
  xor a                           ; ld a,PSG_STOPPED
  ld (PSGMusicStatus),a           ; set music status to PSG_STOPPED
  ld (PSGSFXStatus),a             ; set SFX status to PSG_STOPPED
  ld (PSGChannel2SFX),a           ; set channel 2 SFX to PSG_STOPPED
  ld (PSGChannel3SFX),a           ; set channel 3 SFX to PSG_STOPPED
  ld (PSGMusicVolumeAttenuation),a  ; volume attenuation = none
  ret
.ends

.section "PSGPlay and PSGPlayNoRepeat" free
; ************************************************************************************
; receives in HL the address of the PSG to start playing
; destroys AF
PSGPlayNoRepeat:
  xor a                           ; We don't want the song to loop
  jp +
PSGPlay:
  ld a,$1                         ; the song can loop when finished
+:ld (PSGLoopFlag),a
  call PSGStop                    ; if there's a tune already playing, we should stop it!
  ld (PSGMusicStart),hl           ; store the begin point of music
  ld (PSGMusicPointer),hl         ; set music pointer to begin of music
  ld (PSGMusicLoopPoint),hl       ; looppointer points to begin too
  xor a
  ld (PSGMusicSkipFrames),a       ; reset the skip frames
  ld (PSGMusicSubstringLen),a     ; reset the substring len (for compression)
  ld a,PSGLatch|PSGChannel0|PSGVolumeData|$0F   ; latch channel 0, volume=0xF (silent)
  ld (PSGMusicLastLatch),a        ; reset last latch to chn 0 volume 0
  ld a,PSG_PLAYING
  ld (PSGMusicStatus),a           ; set status to PSG_PLAYING
  ret
.ends

.section "PSGStop" free
; ************************************************************************************
; stops the music (leaving the SFX on, if it's playing)
; destroys AF
PSGStop:
  ld a,(PSGMusicStatus)                         ; if it's already stopped, leave
  or a
  ret z
  ld a,PSGLatch|PSGChannel0|PSGVolumeData|$0F   ; latch channel 0, volume=0xF (silent)
  out (PSGDataPort),a
  ld a,PSGLatch|PSGChannel1|PSGVolumeData|$0F   ; latch channel 1, volume=0xF (silent)
  out (PSGDataPort),a
  ld a,(PSGChannel2SFX)
  or a
  jr nz,+
  ld a,PSGLatch|PSGChannel2|PSGVolumeData|$0F   ; latch channel 2, volume=0xF (silent)
  out (PSGDataPort),a
+:ld a,(PSGChannel3SFX)
  or a
  jr nz,+
  ld a,PSGLatch|PSGChannel3|PSGVolumeData|$0F   ; latch channel 3, volume=0xF (silent)
  out (PSGDataPort),a
+:xor a                                         ; ld a,PSG_STOPPED
  ld (PSGMusicStatus),a                         ; set status to PSG_STOPPED
  ret
.ends

.section "PSGResume" free
; ************************************************************************************
; resume a previously stopped music
; destroys AF
PSGResume:
  ld a,(PSGMusicStatus)                         ; if it's already playing, leave
  or a
  ret nz
  ld a,(PSGChan0Volume)                         ; restore channel 0 volume
  or PSGLatch|PSGChannel0|PSGVolumeData
  out (PSGDataPort),a
  ld a,(PSGChan1Volume)                         ; restore channel 1 volume
  or PSGLatch|PSGChannel1|PSGVolumeData
  out (PSGDataPort),a
  ld a,(PSGChannel2SFX)
  or a
  jr nz,+
  ld a,(PSGChan2LowTone)                        ; restore channel 2 frequency
  or PSGLatch|PSGChannel2
  out (PSGDataPort),a
  ld a,(PSGChan2HighTone)
  out (PSGDataPort),a
  ld a,(PSGChan2Volume)                         ; restore channel 2 volume
  or PSGLatch|PSGChannel2|PSGVolumeData
  out (PSGDataPort),a
+:ld a,(PSGChannel3SFX)
  or a
  jr nz,+
  ld a,(PSGChan3LowTone)                        ; restore channel 3 frequency
  or PSGLatch|PSGChannel3
  out (PSGDataPort),a
  ld a,(PSGChan3Volume)                         ; restore channel 3 volume
  or PSGLatch|PSGChannel2|PSGVolumeData
  out (PSGDataPort),a
+:ld a,PSG_PLAYING
  ld (PSGMusicStatus),a                         ; set status to PSG_PLAYING
  ret
.ends

.section "PSGCancelLoop" free
; ************************************************************************************
; sets the currently looping music to no more loops after the current
; destroys AF
PSGCancelLoop:
  xor a
  ld (PSGLoopFlag),a
  ret
.ends

.section "PSGGetStatus" free
; ************************************************************************************
; gets the current status of music into register A
PSGGetStatus:
  ld a,(PSGMusicStatus)
  ret
.ends

.section "PSGSetMusicVolumeAttenuation" free
; ************************************************************************************
; receives in L the volume attenuation for the music (0-15)
; destroys AF
PSGSetMusicVolumeAttenuation:
  ld a,l
  ld (PSGMusicVolumeAttenuation),a
  ld a,(PSGMusicStatus)            ; if tune is not playing, leave
  or a
  ret z

  ld a,(PSGChan0Volume)
  and $0F
  add a,l
  cp $0F                           ; check overflow
  jr c,+                           ; if it's <=15 then ok
  ld a,$0F                         ; else, reset to 15
+:or PSGLatch|PSGChannel0|PSGVolumeData
  out (PSGDataPort),a              ; output the byte
  
  ld a,(PSGChan1Volume)
  and $0F
  add a,l
  cp $0F                           ; check overflow
  jr c,+                           ; if it's <=15 then ok
  ld a,$0F                         ; else, reset to 15
+:or PSGLatch|PSGChannel1|PSGVolumeData
  out (PSGDataPort),a              ; output the byte

  ld a,(PSGChannel2SFX)            ; channel 2 busy with SFX?
  or a
  jr nz,_restore_channel3          ; if so, skip channel 2

  ld a,(PSGChan2Volume)
  and $0F
  add a,l
  cp $0F                           ; check overflow
  jr c,+                           ; if it's <=15 then ok
  ld a,$0F                         ; else, reset to 15
+:or PSGLatch|PSGChannel2|PSGVolumeData
  out (PSGDataPort),a              ; output the byte

_restore_channel3:
  ld a,(PSGChannel3SFX)            ; channel 3 busy with SFX?
  or a
  ret nz                           ; if so, we're done

  ld a,(PSGChan3Volume)
  and $0F
  add a,l
  cp $0F                           ; check overflow
  jr c,+                           ; if it's <=15 then ok
  ld a,$0F                         ; else, reset to 15
+:or PSGLatch|PSGChannel3|PSGVolumeData
  out (PSGDataPort),a              ; output the byte

  ret
.ends

.section "PSGSilenceChannels" free
; ************************************************************************************
; sets all PSG channels to volume ZERO (useful if you need to pause music)
; destroys AF
PSGSilenceChannels:
  ld a,PSGLatch|PSGChannel0|PSGVolumeData|$0F   ; latch channel 0, volume=0xF (silent)
  out (PSGDataPort),a
  ld a,PSGLatch|PSGChannel1|PSGVolumeData|$0F   ; latch channel 1, volume=0xF (silent)
  out (PSGDataPort),a
  ld a,PSGLatch|PSGChannel2|PSGVolumeData|$0F   ; latch channel 2, volume=0xF (silent)
  out (PSGDataPort),a
  ld a,PSGLatch|PSGChannel3|PSGVolumeData|$0F   ; latch channel 3, volume=0xF (silent)
  out (PSGDataPort),a
  ret
.ends

.section "PSGRestoreVolumes" free
; ************************************************************************************
; resets all PSG channels to previous volume
; destroys AF,HL
PSGRestoreVolumes:
  ld a,(PSGMusicStatus)            ; check if tune is playing
  or a
  jr z,_chkchn2                    ; if not, skip chn0 and chn1

  ld hl,PSGMusicVolumeAttenuation
  ld a,(PSGChan0Volume)
  and $0F
  add a,(hl)
  cp $0F                           ; check overflow
  jr c,+                           ; if it's <=15 then ok
  ld a,$0F                         ; else, reset to 15
+:or PSGLatch|PSGChannel0|PSGVolumeData
  out (PSGDataPort),a              ; output the byte

  ld a,(PSGChan1Volume)
  and $0F
  add a,(hl)
  cp $0F                           ; check overflow
  jr c,+                           ; if it's <=15 then ok
  ld a,$0F                         ; else, reset to 15
+:or PSGLatch|PSGChannel1|PSGVolumeData
  out (PSGDataPort),a              ; output the byte
  
_chkchn2:
  ld a,(PSGChannel2SFX)            ; channel 2 busy with SFX?
  jr nz,_restoreSFX2
  
  ld a,(PSGMusicStatus)            ; check if tune is playing
  or a
  jr z,_chkchn3                    ; if not, skip chn2

  ld a,(PSGChan2Volume)
  and $0F
  add a,(hl)
  cp $0F                           ; check overflow
  jr c,+                           ; if it's <=15 then ok
  ld a,$0F                         ; else, reset to 15
  jr c,+

_restoreSFX2:
  ld a,(PSGSFXChan2Volume)
  and $0F
+:or PSGLatch|PSGChannel2|PSGVolumeData
  out (PSGDataPort),a              ; output the byte

_chkchn3:
  ld a,(PSGChannel3SFX)            ; channel 3 busy with SFX?
  jr nz,_restoreSFX3
  
  ld a,(PSGMusicStatus)            ; check if tune is playing
  or a
  ret z                            ; if not, we've done

  ld a,(PSGChan3Volume)
  and $0F
  add a,(hl)
  cp $0F                           ; check overflow
  jr c,+                           ; if it's <=15 then ok
  ld a,$0F                         ; else, reset to 15
  jr c,+

_restoreSFX3:
  ld a,(PSGSFXChan3Volume)
  and $0F
+:or PSGLatch|PSGChannel3|PSGVolumeData
  out (PSGDataPort),a              ; output the byte

  ret
.ends

.section "PSGSFXPlay and PSGSFXPlayLoop" free
; ************************************************************************************
; receives in HL the address of the SFX PSG to start
; receives in C the mask that indicates which channel(s) the SFX will use
; destroys AF
PSGSFXPlayLoop:
  ld a,$1                       ; SFX _IS_ a looping one
  jp +
PSGSFXPlay:
  xor a                         ; SFX is _NOT_ a looping one
+:ld (PSGSFXLoopFlag),a
  call PSGSFXStop               ; if there's a SFX already playing, we should stop it!
  ld (PSGSFXStart),hl           ; store begin of SFX
  ld (PSGSFXPointer),hl         ; set the pointer to begin of SFX
  ld (PSGSFXLoopPoint),hl       ; looppointer points to begin too
  xor a
  ld (PSGSFXSkipFrames),a       ; reset the skip frames
  ld (PSGSFXSubstringLen),a     ; reset the substring len
  bit 0,c                       ; channel 2 needed?
  jr z,+
  ld a,PSG_PLAYING
  ld (PSGChannel2SFX),a
+:bit 1,c                       ; channel 3 needed?
  jr z,+
  ld a,PSG_PLAYING
  ld (PSGChannel3SFX),a
+:ld (PSGSFXStatus),a           ; set status to PSG_PLAYING
  ld a,(PSGChan3LowTone)        ; test if channel 3 uses the frequency of channel 2
  and 3
  cp 3
  ret nz                        ; if channel 3 doesn't use the frequency of channel 2 we're done
  ld a,PSG_PLAYING
  ld (PSGChannel3SFX),a         ; otherwise mark channel 3 as occupied by the SFX
  ld a,PSGLatch|PSGChannel3|PSGVolumeData|$0F   ; and silence channel 3
  out (PSGDataPort),a
  ret
.ends

.section "PSGSFXStop" free
; ************************************************************************************
; stops the SFX (leaving the music on, if it's playing)
; destroys AF
PSGSFXStop:
  ld a,(PSGSFXStatus)            ; check status
  or a
  ret z                          ; no SFX playing, leave
  ld a,(PSGChannel2SFX)          ; channel 2 playing?
  or a
  jr z,+
  ld a,PSGLatch|PSGChannel2|PSGVolumeData|$0F    ; latch channel 2, volume=0xF (silent)
  out (PSGDataPort),a
+:ld a,(PSGChannel3SFX)          ; channel 3 playing?
  or a
  jr z,+
  ld a,PSGLatch|PSGChannel3|PSGVolumeData|$0F    ; latch channel 3, volume=0xf (silent)
  out (PSGDataPort),a
+:ld a,(PSGMusicStatus)          ; check if a tune is playing
  or a
  jr z,_skipRestore              ; if it's not playing, skip restoring PSG values
  ld a,(PSGChannel2SFX)          ; channel 2 playing?
  or a
  jr z,_skip_chn2
  ld a,(PSGChan2LowTone)
  and $0F                        ; use only low 4 bits of byte
  or PSGLatch|PSGChannel2        ; latch channel 2, low part of tone
  out (PSGDataPort),a
  ld a,(PSGChan2HighTone)        ; high part of tone (latched channel 2, tone)
  and $3F                        ; use only low 6 bits of byte
  out (PSGDataPort),a
  ld a,(PSGChan2Volume)          ; restore music' channel 2 volume
  and $0F                        ; use only low 4 bits of byte
  push bc
    ld b,a                            ; save it temporary
    ld a,(PSGMusicVolumeAttenuation)  ;
    add a,b
    cp $0F                            ; check overflow
    jr c,+                            ; if it's <=15 then ok
    ld a,$0F                          ; else, reset to 15
+:  or PSGLatch|PSGChannel2|PSGVolumeData
    out (PSGDataPort),a              ; output the byte
  pop bc
_skip_chn2:
  ld a,(PSGChannel3SFX)          ; channel 3 playing?
  or a
  jr z,_skip_chn3
  ld a,(PSGChan3LowTone)
  and $07                        ; use only low 3 bits of byte
  or PSGLatch|PSGChannel3        ; latch channel 3, low part of tone (no high part)
  out (PSGDataPort),a
  ld a,(PSGChan3Volume)          ; restore music' channel 3 volume
  and $0F                        ; use only low 4 bits of byte
  push bc
    ld b,a                            ; save it temporary
    ld a,(PSGMusicVolumeAttenuation)  ;
    add a,b
    cp $0F                            ; check overflow
    jr c,+                            ; if it's <=15 then ok
    ld a,$0F                          ; else, reset to 15
+:  or PSGLatch|PSGChannel3|PSGVolumeData
    out (PSGDataPort),a              ; output the byte
  pop bc
_skip_chn3:
  xor a                          ; ld a,PSG_STOPPED
_skipRestore:
  ld (PSGChannel2SFX),a
  ld (PSGChannel3SFX),a
  ld (PSGSFXStatus),a            ; set status to PSG_STOPPED
  ret
.ends

.section "PSGSFXCancelLoop" free
; ************************************************************************************
; sets the currently looping SFX to no more loops after the current
; destroys AF
PSGSFXCancelLoop:
  xor a
  ld (PSGSFXLoopFlag),a
  ret
.ends

.section "PSGSFXGetStatus" free
; ************************************************************************************
; gets the current SFX status into register A
PSGSFXGetStatus:
  ld a,(PSGSFXStatus)
  ret
.ends

.section "PSGFrame" free
; ************************************************************************************
; processes a music frame
; destroys AF,HL,BC
PSGFrame:
  ld a,(PSGMusicStatus)          ; check if we have got to play a tune
  or a
  ret z

  ld a,(PSGMusicSkipFrames)      ; check if we havve got to skip frames
  or a
  jp nz,_skipFrame
  
  ld hl,(PSGMusicPointer)        ; read current address

_intLoop:
  ld b,(hl)                      ; load PSG byte (in B)
  inc hl                         ; point to next byte
  ld a,(PSGMusicSubstringLen)    ; read substring len
  or a
  jr z,_continue                 ; check if it is 0 (we are not in a substring)
  dec a                          ; decrease len
  ld (PSGMusicSubstringLen),a    ; save len
  jr nz,_continue
  ld hl,(PSGMusicSubstringRetAddr)  ; substring is over, retrieve return address

_continue:
  ld a,b                         ; copy PSG byte into A
  cp PSGLatch                    ; is it a latch?
  jr c,_noLatch                  ; if < $80 then it is NOT a latch
  ld (PSGMusicLastLatch),a       ; it is a latch - save it in "LastLatch"
  
  ; we have got the latch PSG byte both in A and in B
  ; and we have to check if the value should pass to PSG or not
  bit 4,a                        ; test if it is a volume
  jr nz,_latch_Volume            ; jump if volume data
  bit 6,a                        ; test if the latch it is for channels 0-1 or for 2-3
  jp z,_send2PSG_A               ; send data to PSG if it is for channels 0-1

  ; we have got the latch (tone, chn 2 or 3) PSG byte both in A and in B
  ; and we have to check if the value should be passed to PSG or not
  bit 5,a                        ; test if tone it is for channel 2 or 3
  jr z,_ifchn2                   ; jump if channel 2
  ld (PSGChan3LowTone),a         ; save tone LOW data
  ld a,(PSGChannel3SFX)          ; channel 3 free?
  or a
  jp nz,_intLoop
  ld a,(PSGChan3LowTone)
  and 3                          ; test if channel 3 is set to use the frequency of channel 2
  cp 3
  jr nz,_send2PSG_B              ; if channel 3 does not use frequency of channel 2 jump
  ld a,(PSGSFXStatus)            ; test if an SFX is playing
  or a
  jr z,_send2PSG_B               ; if no SFX is playing jump
  ld (PSGChannel3SFX),a          ; otherwise mark channel 3 as occupied
  ld a,PSGLatch|PSGChannel3|PSGVolumeData|$0F   ; and silence channel 3
  out (PSGDataPort),a
  jp _intLoop
_ifchn2:
  ld (PSGChan2LowTone),a         ; save tone LOW data
  ld a,(PSGChannel2SFX)          ; channel 2 free?
  or a
  jr z,_send2PSG_B
  jp _intLoop
  
_latch_Volume:
  bit 6,a                        ; test if the latch it is for channels 0-1 or for 2-3
  jr nz,_latch_Volume_23         ; volume is for channel 2 or 3
  bit 5,a                        ; test if volume it is for channel 0 or 1
  jr z,_ifchn0                   ; jump for channel 0
  ld (PSGChan1Volume),a          ; save volume data
  jp _sendVolume2PSG_A
_ifchn0:
  ld (PSGChan0Volume),a          ; save volume data
  jp _sendVolume2PSG_A

_latch_Volume_23:
  bit 5,a                        ; test if volume it is for channel 2 or 3
  jr z,_chn2                     ; jump for channel 2
  ld (PSGChan3Volume),a          ; save volume data
  ld a,(PSGChannel3SFX)          ; channel 3 free?
  or a
  jr z,_sendVolume2PSG_B
  jp _intLoop
_chn2:
  ld (PSGChan2Volume),a          ; save volume data
  ld a,(PSGChannel2SFX)          ; channel 2 free?
  or a
  jr z,_sendVolume2PSG_B
  jp _intLoop
  
_skipFrame:
  dec a
  ld (PSGMusicSkipFrames),a
  ret

_noLatch:
  cp PSGData
  jr c,_command                  ; if < $40 then it is a command
  ; it's a data
  ld a,(PSGMusicLastLatch)       ; retrieve last latch
  jp _output_NoLatch

_command:
  cp PSGWait
  jr z,_done                     ; no additional frames
  jr c,_otherCommands            ; other commands?
  and $07                        ; take only the last 3 bits for skip frames
  ld (PSGMusicSkipFrames),a      ; we got additional frames
_done:
  ld (PSGMusicPointer),hl        ; save current address
  ret                            ; frame done

_otherCommands:
  cp PSGSubString
  jr nc,_substring
  cp PSGEnd
  jr z,_musicLoop
  cp PSGLoop
  jr z,_setLoopPoint

  ; ***************************************************************************
  ; we should never get here!
  ; if we do, it means the PSG file is probably corrupted, so we just RET
  ; ***************************************************************************

  ret

_send2PSG_B:
  ld a,b
_send2PSG_A:
  out (PSGDataPort),a              ; output the byte
  jp _intLoop

_sendVolume2PSG_B:
  ld a,b
_sendVolume2PSG_A:
  ld c,a                           ; save the PSG command byte
  and $0F                          ; keep lower nibble
  ld b,a                           ; save value
  ld a,(PSGMusicVolumeAttenuation) ; load volume attenuation
  add a,b                          ; add value
  cp $0F                           ; check overflow
  jr c,_no_overflow                ; if it is <=15 then ok
  ld a,$0F                         ; else, reset to 15
_no_overflow:
  ld b,a                           ; save new attenuated volume value
  ld a,c                           ; retrieve PSG command
  and $F0                          ; keep upper nibble
  or b                             ; set attenuated volume
  out (PSGDataPort),a              ; output the byte
  jp _intLoop

_output_NoLatch:
  ; we got the last latch in A and the PSG data in B
  ; and we have to check if the value should pass to PSG or not
  ; note that non-latch commands can be only contain frequencies (no volumes)
  ; for channels 0,1,2 only (no noise)
  bit 6,a                        ; test if the latch it is for channels 0-1 or for chn 2
  jr nz,_high_part_Tone          ;  it is tone data for channel 2
  jp _send2PSG_B                 ; otherwise, it is for chn 0 or 1 so we have done!

_setLoopPoint:
  ld (PSGMusicLoopPoint),hl
  jp _intLoop

_musicLoop:
  ld a,(PSGLoopFlag)               ; looping requested?
  or a
  jp z,PSGStop                     ; No:stop it! (tail call optimization)
  ld hl,(PSGMusicLoopPoint)
  jp _intLoop

_substring:
  sub PSGSubString-4                  ; len is value - $08 + 4
  ld (PSGMusicSubstringLen),a         ; save len
  ld c,(hl)                           ; load substring address (offset)
  inc hl
  ld b,(hl)
  inc hl
  ld (PSGMusicSubstringRetAddr),hl    ; save return address
  ld hl,(PSGMusicStart)
  add hl,bc                           ; make substring current
  jp _intLoop

_high_part_Tone:
  ; we got the last latch in A and the PSG data in B
  ; and we have to check if the value should pass to PSG or not
  ; PSG data can only be for channel 2, here
  ld a,b                              ; move PSG data in A
  ld (PSGChan2HighTone),a             ; save channel 2 tone HIGH data
  ld a,(PSGChannel2SFX)               ; channel 2 free?
  or a
  jr z,_send2PSG_B
  jp _intLoop
.ends

.section "PSGSFXFrame" free
; ************************************************************************************
; processes a SFX frame
; destroys AF,HL,BC
PSGSFXFrame:
  ld a,(PSGSFXStatus)            ; check if we have got to play SFX
  or a
  ret z

  ld a,(PSGSFXSkipFrames)        ; check if we have got to skip frames
  or a
  jp nz,_skipSFXFrame
  
  ld hl,(PSGSFXPointer)          ; read current SFX address

_intSFXLoop:
  ld b,(hl)                      ; load a byte in B, temporary
  inc hl                         ; point to next byte
  ld a,(PSGSFXSubstringLen)      ; read substring len
  or a                           ; check if it is 0 (we are not in a substring)
  jr z,_SFXcontinue
  dec a                          ; decrease len
  ld (PSGSFXSubstringLen),a      ; save len
  jr nz,_SFXcontinue
  ld hl,(PSGSFXSubstringRetAddr) ; substring over, retrieve return address

_SFXcontinue:
  ld a,b                         ; restore byte
  cp PSGData
  jp c,_SFXcommand               ; if less than $40 then it is a command
  bit 4,a                        ; check if it is a volume byte
  jr z,_SFXoutbyte               ; if not, output it
  bit 5,a                        ; check if it is volume for channel 2 or channel 3
  jr nz,_SFXvolumechn3
  ld (PSGSFXChan2Volume),a
  jr _SFXoutbyte

_SFXvolumechn3:
  ld (PSGSFXChan3Volume),a

_SFXoutbyte:
  out (PSGDataPort),a            ; output the byte
  jp _intSFXLoop
  
_skipSFXFrame:
  dec a
  ld (PSGSFXSkipFrames),a
  ret

_SFXcommand:
  cp PSGWait
  jr z,_SFXdone                  ; no additional frames
  jr c,_SFXotherCommands         ; other commands?
  and $07                        ; take only the last 3 bits for skip frames
  ld (PSGSFXSkipFrames),a        ; we got additional frames to skip
_SFXdone:
  ld (PSGSFXPointer),hl          ; save current address
  ret                            ; frame done

_SFXotherCommands:
  cp PSGSubString
  jr nc,_SFXsubstring
  cp PSGEnd
  jr z,_sfxLoop
  cp PSGLoop
  jr z,_SFXsetLoopPoint
  
  ; ***************************************************************************
  ; we should never get here!
  ; if we do, it means the PSG SFX file is probably corrupted, so we just RET
  ; ***************************************************************************

  ret

_SFXsetLoopPoint:
  ld (PSGSFXLoopPoint),hl
  jp _intSFXLoop
  
_sfxLoop:
  ld a,(PSGSFXLoopFlag)               ; is it a looping SFX?
  or a
  jp z,PSGSFXStop                     ; No:stop it! (tail call optimization)
  ld hl,(PSGSFXLoopPoint)
  ld (PSGSFXPointer),hl
  jp _intSFXLoop

_SFXsubstring:
  sub PSGSubString-4                  ; len is value - $08 + 4
  ld (PSGSFXSubstringLen),a           ; save len
  ld c,(hl)                           ; load substring address (offset)
  inc hl
  ld b,(hl)
  inc hl
  ld (PSGSFXSubstringRetAddr),hl    ; save return address
  ld hl,(PSGSFXStart)
  add hl,bc                         ; make substring current
  jp _intSFXLoop
.ends

; NOTE: if you don't want to use a ramsection,
;       comment the ".ramsection" line and
;       uncomment the next ".enum" one,
;       setting .enum start RAM address
;       according to your needs.
;       Also change ".ends" into ".ende" at end of this file

.ramsection "PSGlib variables" slot 3
;.enum $c000 export                ; PSGlib variables location in RAM
  ; fundamental vars
  PSGMusicStatus             db    ; are we playing a background music?
  PSGMusicStart              dw    ; the pointer to the beginning of music
  PSGMusicPointer            dw    ; the pointer to the current
  PSGMusicLoopPoint          dw    ; the pointer to the loop begin
  PSGMusicSkipFrames         db    ; the frames we need to skip
  PSGLoopFlag                db    ; the tune should loop or not (flag)
  PSGMusicLastLatch          db    ; the last PSG music latch

  ; decompression vars
  PSGMusicSubstringLen       db    ; lenght of the substring we are playing
  PSGMusicSubstringRetAddr   dw    ; return to this address when substring is over

  ; command buffers
  PSGChan0Volume             db       ; the volume for channel 0
  PSGChan1Volume             db       ; the volume for channel 1
  PSGChan2Volume             db       ; the volume for channel 2
  PSGChan3Volume             db       ; the volume for channel 3
  PSGChan2LowTone            db       ; the low tone bits for channels 2
  PSGChan3LowTone            db       ; the low tone bits for channels 3
  PSGChan2HighTone           db       ; the high tone bits for channel 2

  PSGMusicVolumeAttenuation  db       ; the volume attenuation applied to the tune (0-15)

  ; ******* SFX *************

  ; flags for channels 2-3 access
  PSGChannel2SFX             db       ; !0 means channel 2 is allocated to SFX
  PSGChannel3SFX             db       ; !0 means channel 3 is allocated to SFX

  ; command buffers for SFX
  PSGSFXChan2Volume          db       ; the volume for channel 2
  PSGSFXChan3Volume          db       ; the volume for channel 3

  ; fundamental vars for SFX
  PSGSFXStatus               db       ; are we playing a SFX?
  PSGSFXStart                dw       ; the pointer to the beginning of SFX
  PSGSFXPointer              dw       ; the pointer to the current address
  PSGSFXLoopPoint            dw       ; the pointer to the loop begin
  PSGSFXSkipFrames           db       ; the frames we need to skip
  PSGSFXLoopFlag             db       ; the SFX should loop or not (flag)

  ; decompression vars for SFX
  PSGSFXSubstringLen         db       ; lenght of the substring we are playing
  PSGSFXSubstringRetAddr     dw       ; return to this address when substring is over
.ends
;.ende                    ; in case you want to use .enum instead of .ramsection
