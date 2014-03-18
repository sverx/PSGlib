;==============================================================
; defines
;==============================================================

.define VDPControlPort      $bf ; VDP Control Port (w/o)
.define writeVDPregister    $80 ; to write to VDP register
.define writeVRAM           $40 ; to write to VRAM

.define VDPStatusPort       $bf ; (r/o)
.define VDPVCounter         $7e ; (r/o)

.define TileMapAddress      ($3800 | (writeVRAM<<8)) ; ORed with $4000 for setting VRAM address
.define SpriteTableAddress  ($3f00 | (writeVRAM<<8))
.define SpriteSet           1   ; 0 for sprites to use tiles 0-255, 1 for 256-511

.define IOPort1             $dc ; (r/w)
.define P1_Btn1_bit         4
.define P1_Btn1             (1<<P1_Btn1_bit)

;==============================================================
; WLA-DX banking setup
;==============================================================
.memorymap
defaultslot 0
slotsize $4000 ; ROM
slot 0 $0000
slot 1 $4000
slot 2 $8000
slotsize $2000 ; RAM
slot 3 $c000
.endme

.ramsection "general variables" slot 3
  VBlankFlag   db   		; VBlank flag
  MaxVCount    db
.ends

.rombankmap
bankstotal 1
banksize $4000
banks 1
.endro

.bank 0 slot 0

.org 0                          ; this goes at ROM address 0 (boot) : standard startup
.section "Startup" force
di                              ; disable interrupt
im 1                            ; interrupt mode 1 (this won't change)
ld sp, $dff0                    ; set stack pointer at end of RAM
jp main                         ; run main()
.ends

.org $38
.section "Interrupt handler" force
  push af
    in a,(VDPStatusPort)        ; read port to satisfy interrupt
    ld a,1                      ; the only interrupt enabled is VBlank so...
    ld (VBlankFlag),a           ;   ... write down that it actually happened
  pop af
  ei                            ; enable interrupt (that were disabled by the IRQ call)
  reti                          ; return from interrupt
.ends

.section "waitForVBlank" free
waitForVBlank:
-:ld a,(VBlankFlag)
  or a
  jr z,-
  xor a
  ld (VBlankFlag),a
  ret
.ends

.section "setVideo - Set the video mode I need" free
; set 192 lines, 8x8 sprites, VBlank IRQ, enable display
setVideo:
  ld hl,_Data
  ld b,_End-_Data
  ld c,VDPControlPort
  otir
  ret
_Data:
    .db %00000110,writeVDPregister|$00
    ;    |||||||`- Disable synch
    ;    ||||||`-- Enable extra height modes
    ;    |||||`--- SMS mode instead of SG
    ;    ||||`---- Shift sprites left 8 pixels
    ;    |||`----- Enable line interrupts
    ;    ||`------ Blank leftmost column for scrolling
    ;    |`------- Fix top 2 rows during horizontal scrolling
    ;    `-------- Fix right 8 columns during vertical scrolling
    .db %01100000,writeVDPregister|$01
    ;     |`------ Enable VBlank interrupts
    ;     `------- Enable display
    .db (TileMapAddress>>10)   |%11110001,writeVDPregister|$02
    .db $ff,writeVDPregister|$03      ; For the SMS VDP only, all bits should be set.
    .db $ff,writeVDPregister|$04      ; For the SMS VDP only, all bits should be set.
    .db (SpriteTableAddress>>7)|%10000001,writeVDPregister|$05
    .db (SpriteSet<<2)         |%11111011,writeVDPregister|$06
    .db $0|$f0,writeVDPregister|$07
    ;    `-------- Border palette colour (sprite palette)
    .db $00,writeVDPregister|$08
    ;    ``------- Horizontal scroll
    .db $00,writeVDPregister|$09
    ;    ``------- Vertical scroll
    .db $ff,writeVDPregister|$0a
    ;    ``------- Line interrupt spacing ($ff mean disable)
_End:
.ends

.include "PSGlib.inc"          ; include the library

.section "main" free
main:

  ; ******************** Init lib ********************
  call PSGInit                  ; better do that before activating interrupts ;)
  ei
  call setVideo

  ; ******************** start the tune ********************
  ld hl,theMusic
  call PSGPlay

  ld d,$00
  ld a,192
  ld (MaxVCount),a              ; init vcount at 192

  .rept 5
  call waitForVBlank            ; skip few frames (emulators will thank you)
  .endr

_intLoop:
  call waitForVBlank            ; wait for vertical blanking
  call PSGFrame			; process next music frame
  ; here your can do bank switching if needed
  call PSGSFXFrame              ; process next SFX frame

  in a,(VDPVCounter)
  ld hl,MaxVCount
  cp (hl)
  jp m,+
  ld (hl),a
+:

  in a,(IOPort1)
  cpl
  and P1_Btn1
  ld b,a                        ; save value into B
  jr z,_stop
  xor d
  jr z,_end
  
  ; ******************** fire a SFX ********************
  ld hl,theSFX
  ld c,SFX_CHANNELS2AND3
  call PSGSFXPlay


  jr _end
_stop:
  xor d
  jr z,_end

  ; ******************** cancel a SFX ********************
  call PSGSFXStop

_end:
  ld d,b                        ; save original value
  jp _intLoop

  call PSGStop                  ; we won't never get here, actually...

.ends


;.bank 1 slot 1
.section "assets" free

theMusic:
.incbin "theMusic.psg"

theSFX:
.incbin "theSFX.psg"

.ends

; no sdsc tag in a ROM smaller than 32KB
;.sdsctag 0.01,"PSGtest","build number 201402##","sverx"
