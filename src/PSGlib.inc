;==============================================================
; PSGlib library
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
; initialize PSG 'engine'
; destroys AF
PSGInit:
  xor a                           ; ld a,PSG_STOPPED
  ld (PSGMusicStatus),a           ; set music status to PSG_STOPPED
  ld (PSGSFXStatus),a             ; set SFX status to PSG_STOPPED
  ld (PSGChannel2SFX),a           ; set channel 2 SFX to PSG_STOPPED
  ld (PSGChannel3SFX),a           ; set channel 3 SFX to PSG_STOPPED
  ret
.ends

.section "PSGPlay" free
; ************************************************************************************
; receives in HL the address of the PSG to start playing
; destroys AF
PSGPlay:
  ld (PSGMusicStart),hl           ; store the begin point of music
  ld (PSGMusicPointer),hl         ; set music pointer to begin of music
  ld (PSGMusicLoopPoint),hl       ; looppointer points to begin too
  xor a
  ld (PSGMusicSkipFrames),a       ; reset the skip frames
  ld (PSGMusicSubstringLen),a     ; reset the substring len (for compression)
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

.section "PSGSFXPlay" free
; ************************************************************************************
; (both "PSGSFXPlay" and "PSGSFXPlayLoop")
; receive in HL the address of the SFX PSG to start
; receive in C the mask that indicates which channel(s) the SFX will use
; destroy AF
PSGSFXPlayLoop:
  ld a,$1
  ld (PSGSFXLoopFlag),a         ; SFX _IS_ a looping one
  jp +
PSGSFXPlay:
  xor a
  ld (PSGSFXLoopFlag),a         ; SFX is _NOT_ a looping one
+:ld (PSGSFXStart),hl           ; store begin of SFX
  ld (PSGSFXPointer),hl         ; set the pointer to begin of SFX
  ld (PSGSFXLoopPoint),hl       ; looppointer points to begin too
  xor a
  ld (PSGSFXSkipFrames),a       ; reset the skip frames
  ld (PSGSFXSubstringLen),a     ; reset the substring len
  bit 0,c                       ; channel 2 needed?
  jr z,+
  ld a,PSGLatch|PSGChannel2|PSGVolumeData|$0F   ; latch channel 2, volume=0xF (silent)
  out (PSGDataPort),a
+:bit 1,c                       ; channel 3 needed?
  jr z,+
  ld a,PSGLatch|PSGChannel3|PSGVolumeData|$0F   ; latch channel 3, volume=0xF (silent)
  out (PSGDataPort),a
+:ld a,PSG_PLAYING
  bit 0,c                       ; channel 2 used?
  jr z,+
  ld (PSGChannel2SFX),a
+:bit 1,c                       ; channel 3 used?
  jr z,+
  ld (PSGChannel3SFX),a
+:ld (PSGSFXStatus),a           ; set status to PSG_PLAYING
  ret
.ends

.section "PSGSFXStop" free
; ************************************************************************************
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
  jr z,+                         ; if it's not playing, skip restoring PSG values
  ld a,(PSGChan23LowTone)
  or PSGLatch|PSGChannel2        ; latch channel 2, low part of tone
  out (PSGDataPort),a
  ld a,(PSGChan2HighTone)        ; high part of tone (latched channel 2)
  out (PSGDataPort),a
  ld a,(PSGChan23Volume)         ; restore music' channel 2 volume
  or PSGLatch|PSGChannel2|PSGVolumeData
  out (PSGDataPort),a
  ld a,(PSGChan23LowTone+1)
  or PSGLatch|PSGChannel3        ; latch channel 3, low part of tone (no high part)
  out (PSGDataPort),a
  ld a,(PSGChan23Volume+1)       ; restore music' channel 3 volume
  or PSGLatch|PSGChannel3|PSGVolumeData
  out (PSGDataPort),a
+:xor a                          ; ld a,PSG_STOPPED
  ld (PSGChannel2SFX),a
  ld (PSGChannel3SFX),a
  ld (PSGSFXStatus),a            ; set status to PSG_STOPPED
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
  jp nz,_skipFrame

  ld a,(PSGMusicLastLatch)       ; in case that a playing SFX changed the current channel
  out (PSGDataPort),a

_intLoop:
  ld hl,(PSGMusicPointer)        ; load next byte
  ld b,(hl)                      ; save it in B, temporary
  inc hl                         ; point to next byte
  ld (PSGMusicPointer),hl
  
  ld a,(PSGMusicSubstringLen)    ; read substring len
  or a                           ; check if it's 0 (we're not in a substring)
  jp z,_continue
  dec a                             ; decrease len
  ld (PSGMusicSubstringLen),a       ; save len
  jp nz,_continue
  ld hl,(PSGMusicSubstringRetAddr)  ; substring is over, retrieve return address
  ld (PSGMusicPointer),hl

_continue:
  ld a,b                         ; restore byte
  cp PSGData
  jp c,_command                  ; if less than $40 then it's a command
  cp PSGLatch
  jp c,_data                     ; if less than $80 then it's not a latch
  bit 6,b                        ; test if the latch it's for channels 0-1 or for 2-3
  jp z,_send2PSG                 ; if it's for chn 0 or 1 then ok
  and $0F                        ; separate low 4 bits of byte
  bit 4,b                        ; test if it's a volume
  jp z,_freq                     ; jump if tone data
  ld hl,PSGChan23Volume
  bit 5,b                        ; test if volume it's for channel 3
  jp z,+
  inc hl                         ; it's for channel 3
  ld (hl),a                      ; save volume data
  ld a,(PSGChannel3SFX)          ; channel 3 free?
  or a
  jp z,_send2PSG_B
  jp _intLoop
+:ld (hl),a                      ; save volume data
  ld a,(PSGChannel2SFX)          ; channel 2 free?
  or a
  jp z,_send2PSG_B
  jp _intLoop
  
_skipFrame:
  dec a
  ld (PSGMusicSkipFrames),a
  ret

_freq:
  ld hl,PSGChan23LowTone
  bit 5,b                        ; test if tone it's for channel 3
  jp z,+
  inc hl
  ld (hl),a                      ; save tone data
  ld a,(PSGChannel3SFX)          ; channel 3 free?
  or a
  jp z,_send2PSG_B
  jp _intLoop
+:ld (hl),a                      ; save tone data
  ld a,(PSGChannel2SFX)          ; channel 2 free?
  or a
  jp z,_send2PSG_B
  jp _intLoop
  
_command:
  cp PSGWait
  ret z                          ; frame done, no additional frames
  jp c,_otherCommands
  and $07                        ; take only the last 3 bits
  ld (PSGMusicSkipFrames),a
  ret
  
_send2PSG_B:
  ld a,b
_send2PSG:
  ld hl,PSGMusicLastLatch
  call PSGINTplay_and_latch      ; output the byte and save data
  jp _intLoop

_data: 
  ld hl,PSGMusicLastLatch
  ld c,(hl)                      ; retrieve last latch command
  bit 6,c                        ; test if it's for channels 0-1 or for 2-3
  jp z,_send2PSG                 ; if it's for chn 0 or 1 we've done
  
  bit 4,c                        ; test if it's a volume
  jp z,_highfreq                 ; jump if tone data (high)

  and $0F                        ; separate low 4 bits of byte
  ld hl,PSGChan23Volume
  bit 5,c                        ; test if volume it's for channel 3
  jp z,+
  inc hl                         ; it's for channel 3
  ld (hl),a                      ; save volume data
  ld a,(PSGChannel3SFX)          ; channel 3 free?
  or a
  jp z,_send2PSG_B
  jp _intLoop
+:ld (hl),a                      ; save volume data
  ld a,(PSGChannel2SFX)          ; channel 2 free?
  or a
  jp z,_send2PSG_B
  jp _intLoop
  
_highfreq:
  ld hl,PSGChan2HighTone         ;
  bit 5,b                        ; test if tone it's for channel 3
  jp z,+
  dec hl                         ; if it's for chn 3 then it's low tone (so DEC)
  and $07                        ; separate low 3 bits of byte
  ld (hl),a                      ; save tone data
  ld a,(PSGChannel3SFX)          ; channel 3 free?
  or a
  jp z,_send2PSG_B
  jp _intLoop
+:ld (hl),a                      ; save tone data
  ld a,(PSGChannel2SFX)          ; channel 2 free?
  or a
  jp z,_send2PSG_B
  jp _intLoop

_otherCommands:
  cp PSGSubString
  jp p,_substring
  cp PSGEnd
  jp z,_gotoLoopPoint
  cp PSGLoop
  jp z,_setLoopPoint

  ; ***************************************************************************
  ; we should never get here!
  ; if we do, it means the PSG file is probably corrupted, so we just RET
  ; ***************************************************************************

  ret
  
_setLoopPoint:
  ld (PSGMusicLoopPoint),hl
  jp _intLoop

_gotoLoopPoint:
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

  ld a,(PSGSFXLastLatch)         ; in case that the playing music changed the current channel
  out (PSGDataPort),a

_intLoop:
  ld hl,(PSGSFXPointer)          ; load next byte
  ld b,(hl)                      ; save it in B, temporary
  inc hl                         ; point to next byte
  ld (PSGSFXPointer),hl
  
  ld a,(PSGSFXSubstringLen)      ; read substring len
  or a                           ; check if it's 0 (we're not in a substring)
  jp z,_continue
  dec a                          ; decrease len
  ld (PSGSFXSubstringLen),a      ; save len
  jp nz,_continue
  ld hl,(PSGSFXSubstringRetAddr) ; substring over, retrieve return address
  ld (PSGSFXPointer),hl

_continue:
  ld a,b                         ; restore byte
  cp PSGData
  jp c,_command                  ; if less than $40 then it's a command

_send2PSG:
  ld hl,PSGSFXLastLatch
  call PSGINTplay_and_latch      ; output the byte and save data
  jp _intLoop

_skipFrame:
  dec a
  ld (PSGSFXSkipFrames),a
  ret                            ; done frame processing!

_command:
  cp PSGWait
  ret z                          ; frame done, no additional frames
  jp c,_otherCommands
  and $07                        ; take only the last 3 bits
  ld (PSGSFXSkipFrames),a
  ret                            ; frame done

_otherCommands:
  cp PSGSubString
  jp p,_substring
  cp PSGEnd
  jp z,_sfxLoop
  cp PSGLoop
  jp z,_setLoopPoint

  ; ***************************************************************************
  ; we should never get here!
  ; if we do, it means the PSG file is probably corrupted, so we just RET
  ; ***************************************************************************

  ret
  
_setLoopPoint:
  ld (PSGSFXLoopPoint),hl
  jp _intLoop
  
_sfxLoop:
  ld a,(PSGSFXLoopFlag)
  or a
  jp z,PSGSFXStop                     ; tail call optimization
  ld hl,(PSGSFXLoopPoint)
  ld (PSGSFXPointer),hl
  jp _intLoop

_gotoLoopPoint:
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

  ret
.ends

; ************************************************************************************
;  'internal' functions
; ************************************************************************************

.section "PSG INT play and latch" free
; this plays the PSG byte in A
; and store a latch in the lastlatch pointed by HL
; destroys AF,BC
PSGINTplay_and_latch:
  out (PSGDataPort),a              ; output the byte
  cp PSGLatch
  jp c,_isData                     ; if it's not a latch, is a Data, go there
  ld (hl),a                        ; it's a latch, save it as it is
  ret                              ; done!
_isData:
  ld b,a                           ; save the data in B
  ld a,(hl)                        ; load lastlatch
  bit 4,a
  jp nz,_isTone                    ; if it's not a volume, is a Tone, go there
  and $F0                          ; it's a volume, clear lower 4 bits of lastlatch
  ld c,a                           ; save it temporary
  ld a,b                           ; restore the data
  and $0F                          ; clear upper 4 bits
  or c                             ; add the stored upper 4 bits
  ld (hl),a                        ; save the latch
  ret                              ; done!
_isTone:
  bit 5,a
  ret z                            ; it's not channel 3, leave!
  bit 6,a
  ret z                            ; it's not channel 3, leave!
  ld a,b                           ; restore the data
  and $08                          ; clear upper 5 bits
  or $E0                           ; set highest 3 bits (latch, chn 3)
  ld (hl),a                        ; save the latch
  ret                              ; done!
.ends

.ramsection "PSGPlay variables" slot 3
  ; fundamental vars
  PSGMusicStatus             db    ; are we playing a background music?
  PSGMusicStart              dw    ; the pointer to the beginning of music
  PSGMusicPointer            dw    ; the pointer to the current
  PSGMusicLoopPoint          dw    ; the pointer to the loop begin
  PSGMusicSkipFrames         db    ; the frames we need to skip

  ; decompression vars
  PSGMusicSubstringLen       db    ; lenght of the substring we are playing
  PSGMusicSubstringRetAddr   dw    ; return to this address when substring is over
  
  ; PSG music latch
  PSGMusicLastLatch          db    ; the last music latch that's needed for restoring

  ; ******* SFX *************

  ; command buffering for channels 2-3
  PSGChan23Volume            dsb   2  ; the volume for channels 2-3
  PSGChan23LowTone           dsb   2  ; the low tone bits for channels 2-3
  PSGChan2HighTone           db       ; the high tone bits for channel 2
  
  ; flags for channels 2-3 access
  PSGChannel2SFX             db       ; 1 means channel 2 is allocated to SFX
  PSGChannel3SFX             db       ; 1 means channel 3 is allocated to SFX

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

  ; PSG SFX latch
  PSGSFXLastLatch            db       ; the last SFX latch that's needed for restoring
.ends
