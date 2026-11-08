; =========================================================================
; Pixel Open — démarrage, interruption NMI et primitives matérielles.
; Assembleur ca65. Les fonctions exportées en "_nom" sont appelables en C
; (convention fastcall cc65 : dernier argument dans A/X).
; =========================================================================

.export __STARTUP__ : absolute = 1
.export _oam_buf, _vram_buf, _vram_len
.export _frame_cnt, _nmi_ready, _pal_buf, _pal_dirty, _ppu_mask
.export _pad1, _pad2, _pad1_new, _pad2_new, _region_cnt
.export _wait_frame, _pad_poll, _ppu_off, _ppu_on, _vram_adr, _vram_put
.export _vram_fill, _vram_unrle, _rand8, _rand_seed
.export _spr1, _spr_meta, _spr_end, _oam_i
.export _meta_x, _meta_y, _meta_tile, _meta_attr
.import _main, zerobss, copydata
.import __CSTACK_START__, __CSTACK_SIZE__
.importzp sp, ptr1, tmp1, tmp2

PPUCTRL   = $2000
PPUMASK   = $2001
PPUSTATUS = $2002
OAMADDR   = $2003
PPUSCROLL = $2005
PPUADDR   = $2006
PPUDATA   = $2007
OAMDMA    = $4014
JOY1      = $4016
JOY2      = $4017

; PPUCTRL : NMI activée, sprites en table $1000, fond en table $0000
CTRL_VAL  = %10001000

.segment "HEADER"
    .byte "NES", $1A
    .byte 2             ; 2 x 16 Ko PRG
    .byte 1             ; 1 x 8 Ko CHR
    .byte $01           ; mapper 0, miroir vertical
    .byte $00
    .res 8, 0

.segment "OAM"
_oam_buf:   .res 256

.segment "VRAMBUF"
; Tampon de mises à jour VRAM : [hi, lo, len, données...]..., $FF
_vram_buf:  .res 128

.segment "ZEROPAGE"
_frame_cnt: .res 1
_nmi_ready: .res 1      ; 1 = le thread principal a fini la trame
_pal_dirty: .res 1
_ppu_mask:  .res 1
_pad1:      .res 1
_pad2:      .res 1
_pad1_new:  .res 1
_pad2_new:  .res 1
_vram_len:  .res 1
_rand_seed: .res 2
pad_tmp:    .res 1
_oam_i:     .res 1      ; prochain emplacement libre dans _oam_buf
_meta_x:    .res 1      ; paramètres de _spr_meta (personnage 2x3 tuiles)
_meta_y:    .res 1
_meta_tile: .res 1
_meta_attr: .res 1      ; bit 6 = miroir horizontal
_region_cnt:.res 2      ; nombre de boucles entre deux VBlank (détection NTSC/PAL)

.segment "BSS"
_pal_buf:   .res 32

.segment "STARTUP"
reset:
    sei
    cld
    ldx #$40
    stx JOY2            ; désactive l'IRQ de trame de l'APU
    ldx #$FF
    txs
    inx                 ; X = 0
    stx PPUCTRL
    stx PPUMASK
    stx $4010           ; pas d'IRQ DMC
    bit PPUSTATUS
@vb1:
    bit PPUSTATUS
    bpl @vb1
    ; efface la RAM
    txa
@clr:
    sta $0000,x
    sta $0100,x
    sta $0300,x
    sta $0400,x
    sta $0500,x
    sta $0600,x
    sta $0700,x
    inx
    bne @clr
    lda #$FF
@clroam:
    sta $0200,x         ; sprites hors écran
    inx
    bne @clroam
@vb2:
    bit PPUSTATUS
    bpl @vb2

    ; --- détection de la région : boucle de 16 cycles jusqu'au VBlank suivant
    ldx #0
    ldy #0
@cnt:
    inx                 ; 2
    bne @noc            ; 3 (2+1 si non pris)
    iny
@noc:
    nop                 ; 2
    nop                 ; 2
    bit PPUSTATUS       ; 4
    bpl @cnt            ; 3  => ~16 cycles
    stx _region_cnt
    sty _region_cnt+1

    ; APU : canaux carrés, triangle et bruit activés
    lda #$0F
    sta $4015

    ; initialisation de l'environnement C
    lda #<(__CSTACK_START__ + __CSTACK_SIZE__)
    sta sp
    lda #>(__CSTACK_START__ + __CSTACK_SIZE__)
    sta sp+1
    jsr zerobss
    jsr copydata
    lda #$A5
    sta _rand_seed
    lda #$3C
    sta _rand_seed+1
    lda #$FF
    sta _vram_buf
    lda #CTRL_VAL
    sta PPUCTRL
    jmp _main

; -------------------------------------------------------------------------
; NMI : DMA des sprites, mises à jour VRAM, palette, défilement.
; Ne fait rien tant que le thread principal n'a pas signalé une trame prête,
; ce qui évite d'envoyer un état à moitié construit au PPU.
; -------------------------------------------------------------------------
nmi:
    pha
    txa
    pha
    tya
    pha
    lda _nmi_ready
    beq @skip
    lda _ppu_mask
    and #$18
    beq @skip           ; rendu coupé : la VRAM appartient au thread principal

    lda #0
    sta OAMADDR
    lda #>_oam_buf
    sta OAMDMA          ; 513 cycles

    ldx #0
@upd:
    lda _vram_buf,x
    cmp #$FF
    beq @upd_end
    sta PPUADDR
    lda _vram_buf+1,x
    sta PPUADDR
    ldy _vram_buf+2,x
    inx
    inx
    inx
@cpy:
    lda _vram_buf,x
    sta PPUDATA
    inx
    dey
    bne @cpy
    beq @upd
@upd_end:
    lda #$FF
    sta _vram_buf
    lda #0
    sta _vram_len

    lda _pal_dirty
    beq @nopal
    lda #$3F
    sta PPUADDR
    lda #$00
    sta PPUADDR
    ldx #0
@pal:
    lda _pal_buf,x
    sta PPUDATA
    inx
    cpx #32
    bne @pal
    lda #0
    sta _pal_dirty
@nopal:
    lda #0
    sta PPUADDR
    sta PPUADDR
    sta PPUSCROLL
    sta PPUSCROLL
    lda #CTRL_VAL
    sta PPUCTRL
    lda _ppu_mask
    sta PPUMASK
    lda #0
    sta _nmi_ready
@skip:
    inc _frame_cnt
    pla
    tay
    pla
    tax
    pla
irq:
    rti

; void wait_frame(void) : publie la trame et attend la NMI suivante
_wait_frame:
    lda #1
    sta _nmi_ready
    lda _frame_cnt
@w:
    cmp _frame_cnt
    beq @w
    lda #0
    sta _nmi_ready
    rts

; void ppu_off(void) : coupe le rendu (attend un VBlank)
_ppu_off:
    lda #0
    sta _ppu_mask
    jsr _wait_frame
    lda #0
    sta PPUMASK
    rts

; void ppu_on(void) : active fond + sprites (y compris colonne de gauche)
_ppu_on:
    lda #%00011110
    sta _ppu_mask
    lda #CTRL_VAL
    sta PPUCTRL
    ; la palette sera envoyée par la NMI ; on force aussi l'envoi maintenant
    lda #$3F
    sta PPUADDR
    lda #$00
    sta PPUADDR
    ldx #0
@pal:
    lda _pal_buf,x
    sta PPUDATA
    inx
    cpx #32
    bne @pal
    lda #0
    sta PPUADDR
    sta PPUADDR
    sta PPUSCROLL
    sta PPUSCROLL
    jmp _wait_frame

; void vram_adr(unsigned int adr) — rendu coupé uniquement
_vram_adr:
    bit PPUSTATUS
    stx PPUADDR
    sta PPUADDR
    rts

; void vram_put(unsigned char v) — rendu coupé uniquement
_vram_put:
    sta PPUDATA
    rts

; void vram_fill(unsigned char v, unsigned int n) — rendu coupé uniquement
; fastcall : n dans A/X, v sur la pile C
_vram_fill:
    sta tmp1            ; n lo
    stx tmp2            ; n hi
    ldy #0
    lda (sp),y          ; v
    tay
    inc sp
    bne :+
    inc sp+1
:   tya
    ldx tmp1
    beq @hi
@lo:
    sta PPUDATA
    dex
    bne @lo
@hi:
    ldx tmp2
    beq @done
@pg:
    ldy #0
@pg2:
    sta PPUDATA
    dey
    bne @pg2
    dex
    bne @pg
@done:
    rts

; void vram_unrle(const unsigned char *data) — décompresse vers PPUDATA
; format : [MARK] puis octets ; MARK,v,n = v répété n fois ; MARK,x,0 = fin
_vram_unrle:
    sta ptr1
    stx ptr1+1
    ldy #0
    lda (ptr1),y
    sta tmp1            ; marqueur
    jsr @next
@loop:
    lda (ptr1),y
    cmp tmp1
    beq @run
    sta PPUDATA
    jsr @next
    jmp @loop
@run:
    jsr @next
    lda (ptr1),y
    sta tmp2            ; valeur
    jsr @next
    lda (ptr1),y        ; compte
    beq @end
    tax
    jsr @next
    lda tmp2
@rep:
    sta PPUDATA
    dex
    bne @rep
    jmp @loop
@end:
    rts
@next:
    inc ptr1
    bne :+
    inc ptr1+1
:   rts

; void pad_poll(void) : lit les deux manettes (bits A B Sel St H B G D)
_pad_poll:
    lda _pad1
    sta pad_tmp
    jsr read_pads
    lda pad_tmp
    eor #$FF
    and _pad1
    sta _pad1_new
    lda _pad2
    pha
    pla
    rts

read_pads:
    ; lecture double pour se prémunir d'éventuels conflits de lecture
    lda _pad2
    sta tmp2
    lda #1
    sta JOY1
    sta _pad2           ; bit sentinelle
    lsr a
    sta JOY1
@l:
    lda JOY1
    and #$03
    cmp #1
    rol _pad1
    lda JOY2
    and #$03
    cmp #1
    rol _pad2
    bcc @l
    lda tmp2
    eor #$FF
    and _pad2
    sta _pad2_new
    rts

; -------------------------------------------------------------------------
; Sprites (routines critiques : environ 10 fois plus rapides qu'en C)
; -------------------------------------------------------------------------
; void __fastcall__ spr1(u8 x, u8 y, u8 tile, u8 attr)
; A = attr ; pile C : tile (sp+0), y (sp+1), x (sp+2)
_spr1:
    ldx _oam_i
    sta _oam_buf+2,x
    ldy #0
    lda (sp),y
    sta _oam_buf+1,x
    iny
    lda (sp),y
    sec
    sbc #1
    sta _oam_buf,x
    iny
    lda (sp),y
    sta _oam_buf+3,x
    txa
    clc
    adc #4
    sta _oam_i
    lda sp
    clc
    adc #3
    sta sp
    bcc :+
    inc sp+1
:   rts

; void spr_meta(void) : personnage 16x24 (2 colonnes x 3 rangées de tuiles
; consécutives) en (_meta_x, _meta_y), tuile de base _meta_tile.
_spr_meta:
    ldx _oam_i
    ldy _meta_y
    dey                 ; le PPU affiche les sprites une ligne plus bas
    lda #3
    sta tmp1            ; rangées restantes
    lda _meta_tile
    sta tmp2
@row:
    tya
    sta _oam_buf,x
    sta _oam_buf+4,x
    lda _meta_attr
    sta _oam_buf+2,x
    sta _oam_buf+6,x
    and #$40
    bne @flip
    lda tmp2
    sta _oam_buf+1,x
    clc
    adc #1
    sta _oam_buf+5,x
    jmp @pos
@flip:
    lda tmp2
    sta _oam_buf+5,x
    clc
    adc #1
    sta _oam_buf+1,x
@pos:
    lda _meta_x
    sta _oam_buf+3,x
    clc
    adc #8
    sta _oam_buf+7,x
    inc tmp2
    inc tmp2
    tya
    clc
    adc #8
    tay
    txa
    clc
    adc #8
    tax
    dec tmp1
    bne @row
    stx _oam_i
    rts

; void spr_end(void) : cache les sprites inutilisés
_spr_end:
    ldx _oam_i
    lda #$FF
@h: sta _oam_buf,x
    inx
    inx
    inx
    inx
    bne @h
    rts

; unsigned char rand8(void) : LFSR 16 bits (Galois)
_rand8:
    ldx #8
    lda _rand_seed
@r:
    asl a
    rol _rand_seed+1
    bcc :+
    eor #$39
:   dex
    bne @r
    sta _rand_seed
    ldx #0
    rts

.segment "VECTORS"
    .word nmi, reset, irq

.segment "CHARS"
    .incbin "../assets/tiles.chr"
