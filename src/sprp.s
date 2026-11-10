; Royal Martinot — affichage d'un personnage 16 x 24 (banque fixe, assembleur).
;
; Version assembleur de spr_person (ancienne version C dans gfx.c) : c'était
; la fonction la plus coûteuse de chaque trame (environ 570 cycles par
; personnage, passage des 7 arguments compris). Même comportement :
;  - rognage du coin haut-gauche (0..240, 0..216) ;
;  - tuiles selon le sexe, la vue (face/dos) et la pose ;
;  - course de profil selon le sens du déplacement, bruit de pas ;
;  - Margaux (personnage 16) : rangée de la tête remplacée par ses tuiles.

.export _spr_person
.export _run_x, _run_y, _run_dir, _run_pose
.import _spr_meta, _oam_buf
.importzp _meta_x, _meta_y, _meta_tile, _meta_attr, _oam_i
.import _person_base, _profile_base, _char_female
.import _marg_head, _marg_prof_l, _marg_prof_r
.import _sfx_step
.import _ms_timer, _confetti_tile
.export _win_fx
.import incsp8
.importzp sp, tmp1, tmp2, tmp3

POSE_RUN1 = 1
POSE_RUN2 = 2
MARGAUX   = 16
NCHARS    = 17

; positions et état de course par personnage (RAM de cartouche)
.segment "WBSS"
_run_x:    .res NCHARS * 2
_run_y:    .res NCHARS * 2
_run_dir:  .res NCHARS
_run_pose: .res NCHARS

.segment "WBSS"         ; RAM de cartouche (la RAM interne est pleine)
px:     .res 2
py:     .res 2
pc:     .res 1
pfront: .res 1
ppose:  .res 1
pflip:  .res 1
ppal:   .res 1
ptile:  .res 1
pdx:    .res 2
pdy:    .res 2

.segment "RODATA"
; variante de bras de la tête de Margaux par pose : attente, frappe armée,
; accompagnement, service
marg_var: .byte 0, 0, 0, 1, 2, 3, 0, 0

.segment "CODE"

; void __fastcall__ spr_person(s16 x, s16 y, u8 c, u8 front, u8 pose, u8 flip, u8 pal)
; pile : (sp)+0 flip, +1 pose, +2 front, +3 c, +4/5 y, +6/7 x ; pal dans A
_spr_person:
    sta ppal
    ldy #0
    lda (sp),y
    sta pflip
    iny
    lda (sp),y
    sta ppose
    iny
    lda (sp),y
    sta pfront
    iny
    lda (sp),y
    sta pc
    iny
    lda (sp),y
    sta py
    iny
    lda (sp),y
    sta py+1
    iny
    lda (sp),y
    sta px
    iny
    lda (sp),y
    sta px+1

    ; coin gauche = x - 8, borné à 0..240
    sec
    lda px
    sbc #8
    tax
    lda px+1
    sbc #0
    bmi @l0
    bne @l240
    cpx #241
    bcc @lok
@l240:
    ldx #240
    bne @lok
@l0:
    ldx #0
@lok:
    stx _meta_x
    ; haut = y - 24, borné à 0..216
    sec
    lda py
    sbc #24
    tax
    lda py+1
    sbc #0
    bmi @t0
    bne @t216
    cpx #217
    bcc @tok
@t216:
    ldx #216
    bne @tok
@t0:
    ldx #0
@tok:
    stx _meta_y

    ; tuile = person_base[(femme << 1) | face] + pose * 6
    ldx pc
    lda _char_female,x
    asl a
    ora pfront
    tax
    lda _person_base,x
    sta tmp1
    lda ppose
    asl a
    sta tmp2
    asl a
    clc
    adc tmp2
    clc
    adc tmp1
    sta ptile

    ; course : de profil si le déplacement est surtout latéral
    lda ppose
    cmp #POSE_RUN1
    beq @run
    cmp #POSE_RUN2
    beq @run
    ldx pc
    lda #0
    sta _run_dir,x
    jmp @store
@run:
    lda pc
    asl a
    tay
    sec
    lda px
    sbc _run_x,y
    sta pdx
    lda px+1
    sbc _run_x+1,y
    sta pdx+1
    sec
    lda py
    sbc _run_y,y
    sta pdy
    lda py+1
    sbc _run_y+1,y
    sta pdy+1
    lda pdx
    ora pdx+1
    ora pdy
    ora pdy+1
    beq @nomove
    lda pdx+1
    sta tmp3                ; signe de dx
    bpl @dxpos
    sec
    lda #0
    sbc pdx
    sta pdx
    lda #0
    sbc pdx+1
    sta pdx+1
@dxpos:
    lda pdy+1
    bpl @dypos
    sec
    lda #0
    sbc pdy
    sta pdy
    lda #0
    sbc pdy+1
    sta pdy+1
@dypos:
    ; |dx| >= |dy| ?
    lda pdx
    cmp pdy
    lda pdx+1
    sbc pdy+1
    bcc @vert
    lda #1                  ; vers la droite
    ldx tmp3
    bpl @setdir
    lda #2                  ; vers la gauche
    bne @setdir
@vert:
    lda #0
@setdir:
    ldx pc
    sta _run_dir,x
@nomove:
    ldx pc
    lda _run_dir,x
    beq @nodir
    ; profil : profile_base + (femme ? 12 : 0) + (course B ? 6 : 0)
    lda _profile_base
    ldy _char_female,x
    beq @pm
    clc
    adc #12
@pm:
    ldy ppose
    cpy #POSE_RUN2
    bne @pa
    clc
    adc #6
@pa:
    sta ptile
    ldy #0
    lda _run_dir,x
    cmp #2
    bne @pf
    iny
@pf:
    sty pflip
@nodir:
    ; bruit de pas discret à chaque foulée
    lda ppose
    cmp #POSE_RUN1
    bne @store
    ldx pc
    lda _run_pose,x
    cmp #POSE_RUN2
    bne @store
    jsr _sfx_step

@store:
    lda pc
    asl a
    tay
    lda px
    sta _run_x,y
    lda px+1
    sta _run_x+1,y
    lda py
    sta _run_y,y
    lda py+1
    sta _run_y+1,y
    ldx pc
    lda ppose
    sta _run_pose,x
    lda ptile
    sta _meta_tile
    lda ppal
    ldy pflip
    beq @na
    ora #$40
@na:
    sta _meta_attr
    jsr _spr_meta

    ; Margaux : tuiles de sa tête (cheveux longs, blonds et bouclés)
    lda pc
    cmp #MARGAUX
    bne @done
    lda _oam_i
    sec
    sbc #24
    tax                     ; première entrée OAM de ce personnage
    ldy pc
    lda _run_dir,y
    beq @head
    lda ppose
    cmp #POSE_RUN1
    beq @prof
    cmp #POSE_RUN2
    beq @prof
@head:
    ldy ppose
    lda marg_var,y
    asl a
    sta tmp1
    lda pfront
    asl a
    asl a
    asl a
    clc
    adc tmp1
    clc
    adc _marg_head
    sta tmp1
    clc
    adc #1
    sta tmp2
    jmp @put
@prof:
    lda _marg_prof_l
    sta tmp1
    lda _marg_prof_r
    sta tmp2
@put:
    ; en miroir, les deux tuiles s'échangent ; le 2e sprite est 4 octets plus
    ; loin, l'index reboucle sur 256 (rotation des sprites)
    lda pflip
    beq @nf
    lda tmp1
    ldy tmp2
    sty tmp1
    sta tmp2
@nf:
    lda tmp1
    sta _oam_buf+1,x
    inx
    inx
    inx
    inx
    lda tmp2
    sta _oam_buf+1,x
@done:
    ; tête entièrement dans le bandeau du haut (lignes 0 à 31 : score et
    ; annonces) : ses 2 sprites passent derrière le texte du bandeau, qui
    ; reste lisible ; la tête reste visible entre les lettres (fond du
    ; bandeau transparent). Cas du joueur du haut au service.
    lda _meta_y
    cmp #25
    bcs @end
    lda _oam_i
    sec
    sbc #24
    tax
    lda _oam_buf+2,x
    ora #$20
    sta _oam_buf+2,x
    inx
    inx
    inx
    inx
    lda _oam_buf+2,x
    ora #$20
    sta _oam_buf+2,x
@end:
    jmp incsp8              ; 8 octets d'arguments sur la pile (x, y : 2 octets chacun ; c, front, pose, flip : 1)

; ---------------------------------------------------------------------------
; void win_fx(void) : fête de victoire. Pendant « JEU, SET ET MATCH ! »
; (ms_timer de 160 à 0), 10 sprites tombent à 3 vitesses en se balançant :
; 5 confettis et 5 étoiles (tuile de la frappe parfaite) blanches ou jaunes.
; En assembleur : la version C débordait de la trame en double.
.segment "WBSS"
wf_y:   .res 10

.segment "RODATA"
wf_x:    .byte 24, 56, 88, 120, 152, 184, 216, 40, 136, 200
wf_v:    .byte 2, 3, 2, 1, 3, 2, 1, 3, 2, 1
wf_y0:   .byte 32, 55, 78, 101, 124, 147, 170, 193, 32, 55
wf_tile: .byte 0, 3, 0, 3, 0, 3, 0, 3, 0, 3      ; 0 = tuile des confettis
wf_pal:  .byte 0, 2, 2, 3, 0, 2, 2, 3, 0, 2      ; étoiles blanches (2) et jaunes (3)
wf_sway: .byte 0, 1, 2, 3, 3, 2, 1, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0   ; phase (0..7) + index (0..9)

.segment "CODE"
_win_fx:
    lda #160
    sec
    sbc _ms_timer
    lsr a
    lsr a
    lsr a
    and #7
    sta tmp1                ; phase du balancement
    lda _ms_timer
    cmp #159
    bcc @run
    ldy #9                  ; début de la fête : positions de départ
@init:
    lda wf_y0,y
    sta wf_y,y
    dey
    bpl @init
@run:
    ldx _oam_i
    ldy #9
@loop:
    lda wf_y,y              ; chute, puis retour en haut sous le bandeau
    clc
    adc wf_v,y
    cmp #216
    bcc @yok
    sbc #184
@yok:
    sta wf_y,y
    sec
    sbc #1                  ; le PPU affiche les sprites une ligne plus bas
    sta _oam_buf,x
    lda wf_tile,y
    bne @tile
    lda _confetti_tile
@tile:
    sta _oam_buf+1,x
    lda wf_pal,y
    sta _oam_buf+2,x
    sty tmp2
    tya
    clc
    adc tmp1
    tay
    lda wf_sway,y
    ldy tmp2
    clc
    adc wf_x,y
    sta _oam_buf+3,x
    inx                     ; l'index reboucle sur 256 (rotation des sprites)
    inx
    inx
    inx
    dey
    bpl @loop
    stx _oam_i
    rts

; ---------------------------------------------------------------------------
; void audio_tick(void) : lecteur de musique, appelé par la NMI à chaque trame
; (même pendant la construction d'un écran, rendu coupé : la musique ne prend
; plus de retard). Mélodie sur le carré 2, basse sur le triangle ; données en
; couples (note, durée), $FE silence, $FF fin (ou boucle). music_play (C)
; positionne audio_lock pendant qu'il change l'état.
.export _audio_tick
.import _noise_hold, _mel_ptr, _bass_ptr, _mel_start, _bass_start
.import _mel_timer, _bass_timer, _music_loop, _mel_duty, _music_cur
.import _note_period, _audio_lock

.segment "ZEROPAGE"
aptr:   .res 2

.segment "CODE"
_audio_tick:
    lda _noise_hold
    beq @nh
    dec _noise_hold
@nh:
    lda _audio_lock
    beq @mel
    rts
@mel:
    lda _mel_ptr+1          ; pointeur nul : pas de mélodie (données hors page 0)
    beq @bass
    dec _mel_timer
    bne @bass
    lda _mel_ptr
    sta aptr
    lda _mel_ptr+1
    sta aptr+1
    ldy #0
    lda (aptr),y
    cmp #$FF
    bne @mplay
    lda _music_loop
    bne @mloop
    lda #0                  ; fin du morceau
    sta _mel_ptr
    sta _mel_ptr+1
    sta _music_cur
    lda #$30
    sta $4004
    jmp @bass
@mloop:
    lda _mel_start
    sta aptr
    lda _mel_start+1
    sta aptr+1
    ldy #0
    lda (aptr),y
@mplay:
    cmp #$FE
    bne @mnote
    lda #$30                ; silence
    sta $4004
    jmp @mdur
@mnote:
    asl a
    tax
    lda _mel_duty
    sta $4004
    lda #0
    sta $4005
    lda _note_period,x
    sta $4006
    lda _note_period+1,x
    ora #$08
    sta $4007
@mdur:
    ldy #1
    lda (aptr),y
    sta _mel_timer
    clc
    lda aptr
    adc #2
    sta _mel_ptr
    lda aptr+1
    adc #0
    sta _mel_ptr+1

@bass:
    lda _bass_ptr+1
    beq @done
    dec _bass_timer
    bne @done
    lda _bass_ptr
    sta aptr
    lda _bass_ptr+1
    sta aptr+1
    ldy #0
    lda (aptr),y
    cmp #$FF
    bne @bplay
    lda _music_loop
    bne @bloop
    lda #0
    sta _bass_ptr
    sta _bass_ptr+1
    lda #$80
    sta $4008
    rts
@bloop:
    lda _bass_start
    sta aptr
    lda _bass_start+1
    sta aptr+1
    ldy #0
    lda (aptr),y
@bplay:
    cmp #$FE
    bne @bnote
    lda #$80
    sta $4008
    jmp @bdur
@bnote:
    asl a
    tax
    lda #$FF
    sta $4008
    lda _note_period,x
    sta $400A
    lda _note_period+1,x
    ora #$08
    sta $400B
@bdur:
    ldy #1
    lda (aptr),y
    sta _bass_timer
    clc
    lda aptr
    adc #2
    sta _bass_ptr
    lda aptr+1
    adc #0
    sta _bass_ptr+1
@done:
    rts

; ---------------------------------------------------------------------------
; void ump_draw(void) : arbitre de chaise, assis de profil sur la chaise du
; décor (x 16-31, y 112-159), tourné vers le court. Tête (11) et buste (12 au
; repos, 13 bras tendu, coupe + 2 bras levé), plus un 3e sprite pour la main
; (coupe + 0) ou le bras levé (coupe + 1). En assembleur : la version C, à
; chaque trame dans tous les lieux, faisait déborder des trames chargées.
.export _ump_draw
.import _ms, _end_reason, _ump_arm, _trophy_tile
.importzp _frame_cnt
.importzp tmp4
CHAIR_SY = 112

.segment "CODE"
_ump_draw:
    lda #12
    sta tmp1                ; buste
    lda #0
    sta tmp2                ; 3e sprite (0 : aucun)
    lda #27
    sta tmp3                ; x du 3e sprite
    lda #CHAIR_SY + 11
    sta tmp4                ; y du 3e sprite
    lda _ms
    cmp #4                  ; MS_OVER : bras levé qui salue
    bne @dead
    lda _frame_cnt
    lsr a
    lsr a
    lsr a
    and #1
    eor #$FF
    sec
    adc #CHAIR_SY - 1       ; CHAIR_SY - 1 - ((frame_cnt >> 3) & 1)
    sta tmp4
    lda #25
    sta tmp3
    jmp @up
@dead:
    cmp #3                  ; MS_DEAD
    bne @draw
    lda _end_reason
    cmp #3                  ; END_LET : bras levé qui s'agite
    bne @arm
    lda _frame_cnt
    lsr a
    lsr a
    and #1
    clc
    adc #25
    sta tmp3
    lda #CHAIR_SY - 1
    sta tmp4
@up:
    lda _trophy_tile
    clc
    adc #2
    sta tmp1
    sec
    sbc #1
    sta tmp2
    jmp @draw
@arm:
    lda _ump_arm            ; faute, balle dehors : bras tendu
    beq @draw
    lda _ms_timer
    cmp #16
    bcc @draw
    lda #13
    sta tmp1
    lda _trophy_tile
    sta tmp2
@draw:
    ldx _oam_i
    lda #CHAIR_SY + 3 - 1   ; tête (le PPU affiche une ligne plus bas)
    sta _oam_buf,x
    lda #11
    sta _oam_buf+1,x
    lda #2
    sta _oam_buf+2,x
    lda #19
    sta _oam_buf+3,x
    inx
    inx
    inx
    inx
    lda #CHAIR_SY + 11 - 1  ; buste
    sta _oam_buf,x
    lda tmp1
    sta _oam_buf+1,x
    lda #2
    sta _oam_buf+2,x
    lda #19
    sta _oam_buf+3,x
    inx
    inx
    inx
    inx
    lda tmp2
    beq @end
    sta _oam_buf+1,x
    ldy tmp4
    dey
    tya
    sta _oam_buf,x
    lda #2
    sta _oam_buf+2,x
    lda tmp3
    sta _oam_buf+3,x
    inx
    inx
    inx
    inx
@end:
    stx _oam_i
    rts
