;==============================================================
; PSGlib library - by sverx
; https://github.com/sverx/PSGlib
;==============================================================

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
  jr z,+
  ld a,(PSGChan2LowTone)
  and $0F                        ; use only low 4 bits of byte
  or PSGLatch|PSGChannel2        ; latch channel 2, low part of tone
  out (PSGDataPort),a
  ld a,(PSGChan2HighTone)        ; high part of tone (latched channel 2, tone)
  and $3F                        ; use only low 6 bits of byte
  out (PSGDataPort),a
  ld a,(PSGChan2Volume)          ; restore music' channel 2 volume
  and $0F                        ; use only low 4 bits of byte
  or PSGLatch|PSGChannel2|PSGVolumeData
  out (PSGDataPort),a
+:ld a,(PSGChannel3SFX)          ; channel 3 playing?
  or a
  jr z,+
  ld a,(PSGChan3LowTone)
  and $07                        ; use only low 3 bits of byte
  or PSGLatch|PSGChannel3        ; latch channel 3, low part of tone (no high part)
  out (PSGDataPort),a
  ld a,(PSGChan3Volume)          ; restore music' channel 3 volume
  and $0F                        ; use only low 4 bits of byte
  or PSGLatch|PSGChannel3|PSGVolumeData
  out (PSGDataPort),a
+:xor a                          ; ld a,PSG_STOPPED
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
  ld a,(PSGMusicStatus)          ; check if we've got to play a tune
  or a
  ret z

  ld a,(PSGMusicSkipFrames)      ; check if we've got to skip frames
  or a
  jr nz,_skipFrame

_intLoop:
  ld hl,(PSGMusicPointer)        ; current address
  ld b,(hl)                      ; load PSG byte (in B)
  inc hl                         ; point to next byte
  ld a,(PSGMusicSubstringLen)    ; read substring len
  or a
  jr z,_continue                 ; check if it's 0 (we're not in a substring)
  dec a                          ; decrease len
  ld (PSGMusicSubstringLen),a    ; save len
  jr nz,_continue
  ld hl,(PSGMusicSubstringRetAddr)  ; substring is over, retrieve return address

_continue:
  ld (PSGMusicPointer),hl        ; save current address
  ld a,b                         ; copy PSG byte into A
  cp PSGLatch                    ; is it a latch?
  jr c,_noLatch                  ; if < $80 then it's NOT a latch
  ld (PSGMusicLastLatch),a       ; it's a latch - save it
  
  ; we've got the latch PSG byte both in A and in B
  ; and we have to check if the value should pass to PSG or not
  bit 6,a                        ; test if the latch it's for channels 0-1 or for 2-3
  jr z,_send2PSG_A               ; jump if it's for channels 0-1
  bit 4,a                        ; test if it's a volume
  jr z,_low_part_Tone            ; jump if tone data (it's the low part)
  bit 5,a                        ; test if volume it's for channel 2 or 3
  jr z,+                         ; jump for channel 2
  ld (PSGChan3Volume),a          ; save volume data
  ld a,(PSGChannel3SFX)          ; channel 3 free?
  or a
  jr z,_send2PSG
  jp _intLoop
+:ld (PSGChan2Volume),a          ; save volume data
  ld a,(PSGChannel2SFX)          ; channel 2 free?
  or a
  jr z,_send2PSG
  jp _intLoop
  
_low_part_Tone:
  ; we've got the latch PSG byte both in A and in B
  ; and we have to check if the value should pass to PSG or not
  bit 5,a                        ; test if tone it's for channel 2 or 3
  jr z,+                         ; jump if channel 2
  ld (PSGChan3LowTone),a         ; save tone LOW data
  ld a,(PSGChannel3SFX)          ; channel 3 free?
  or a
  jr z,_send2PSG
  jp _intLoop
+:ld (PSGChan2LowTone),a         ; save tone LOW data
  ld a,(PSGChannel2SFX)          ; channel 2 free?
  or a
  jr z,_send2PSG
  jp _intLoop
  
_skipFrame:
  dec a
  ld (PSGMusicSkipFrames),a
  ret

_noLatch:
  cp PSGData
  jr c,_command                  ; if < $40 then it's a command
  ; it's a data
  ld a,(PSGMusicLastLatch)       ; retrieve last latch
  jp _output_NoLatch

_command:
  cp PSGWait
  ret z                          ; frame done, no additional frames
  jr c,+                         ; other commands?
  and $07                        ; take only the last 3 bits for skip frames
  ld (PSGMusicSkipFrames),a
  ret                            ; frame done, with additional frames
+:cp PSGSubString
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

_send2PSG:
  ld a,b
_send2PSG_A:
  out (PSGDataPort),a              ; output the byte
  jp _intLoop

_output_NoLatch:
  ; we got the last latch in A and the PSG data in B
  ; and we have to check if the value should pass to PSG or not
  bit 6,a                        ; test if the latch it's for channels 0-1 or for 2-3
  jr z,_send2PSG                 ;    if it's for chn 0 or 1 then we've done!
  bit 4,a                        ; test if it's a volume
  jr z,_high_part_Tone           ; jump if tone data (it's the high part)
  bit 5,a                        ; test if volume it's for channel 2 or 3
  jr z,+
  ld a,b                         ; restore data in A
  ld (PSGChan3Volume),a          ; save volume data
  ld a,(PSGChannel3SFX)          ; channel 3 free?
  or a
  jr z,_send2PSG
  jp _intLoop
+:ld a,b                         ; restore data in A
  ld (PSGChan2Volume),a          ; save volume data
  ld a,(PSGChannel2SFX)          ; channel 2 free?
  or a
  jr z,_send2PSG
  jp _intLoop

_setLoopPoint:
  ld (PSGMusicLoopPoint),hl
  jp _intLoop

_musicLoop:
  ld a,(PSGLoopFlag)               ; looping requested?
  or a
  jp z,PSGStop                     ; No:stop it! (tail call optimization)
  ld hl,(PSGMusicLoopPoint)
  ld (PSGMusicPointer),hl
  jp _intLoop
  
_substring:
  sub (PSGSubString-4)                ; len is value - $08 + 4
  ld (PSGMusicSubstringLen),a         ; save len
  ld c,(hl)                           ; load substring address (offset)
  inc hl
  ld b,(hl)
  inc hl
  ld (PSGMusicSubstringRetAddr),hl    ; save return address
  ld hl,(PSGMusicStart)
  add hl,bc
  ld (PSGMusicPointer),hl             ; make substring current
  jp _intLoop
  
_high_part_Tone:
  ; we got the last latch in A and the PSG data in B
  ; and we have to check if the value should pass to PSG or not
  bit 5,a                        ; test if tone it's for channel 2 or 3
  jr z,+                         ; jump if channel 2
  ld a,b                         ; PSG data in A
  and $07                        ; keep last 3 bits of PSG data only
  or PSGLatch|PSGChannel3        ; set it to latch for channel 3
  ld (PSGChan3LowTone),a         ; save channel 3 tone LOW data (latched)
  ld a,(PSGChannel3SFX)          ; channel 3 free?
  or a
  jr z,_send2PSG
  jp _intLoop
+:ld a,b                         ; PSG data in A
  ld (PSGChan2HighTone),a        ; save channel 2 tone HIGH data
  ld a,(PSGChannel2SFX)          ; channel 2 free?
  or a
  jr z,_send2PSG
  jp _intLoop
.ends

.section "PSGSFXFrame" free
; ************************************************************************************
; processes a SFX frame
; destroys AF,HL,BC
PSGSFXFrame:
  ld a,(PSGSFXStatus)            ; check if we've got to play SFX
  or a
  ret z

  ld a,(PSGSFXSkipFrames)        ; check if we've got to skip frames
  or a
  jp nz,_skipFrame

_intLoop:
  ld hl,(PSGSFXPointer)          ; load next byte
  ld b,(hl)                      ; save it in B, temporary
  inc hl                         ; point to next byte
  ld a,(PSGSFXSubstringLen)      ; read substring len
  or a                           ; check if it's 0 (we're not in a substring)
  jr z,_continue
  dec a                          ; decrease len
  ld (PSGSFXSubstringLen),a      ; save len
  jr nz,_continue
  ld hl,(PSGSFXSubstringRetAddr) ; substring over, retrieve return address

_continue:
  ld (PSGSFXPointer),hl          ; save current address
  ld a,b                         ; restore byte
  cp PSGData
  jp c,_command                  ; if less than $40 then it's a command
  out (PSGDataPort),a            ; output the byte
  jp _intLoop
  
_skipFrame:
  dec a
  ld (PSGSFXSkipFrames),a
  ret

_command:
  cp PSGWait
  ret z                          ; frame done, no additional frames
  jr c,+                         ; other commands?
  and $07                        ; take only the last 3 bits for skip frames
  ld (PSGSFXSkipFrames),a
  ret                            ; frame done, with additional frames
+:cp PSGSubString
  jr nc,_substring
  cp PSGEnd
  jr z,_sfxLoop
  cp PSGLoop
  jr z,_setLoopPoint
  
  ; ***************************************************************************
  ; we should never get here!
  ; if we do, it means the PSG SFX file is probably corrupted, so we just RET
  ; ***************************************************************************

  ret

_setLoopPoint:
  ld (PSGSFXLoopPoint),hl
  jp _intLoop
  
_sfxLoop:
  ld a,(PSGSFXLoopFlag)               ; is it a looping SFX?
  or a
  jp z,PSGSFXStop                     ; No:stop it! (tail call optimization)
  ld hl,(PSGSFXLoopPoint)
  ld (PSGSFXPointer),hl
  jp _intLoop

_substring:
  sub (PSGSubString-4)                ; len is value - $08 + 4
  ld (PSGSFXSubstringLen),a           ; save len
  ld c,(hl)                           ; load substring address (offset)
  inc hl
  ld b,(hl)
  inc hl
  ld (PSGSFXSubstringRetAddr),hl    ; save return address
  ld hl,(PSGSFXStart)
  add hl,bc
  ld (PSGSFXPointer),hl             ; make substring current
  jp _intLoop
.ends

.ramsection "PSGlib variables" slot 3
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

  ; ******* SFX *************

  ; command buffering for channels 2-3
  PSGChan2Volume             db       ; the volume for channel 2
  PSGChan3Volume             db       ; the volume for channel 3
  PSGChan2LowTone            db       ; the low tone bits for channels 2
  PSGChan3LowTone            db       ; the low tone bits for channels 3
  PSGChan2HighTone           db       ; the high tone bits for channel 2

  ; flags for channels 2-3 access
  PSGChannel2SFX             db       ; !0 means channel 2 is allocated to SFX
  PSGChannel3SFX             db       ; !0 means channel 3 is allocated to SFX

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
