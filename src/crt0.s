; =========================================================================
; Royal Martinot — démarrage, interruption NMI et primitives matérielles.
; Assembleur ca65. Les fonctions exportées en "_nom" sont appelables en C
; (convention fastcall cc65 : dernier argument dans A/X).
; =========================================================================

.export __STARTUP__ : absolute = 1
.export _oam_buf, _vram_buf, _vram_len
.export _frame_cnt, _nmi_ready, _pal_buf, _pal_dirty, _ppu_mask, _fade_lvl, _fade_skip
.import _audio_tick, _is_pal
.export _pad1, _pad2, _pad1_new, _pad2_new, _region_cnt
.export _wait_frame, _pad_poll, _ppu_off, _ppu_on, _vram_adr, _vram_put
.export _swap_put, _vram_fill, _vram_unrle, _rand8, _rand_seed
.export _scroll_y, _scroll_nt, _mirror_h, _scroll_x, _tz_bank, _tz_n, _tz_unpack, _tz_copy
.export _chr_data
.export _spr1, _spr_meta, _spr_end, _oam_i, _spr_rot, _set_bank, _cur_bank, _bank_call, _far_call, _rp_record
.import _bx, _by, _bz, _b_vis, _plx, _ply, _pose_cache, _flip_cache
.import _rp_buf, _rp_head, _rp_count, _swp_dst
.import __WBSS_RUN__, __WBSS_SIZE__
.export _meta_x, _meta_y, _meta_tile, _meta_attr, _meta_t1, _meta_t2
.import _main, zerobss, copydata
.import __CSTACK_START__, __CSTACK_SIZE__
.importzp sp, ptr1, ptr2, tmp1, tmp2

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
    .byte 16            ; 16 x 16 Ko PRG (256 Ko)
    .byte 0             ; 0 = CHR-RAM (8 Ko)
    .byte $12           ; mapper 1 (MMC1), PRG-RAM sauvegardée par pile
    .byte $00
    .byte $00           ; 8 Ko de PRG-RAM
    .res 7, 0

; Écriture série dans un registre MMC1 (5 bits, poids faible d'abord)
.macro MMC1_WRITE addr
    sta addr
    lsr a
    sta addr
    lsr a
    sta addr
    lsr a
    sta addr
    lsr a
    sta addr
.endmacro

.segment "OAM"
_oam_buf:   .res 256

.segment "VRAMBUF"
; Tampon de mises à jour VRAM : [hi, lo, len, données...]..., $FF
_vram_buf:  .res 128

.segment "ZEROPAGE"
_frame_cnt: .res 1
_nmi_ready: .res 1      ; 1 = le thread principal a fini la trame
_pal_dirty: .res 1
_fade_lvl:  .res 1      ; fondu de la palette : 0 (normal) à 64 (noir)
_fade_skip: .res 1      ; 1 : prochaine coupure sans fondu
_ppu_mask:  .res 1
_pad1:      .res 1
_pad2:      .res 1
_pad1_new:  .res 1
_pad2_new:  .res 1
_vram_len:  .res 1
nmi_vlen:   .res 1      ; copie de vram_len pour la NMI
nmi_pal:    .res 1      ; compteur 50 Hz du son
_scroll_y:  .res 1      ; défilement vertical (0..239), générique de fin
_scroll_nt: .res 1      ; écran de nametable (0 ou 2 : bit 1 de PPUCTRL)
_scroll_x:  .res 1      ; défilement horizontal (bande-annonce : travellings)
_tz_bank:   .res 1      ; tz_unpack : banque des tuiles compressées
_tz_n:      .res 1      ; tz_unpack : nombre de tuiles
tz_src:     .res 2
tz_m:       .res 2
tz_prev:    .res 1
_rand_seed: .res 2
pad_tmp:    .res 1
pad_chk:    .res 2      ; lecture de contrôle des manettes (DPCM)
_oam_i:     .res 1      ; prochain emplacement libre dans _oam_buf
_spr_rot:   .res 1      ; premier emplacement utilisé (rotation anti-disparition)
_meta_x:    .res 1      ; paramètres de _spr_meta (personnage 2x3 tuiles)
_meta_y:    .res 1
_meta_tile: .res 1
_meta_attr: .res 1      ; bit 6 = miroir horizontal
_meta_t1:   .res 1      ; tuile gauche des rangées 2 et 3 (la droite suit)
_meta_t2:   .res 1
_cur_bank:  .res 1      ; banque commutable actuellement en $8000
call_vec:   .res 2      ; adresse appelée par _bank_call / _far_call
far_arg:    .res 2      ; argument (A/X) puis résultat de _far_call
_region_cnt:.res 2      ; nombre de boucles entre deux VBlank (détection NTSC/PAL)

.segment "BSS"
_pal_buf:   .res 32

.segment "STARTUP"
reset:
    sei
    cld
    ldx #$FF
    stx $8000           ; réinitialise le MMC1 (mode : dernière banque fixe en $C000)
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

    ; MMC1 : miroir vertical, $8000 commutable, CHR 8 Ko ; banque 0
    lda #$0E
    MMC1_WRITE $8000
    lda #0
    MMC1_WRITE $A000
    lda #0
    MMC1_WRITE $C000
    lda #2
    jsr _set_bank       ; banque 2 : motifs graphiques
    ; copie des 8 Ko de motifs dans la CHR-RAM
    lda #<_chr_data
    sta ptr1
    lda #>_chr_data
    sta ptr1+1
    bit PPUSTATUS
    lda #0
    sta PPUADDR
    sta PPUADDR
    ldx #32             ; 32 pages de 256 octets
    ldy #0
@chr:
    lda (ptr1),y
    sta PPUDATA
    iny
    bne @chr
    inc ptr1+1
    dex
    bne @chr
    lda #0
    jsr _set_bank
    ; efface la RAM de cartouche de travail (la zone de sauvegarde est conservée)
    lda #<__WBSS_RUN__
    sta ptr1
    lda #>__WBSS_RUN__
    sta ptr1+1
    ldx #>(__WBSS_SIZE__ + 255)
    lda #0
    tay
@wclr:
    sta (ptr1),y
    iny
    bne @wclr
    inc ptr1+1
    dex
    bne @wclr

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
    beq @skipj
    lda _ppu_mask
    and #$18
    bne @go             ; rendu coupé : la VRAM appartient au thread principal
@skipj:
    jmp @skip
@go:

    lda #0
    sta OAMADDR
    lda #>_oam_buf
    sta OAMDMA          ; 513 cycles

    lda _vram_len       ; taille du tampon de cette trame (remise à 0 plus bas)
    sta nmi_vlen
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
    ; budget du VBlank (~2270 cycles en NTSC) : DMA 513, tampon jusqu'à
    ; ~1450, palette ~750. Les deux ensemble débordaient sur le haut de
    ; l'image (traits horizontaux pendant les fondus et le défilement des
    ; menus) : avec un tampon chargé, la palette attend la trame suivante.
    lda nmi_vlen
    cmp #33
    bcs @nopal          ; pal_dirty reste à 1
    lda #$3F
    sta PPUADDR
    lda #$00
    sta PPUADDR
    ldx #0
    lda _fade_lvl
    bne @pal
@palf:                  ; sans fondu : copie directe (~480 cycles au lieu de ~740)
    lda _pal_buf,x
    sta PPUDATA
    inx
    cpx #32
    bne @palf
    beq @pald
@pal:
    lda _pal_buf,x
    sec
    sbc _fade_lvl       ; fondu : 0, 16, 32, 48 ou 64 (un cran plus sombre par 16)
    bcs @pok
    lda #$0F            ; en dessous du plus sombre : noir
@pok:
    sta PPUDATA
    inx
    cpx #32
    bne @pal
@pald:
    lda #0
    sta _pal_dirty
@nopal:
    lda #0
    sta PPUADDR
    sta PPUADDR
    lda _scroll_x
    sta PPUSCROLL
    lda _scroll_y
    sta PPUSCROLL
    lda #CTRL_VAL
    ora _scroll_nt
    sta PPUCTRL
    lda _ppu_mask
    sta PPUMASK
    lda #0
    sta _nmi_ready
@skip:
    ; son : à chaque trame, rendu coupé compris (après le travail du VBlank) ;
    ; en 50 Hz, une mise à jour de plus toutes les 5 trames (tempo du 60 Hz)
    jsr _audio_tick
    lda _is_pal
    beq @nopal50
    inc nmi_pal
    lda nmi_pal
    cmp #5
    bcc @nopal50
    lda #0
    sta nmi_pal
    jsr _audio_tick
@nopal50:
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
; Écran allumé : fondu au noir en 3 crans de 2 trames (la musique continue),
; puis coupure. Le nouvel écran s'allume noir (fondu à 64) et main.c le
; rallume progressivement.
_ppu_off:
    lda _fade_skip      ; même écran reconstruit (menu, pages) : pas de fondu
    beq @nfs
    lda #0
    sta _fade_skip
    beq @cut
@nfs:
    lda _ppu_mask
    beq @cut
    lda _fade_lvl
    bne @cut            ; déjà en fondu
    lda #16
@step:
    sta _fade_lvl
    lda #1
    sta _pal_dirty
    jsr _wait_frame
    jsr _wait_frame
    lda _fade_lvl
    clc
    adc #16
    cmp #64
    bcc @step
    sta _fade_lvl
@cut:
    lda #0
    sta _ppu_mask
    jsr _wait_frame
    lda #0
    sta PPUMASK
    rts

; void __fastcall__ tz_unpack(const u8 *src) : tuiles compressées de la
; bande-annonce (banque tz_bank, tz_n tuiles) vers la mémoire du PPU, à
; l'adresse déjà fixée (rendu coupé). Par tuile : un masque de 16 bits (bit
; i à 1 : l'octet i répète le précédent), puis les octets nouveaux.
_tz_unpack:
    sta tz_src
    stx tz_src+1
    lda _cur_bank
    pha
    lda _tz_bank
    jsr _set_bank
    ldy #0
@tile:
    lda (tz_src),y
    sta tz_m
    inc tz_src
    bne :+
    inc tz_src+1
:   lda (tz_src),y
    sta tz_m+1
    inc tz_src
    bne :+
    inc tz_src+1
:   sty tz_prev
    ldx #16
@byte:
    lsr tz_m+1
    ror tz_m
    bcs @rep
    lda (tz_src),y
    sta tz_prev
    inc tz_src
    bne @rep
    inc tz_src+1
@rep:
    lda tz_prev
    sta PPUDATA
    dex
    bne @byte
    dec _tz_n
    bne @tile
    pla
    jmp _set_bank

; void __fastcall__ tz_copy(const u8 *src) : tz_n octets (0 : 256) de la
; banque tz_bank vers la mémoire du PPU (rendu coupé)
_tz_copy:
    sta tz_src
    stx tz_src+1
    lda _cur_bank
    pha
    lda _tz_bank
    jsr _set_bank
    ldy #0
    ldx _tz_n
@l: lda (tz_src),y
    sta PPUDATA
    iny
    bne :+
    inc tz_src+1
:   dex
    bne @l
    pla
    jmp _set_bank

; void __fastcall__ mirror_h(u8 h) : miroir MMC1 horizontal (1, écrans
; empilés : défilement vertical du générique de fin) ou vertical (0, normal).
; Rendu coupé uniquement.
_mirror_h:
    and #1
    ora #$0E
    MMC1_WRITE $8000
    rts

; void ppu_on(void) : active fond + sprites (y compris colonne de gauche)
_ppu_on:
    ; La palette s'écrit dans le VBlank : rendu coupé, la NES affiche la
    ; couleur pointée par l'adresse VRAM quand elle est dans la palette ;
    ; l'écrire pendant que l'image se dessine faisait des traits de couleur
    ; à chaque allumage d'écran (générique, transitions).
    lda _ppu_mask
    beq @off
    lda #1              ; écran déjà allumé : la NMI enverra la palette
    sta _pal_dirty
    rts
@off:
    jsr _wait_frame     ; début du VBlank (rendu coupé : la NMI ne fait que le son)
    lda #CTRL_VAL
    sta PPUCTRL
    lda #$3F
    sta PPUADDR
    lda #$00
    sta PPUADDR
    ldx #0
@pal:
    lda _pal_buf,x
    sec
    sbc _fade_lvl       ; fondu : 0, 16, 32, 48 ou 64 (un cran plus sombre par 16)
    bcs @pok
    lda #$0F            ; en dessous du plus sombre : noir
@pok:
    sta PPUDATA
    inx
    cpx #32
    bne @pal
    lda #0
    sta PPUADDR
    sta PPUADDR
    sta PPUSCROLL
    sta PPUSCROLL
    sta _pal_dirty      ; palette déjà envoyée
    lda #%00011110      ; la NMI suivante active le rendu
    sta _ppu_mask
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

; u8 __fastcall__ swap_put(const u8 *src) — met dans le tampon VRAM une
; tuile (16 octets) du jeu de tuiles par phase, destination swp_dst. Rend 0
; si le tampon de la trame est plein (VBUF_MAX = 96 : 19 octets avec
; l'en-tête). En banque 13, à côté des tuiles (appelée par far_call) : bien
; plus rapide que bank_read + vbuf_raw en C.
.segment "BANK13"
_swap_put:
    sta ptr1
    stx ptr1+1
    ldx _vram_len
    cpx #96 - 19
    bcs @full
    lda _swp_dst+1
    sta _vram_buf,x
    lda _swp_dst
    sta _vram_buf+1,x
    lda #16
    sta _vram_buf+2,x
    ldy #0
@c: lda (ptr1),y
    sta _vram_buf+3,x
    inx
    iny
    cpy #16
    bne @c
    lda #$FF
    sta _vram_buf+3,x
    inx
    inx
    inx
    stx _vram_len
    lda #1
    ldx #0
    rts
@full:
    lda #0
    tax
    rts
.segment "CODE"

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
    lda _pad2
    pha                 ; état précédent de la manette 2
    ; pendant un échantillon DPCM, une lecture du DMC peut faire perdre un
    ; bit à la manette : on lit alors deux fois, jusqu'à deux lectures égales
@again:
    jsr read_raw
    lda $4015
    and #$10
    beq @ok
    lda _pad1
    sta pad_chk
    lda _pad2
    sta pad_chk+1
    jsr read_raw
    lda _pad1
    cmp pad_chk
    bne @again
    lda _pad2
    cmp pad_chk+1
    bne @again
@ok:
    pla
    eor #$FF
    and _pad2
    sta _pad2_new
    rts

read_raw:
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

; void spr_meta(void) : personnage 16x24 (2 colonnes x 3 rangées de tuiles)
; en (_meta_x, _meta_y). Tuile gauche de chaque rangée : _meta_tile, _meta_t1,
; _meta_t2 ; la tuile droite est la suivante.
_spr_meta:
    ; deux sprites par rangée, écrits l'un après l'autre : l'index X reboucle
    ; naturellement sur 256 octets (nécessaire avec la rotation des sprites)
    ldx _oam_i
    ldy _meta_y
    dey                 ; le PPU affiche les sprites une ligne plus bas
    lda #3
    sta tmp1            ; rangées restantes
    lda _meta_tile
    sta tmp2
@row:
    ; sprite de gauche (tuile + 1 en miroir)
    tya
    sta _oam_buf,x
    lda _meta_attr
    sta _oam_buf+2,x
    and #$40
    beq @l_n
    lda tmp2
    clc
    adc #1
    jmp @l_t
@l_n:
    lda tmp2
@l_t:
    sta _oam_buf+1,x
    lda _meta_x
    sta _oam_buf+3,x
    inx
    inx
    inx
    inx
    ; sprite de droite (tuile en miroir, tuile + 1 sinon)
    tya
    sta _oam_buf,x
    lda _meta_attr
    sta _oam_buf+2,x
    and #$40
    bne @r_m
    lda tmp2
    clc
    adc #1
    jmp @r_t
@r_m:
    lda tmp2
@r_t:
    sta _oam_buf+1,x
    lda _meta_x
    clc
    adc #8
    sta _oam_buf+3,x
    inx
    inx
    inx
    inx
    lda _meta_t1
    sta tmp2
    lda _meta_t2
    sta _meta_t1
    tya
    clc
    adc #8
    tay
    dec tmp1
    bne @row
    stx _oam_i
    rts

; Les sprites sont écrits à partir de _spr_rot (0 en temps normal). Quand
; trop de sprites partagent les mêmes lignes, le moteur fait tourner ce point
; de départ à chaque trame : le PPU, qui n'affiche que les 8 premiers sprites
; d'une ligne, en écarte alors un différent à chaque trame (léger clignotement
; au lieu d'un sprite qui disparaît). On masque tout jusqu'au point de départ.
_spr_end:
    ldx _oam_i
    lda #$FF
@h: sta _oam_buf,x
    inx
    inx
    inx
    inx
    cpx _spr_rot
    bne @h
    rts

; void __fastcall__ set_bank(u8 bank) : banque commutable en $8000
; (bit 4 = 0 : PRG-RAM activée)
_set_bank:
    and #$0F
    sta _cur_bank
    MMC1_WRITE $E000
    rts

; void __fastcall__ bank_call(u8 bank, void (*fn)(void))
; Appelle une fonction sans argument située dans une autre banque commutable,
; puis restaure la banque d'origine (trampoline en banque fixe).
_bank_call:
    sta call_vec
    stx call_vec+1
    ldy #0
    lda (sp),y          ; banque cible
    inc sp
    bne :+
    inc sp+1
:   tay
    lda _cur_bank
    pha
    tya
    jsr _set_bank
    jsr @go
    pla
    jmp _set_bank
@go:
    jmp (call_vec)

; u16 __fastcall__ far_call(u8 bank, void *fn, u16 arg)
; Appelle fn (fastcall, un argument en A/X, résultat en A/X) dans une autre
; banque commutable, puis restaure la banque d'origine.
_far_call:
    sta far_arg
    stx far_arg+1
    ldy #0
    lda (sp),y          ; fn (poids faible)
    sta call_vec
    iny
    lda (sp),y
    sta call_vec+1
    iny
    lda (sp),y          ; banque
    tay
    lda sp
    clc
    adc #3
    sta sp
    bcc :+
    inc sp+1
:   lda _cur_bank
    pha
    tya
    jsr _set_bank
    lda far_arg
    ldx far_arg+1
    jsr @go
    sta far_arg
    stx far_arg+1
    pla
    jsr _set_bank
    lda far_arg
    ldx far_arg+1
    rts
@go:
    jmp (call_vec)

; void rp_record(void) : enregistre la trame courante pour les ralentis
; (16 octets : balle x, y, z, visible, puis x, y, pose de 4 joueurs).
; Version assembleur (~450 cycles) : la version C coûtait ~1 700 cycles par
; trame et faisait déborder le double.
RP_LEN = 160
; A = (v >> 6) & $FF pour la valeur 16 bits pointée par ptr2 (positions < 16384)
.macro PX_TO_A addr, off
    lda addr+off+1
    sta tmp1
    lda addr+off
    asl a
    rol tmp1
    asl a
    rol tmp1
    lda tmp1
.endmacro
_rp_record:
    ; ptr1 = rp_buf + rp_head * 16
    lda _rp_head
    sta ptr1
    lda #0
    sta ptr1+1
    ldx #4
@sh:
    asl ptr1
    rol ptr1+1
    dex
    bne @sh
    lda ptr1
    clc
    adc #<_rp_buf
    sta ptr1
    lda ptr1+1
    adc #>_rp_buf
    sta ptr1+1
    ldy #0
    PX_TO_A _bx, 0
    sta (ptr1),y
    iny
    PX_TO_A _by, 0
    sta (ptr1),y
    iny
    lda _bz+1
    bpl @zpos
    lda #0
    beq @zst
@zpos:
    PX_TO_A _bz, 0
@zst:
    sta (ptr1),y
    iny
    lda _b_vis
    sta (ptr1),y
    ldx #0              ; joueur
@pl:
    stx tmp2
    txa
    asl a
    tax                 ; index des tableaux 16 bits
    lda _plx+1,x
    sta tmp1
    lda _plx,x
    asl a
    rol tmp1
    asl a
    rol tmp1
    iny
    lda tmp1
    sta (ptr1),y
    lda _ply+1,x
    sta tmp1
    lda _ply,x
    asl a
    rol tmp1
    asl a
    rol tmp1
    iny
    lda tmp1
    sta (ptr1),y
    ldx tmp2
    ; pose (bits 0-4) | miroir << 5 | saut (bits 6-7 de pose_cache) ; le
    ; côté est recalculé au ralenti (sc_swap ne change qu'après le ralenti)
    lda _flip_cache,x
    asl a
    asl a
    asl a
    asl a
    asl a
    ora _pose_cache,x
    iny
    sta (ptr1),y
    inx
    cpx #4
    bne @pl
    ; avance la tête du tampon circulaire
    ldx _rp_head
    inx
    cpx #RP_LEN
    bne :+
    ldx #0
:   stx _rp_head
    lda _rp_count
    cmp #RP_LEN
    bcs :+
    inc _rp_count
:   rts

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

.segment "STUBF"
    sei
    ldx #$FF
    txs
    stx $8000
    jmp reset
    .word nmi, reset, irq

; Programmes de démarrage des banques commutables : si une de ces banques
; occupe $C000 à la mise sous tension, elle réinitialise le MMC1 (ce qui fixe
; la dernière banque, la 15, en $C000) puis saute au vrai démarrage.
.macro BANK_STUB seg
.segment seg
    sei
    ldx #$FF
    txs
    stx $8000
    jmp reset
    .word $FFF0, $FFF0, $FFF0
.endmacro
BANK_STUB "STUB0"
BANK_STUB "STUB1"
BANK_STUB "STUB2"
BANK_STUB "STUB3"
BANK_STUB "STUB4"
BANK_STUB "STUB5"
BANK_STUB "STUB6"
BANK_STUB "STUB7"
BANK_STUB "STUB8"
BANK_STUB "STUB9"
BANK_STUB "STUB10"
BANK_STUB "STUB11"
BANK_STUB "STUB12"
BANK_STUB "STUB13"
BANK_STUB "STUB14"

.segment "BANK2"
_chr_data:
    .incbin "../assets/tiles.chr"
