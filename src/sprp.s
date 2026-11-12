; Royal Martinot — affichage d'un personnage 16 x 24 (banque fixe, assembleur).
;
; Version assembleur de spr_person (ancienne version C dans gfx.c) : c'était
; la fonction la plus coûteuse de chaque trame (environ 570 cycles par
; personnage, passage des 7 arguments compris). Même comportement :
;  - rognage du coin haut-gauche (0..240, 0..216) ;
;  - tuiles selon le sexe, la vue (face/dos) et la pose ;
;  - course de profil selon le sens du déplacement, bruit de pas ;
;  - Margaux (personnage 16) : rangée de la tête remplacée par ses tuiles.

.segment "ZEROPAGE"      ; vue en perspective (voir _proj, en fin de fichier)
_persp:  .res 1         ; 1 : match en vue perspective
_proj_x: .res 1
_proj_y: .res 1
_proj_s: .res 1
pj_v:    .res 1
pj_vh:   .res 1
pj_v0:   .res 1
pj_sg:   .res 1          ; indice du joueur (spr_person)
pj_m:    .res 1
pj_lo:   .res 1
pj_hi:   .res 1
.segment "CODE"
.export _spr_person
.export _run_x, _run_y, _run_dir, _run_pose
.import _spr_meta, _oam_buf
.importzp _meta_x, _meta_y, _meta_tile, _meta_attr, _meta_t1, _meta_t2, _oam_i
.import _person_map_m, _person_map_f, _person_row, _prof_map, _char_female
.import _marg_map, _marg_prof_l, _marg_prof_r
.import _sfx_step
.import _ms_timer, _confetti_tile
.export _win_fx
.import incsp8
.importzp sp, tmp1, tmp2, tmp3

POSE_RUN1 = 1
POSE_RUN2 = 2
POSE_LIFT = 32          ; drapeau : personnage relevé d'1 px
POSE_DOWN = 64          ; drapeau : personnage abaissé d'1 px
NPOSE     = 28
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
pt1:    .res 1
pt2:    .res 1
plift:  .res 1
pdx:    .res 2
pdy:    .res 2

.segment "RODATA"
; image de course de profil par pose (décalage dans prof_map), $FF : pas une
; pose de course. Ordre de la course : 1, 3 (passage), 2, 4 (passage).
runfr:  .byte $FF, 0, 3, $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
        .byte $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF, 6, 9, $FF
        .byte $FF, $FF, $FF, $FF, $FF          ; 22-26 : revers (version 2.42)

.segment "CODE"

; void __fastcall__ spr_person(s16 x, s16 y, u8 c, u8 front, u8 pose, u8 flip, u8 pal)
; pile : (sp)+0 flip, +1 pose, +2 front, +3 c, +4/5 y, +6/7 x ; pal dans A
_spr_person:
    sta pj_sg               ; indice du joueur (0..3) : position projetée gardée
    and #1                  ; pour son ombre (match_fx) ; palette = indice & 1
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

    ; vue en perspective : position projetée (x, y à plat bornés à 0..255)
    lda _persp
    beq @flat
    ldy #255
    lda px+1
    bmi @x0
    beq :+
    sty px
    bne :+
@x0:
    lda #0
    sta px
:   lda py+1
    bmi @y0
    beq :+
    sty py
    bne :+
@y0:
    lda #0
    sta py
:   ldx py
    lda px
    jsr _proj
    lda pj_sg
    and #3
    tax
    lda _proj_x
    sta px
    sta _psh_x,x
    lda _proj_y
    sta py
    sta _psh_y,x
    lda #0
    sta px+1
    sta py+1
@flat:

    ; drapeaux POSE_LIFT / POSE_DOWN : 1 px plus haut (rebond, foulée) ou
    ; plus bas (genoux fléchis) ; la position mémorisée pour la course reste
    ; celle du sol
    lda ppose
    and #POSE_LIFT | POSE_DOWN
    sta plift
    eor ppose
    sta ppose
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
    lda plift
    beq @ty
    and #POSE_LIFT
    beq @tdn
    txa
    beq @ty
    dex
    jmp @ty
@tdn:
    cpx #216
    bcs @ty
    inx
@ty:
    stx _meta_y

    ; tuiles : person_map_(m|f)[person_row[face] + pose * 3 + rangée]
    ldx pfront
    lda ppose
    asl a
    clc
    adc ppose
    adc _person_row,x
    tay
    ldx pc
    lda _char_female,x
    bne @fem
    lda _person_map_m,y
    sta ptile
    lda _person_map_m+1,y
    sta pt1
    lda _person_map_m+2,y
    sta pt2
    jmp @tiles
@fem:
    lda _person_map_f,y
    sta ptile
    lda _person_map_f+1,y
    sta pt1
    lda _person_map_f+2,y
    sta pt2
@tiles:

    ; course : de profil si le déplacement est surtout latéral
    ldy ppose
    lda runfr,y
    bpl @run
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
    ; profil : prof_map[(femme ? 12 : 0) + image * 3 + rangée]
    ldy ppose
    lda runfr,y
    ldy _char_female,x
    beq @pa
    clc
    adc #12
@pa:
    tay
    lda _prof_map,y
    sta ptile
    lda _prof_map+1,y
    sta pt1
    lda _prof_map+2,y
    sta pt2
    ldy #0
    lda _run_dir,x
    cmp #2
    bne @pf
    iny
@pf:
    sty pflip
@nodir:
    ; bruit de pas discret à chaque appui (début des foulées A et B)
    lda ppose
    cmp #POSE_RUN1
    beq @step
    cmp #POSE_RUN2
    bne @store
@step:
    ldx pc
    cmp _run_pose,x
    beq @store
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
    lda pt1
    sta _meta_t1
    lda pt2
    sta _meta_t2
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
    ldy ppose
    lda runfr,y
    bpl @prof
@head:
    lda ppose               ; marg_map[face * NPOSE + pose]
    ldy pfront
    beq :+
    clc
    adc #NPOSE
:   tay
    lda _marg_map,y
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
; Lecteur de musique, appelé par la NMI à chaque trame (la musique avance même
; pendant la construction d'un écran). 4 voix, copiées en RAM de cartouche par
; music_play : mélodie (carré 2), basse (triangle), harmonie (carré 1, laissé
; aux bruitages pendant p1_hold trames), percussions (bruit, laissé aux
; bruitages tant que noise_hold). Une voix : paires (note, durée) ; $FE
; silence, $FF fin (boucle si music_loop).
; ---------------------------------------------------------------------------
.export _audio_tick
.export _trk_ptr, _trk_start, _trk_timer
.export _mel_ptr, _bass_ptr, _mel_start, _bass_start, _mel_timer, _bass_timer
.export _music_loop, _mel_duty, _harm_duty, _p1_hold
.import _noise_hold, _music_cur, _note_period, _audio_lock

.segment "WBSS"
_trk_ptr:                   ; pointeurs des 4 voix (mélodie, basse, harmonie, percussions)
_mel_ptr:   .res 2
_bass_ptr:  .res 2
            .res 4
_trk_start:
_mel_start: .res 2
_bass_start: .res 2
            .res 4
_trk_timer:                 ; trames restantes de la note en cours (pas de 2)
_mel_timer: .res 2
_bass_timer: .res 2
            .res 4
_music_loop: .res 1
_mel_duty:  .res 1
_harm_duty: .res 1
_p1_hold:   .res 1          ; carré 1 réservé à un bruitage pendant n trames
anote:      .res 1
atrk:       .res 1

.segment "ZEROPAGE"
aptr:   .res 2

.segment "RODATA"
; percussions : $400C (volume constant, durée), $400E (période), $400F (durée)
drum_c: .byte $16, $15, $13         ; grosse caisse, caisse claire, charleston
drum_e: .byte $0D, $06, $01
drum_f: .byte $28, $48, $18

.segment "CODE"
_audio_tick:
    lda _noise_hold
    beq :+
    dec _noise_hold
:   lda _p1_hold
    beq :+
    dec _p1_hold
:   lda _audio_lock
    beq :+
    rts
:   ldx #0
@trk:
    lda _trk_ptr+1,x        ; pointeur nul : voix absente (données hors page 0)
    beq @next
    dec _trk_timer,x
    bne @next
    lda _trk_ptr,x
    sta aptr
    lda _trk_ptr+1,x
    sta aptr+1
    ldy #0
    lda (aptr),y
    cmp #$FF
    bne @play
    lda _music_loop
    beq @stop
    lda _trk_start,x
    sta aptr
    lda _trk_start+1,x
    sta aptr+1
    lda (aptr),y
@play:
    sta anote
    iny
    lda (aptr),y
    sta _trk_timer,x
    clc
    lda aptr
    adc #2
    sta _trk_ptr,x
    lda aptr+1
    adc #0
    sta _trk_ptr+1,x
    stx atrk
    jsr @out
    ldx atrk
@next:
    inx
    inx
    cpx #8
    bne @trk
    rts
@stop:
    lda #0
    sta _trk_ptr,x
    sta _trk_ptr+1,x
    lda #$FE                ; fin : silence sur la voix
    sta anote
    stx atrk
    txa
    bne :+
    lda #0                  ; fin de la mélodie : morceau terminé
    sta _music_cur
:   jsr @out
    ldx atrk
    jmp @next

; sortie de la note anote sur le canal de la voix X (0, 2, 4, 6)
@out:
    cpx #2
    beq @bass
    bcc @mel
    cpx #4
    beq @harm
    ; percussions
    lda _noise_hold
    bne @rts
    ldy anote
    cpy #3
    bcs @rts
    lda drum_c,y
    sta $400C
    lda drum_e,y
    sta $400E
    lda drum_f,y
    sta $400F
@rts:
    rts
@mel:
    lda anote
    cmp #$FE
    bne :+
    lda #$30
    sta $4004
    rts
:   asl a
    tay
    lda _mel_duty
    sta $4004
    lda #$08                ; balayage inactif sans couper les notes graves
    sta $4005
    lda _note_period,y
    sta $4006
    lda _note_period+1,y
    ora #$08
    sta $4007
    rts
@bass:
    lda anote
    cmp #$FE
    bne :+
    lda #$80
    sta $4008
    rts
:   asl a
    tay
    lda #$FF
    sta $4008
    lda _note_period,y
    sta $400A
    lda _note_period+1,y
    ora #$08
    sta $400B
    rts
@harm:
    lda _p1_hold            ; un bruitage occupe le carré 1
    bne @rts
    lda anote
    cmp #$FE
    bne :+
    lda #$30
    sta $4000
    rts
:   asl a
    tay
    lda _harm_duty
    sta $4000
    lda #$08
    sta $4001
    lda _note_period,y
    sta $4002
    lda _note_period+1,y
    ora #$08
    sta $4003
    rts

; ---------------------------------------------------------------------------
; void __fastcall__ spr_ball(u8 x, s16 sy, u8 shy, u8 zpx) : balle et ombre.
; (x, sy) = position affichée de la balle, shy = ombre au sol, zpx = hauteur.
;  - la balle grossit avec la hauteur : 4 px, 5 px dès 12 px de haut, 6 px
;    dès 20 px, 7 px dès 40 px (vrais lobs) ; son ombre rétrécit (grande sous
;    12 px, moyenne, petite dès 40 px) ;
;  - la couture blanche tourne selon l'effet : lifté vers l'avant, coupé et
;    amortie vers l'arrière (plus vite pour l'amortie), à plat immobile ;
;  - au rebond, la balle est écrasée pendant 3 trames ;
;  - dans le camp du haut, à hauteur du filet, la balle est vue à travers
;    les mailles (dessin en damier) ;
;  - palette des effets (3) : couleur de la balle selon le court.
; pile : (sp)+0 shy, +1/+2 sy, +3 x ; zpx dans A
; ---------------------------------------------------------------------------
.export _spr_ball
.import _ball_base, _bspin, _bvy
.import incsp4

SPIN_TOP   = 1
SPIN_SLICE = 2
SPIN_DROP  = 3
NET_Y      = 132

.segment "WBSS"
bz_px:  .res 1
bshy:   .res 1
bsy:    .res 2
bxx:    .res 1
brot:   .res 1          ; position de la couture (0..3)
brot_t: .res 1          ; trames avant le prochain quart de tour
blast:  .res 1          ; hauteur à la trame précédente
bsq:    .res 1          ; trames de balle écrasée restantes
btile:  .res 1

.segment "CODE"
_spr_ball:
    sta bz_px
    ldy #0
    lda (sp),y
    sta bshy
    iny
    lda (sp),y
    sta bsy
    iny
    lda (sp),y
    sta bsy+1
    iny
    lda (sp),y
    sta bxx

    ; rotation de la couture
    dec brot_t
    bpl @norot
    lda _bspin
    beq @norot0             ; à plat : immobile
    ldx #2                  ; un quart de tour toutes les 3 trames
    cmp #SPIN_DROP
    bne :+
    ldx #1                  ; amortie : toutes les 2 trames
:   stx brot_t
    ; sens : +1 (la couture descend) pour un lifté qui descend l'écran ;
    ; inversé si la balle monte l'écran, inversé pour le coupé et l'amortie
    ldx #1
    cmp #SPIN_TOP
    beq :+
    ldx #$FF
:   lda _bvy+1
    bpl :+
    txa
    eor #$FF
    clc
    adc #1
    tax
:   txa
    clc
    adc brot
    and #3
    sta brot
    jmp @norot
@norot0:
    lda #2
    sta brot_t
@norot:
    ; rebond : la balle touche le sol après avoir été en l'air
    lda bz_px
    cmp #2
    bcs @air
    lda blast
    cmp #2
    bcc @air
    lda #3
    sta bsq
@air:
    lda bz_px
    sta blast

    ; tuile de la balle
    lda bsq
    beq @size
    dec bsq
    lda _ball_base
    clc
    adc #12                 ; balle écrasée
    sta btile
    jmp @put
@size:
    ldx #0                  ; taille : 0 petite (4 px), 4 moyenne (5 px),
    lda bz_px               ; 17 balle de 6 px, 8 grande (7 px)
    cmp #12
    bcc @sz
    ldx #4
    cmp #20
    bcc @sz
    ldx #17
    cmp #40
    bcc @sz
    ldx #8
@sz:
    stx tmp1
    ; derrière le filet : camp du haut, centre de la balle dans la bande du
    ; filet (y 124 à 134), entre les poteaux (x 48 à 208)
    cpx #5
    bcs @rot
    lda bshy
    cmp #NET_Y
    bcs @rot
    lda bsy+1
    bne @rot
    lda bsy
    cmp #124 - 3
    bcc @rot
    cmp #134 - 3 + 1
    bcs @rot
    lda bxx
    cmp #48
    bcc @rot
    cmp #209
    bcs @rot
    txa
    lsr a
    lsr a                   ; 0 petite, 1 moyenne
    clc
    adc #13
    adc _ball_base
    sta btile
    jmp @put
@rot:
    lda tmp1
    clc
    adc brot
    adc _ball_base
    sta btile
@put:
    ; vue en perspective : ombre projetée, hauteur réduite selon la distance
    lda _persp
    beq @pdone
    sec
    lda bshy
    sbc bsy
    sta tmp1                ; hauteur affichée (bshy - bsy), bornée à 255
    lda #0
    sbc bsy+1
    beq :+
    lda #255
    sta tmp1
:   ldx bshy
    lda bxx
    jsr _proj
    lda _proj_x
    sta bxx
    lda _proj_y
    sta bshy
    lda tmp1
    jsr _pscale
    sta tmp1
    sec
    lda bshy
    sbc tmp1
    sta bsy
    lda #0
    sbc #0
    sta bsy+1
@pdone:
    ; balle (si elle est dans l'écran)
    lda bsy+1
    bne @shadow
    ldx _oam_i
    lda bsy
    sec
    sbc #1
    sta _oam_buf,x
    lda btile
    sta _oam_buf+1,x
    lda #3
    sta _oam_buf+2,x
    lda bxx
    sec
    sbc #3
    sta _oam_buf+3,x
    txa
    clc
    adc #4
    sta _oam_i
@shadow:
    ; ombre : grande au sol, puis moyenne et petite quand la balle monte
    lda #2
    ldx bz_px
    cpx #12
    bcc @sh
    lda _ball_base
    clc
    adc #15
    cpx #40
    bcc @sh
    adc #0                  ; retenue à 1 : + 1, petite ombre
@sh:
    ldx _oam_i
    sta _oam_buf+1,x
    lda bshy
    sec
    sbc #3 + 1
    sta _oam_buf,x
    lda #2
    sta _oam_buf+2,x
    lda bxx
    sec
    sbc #3
    sta _oam_buf+3,x
    txa
    clc
    adc #4
    sta _oam_i
    jmp incsp4

; ---------------------------------------------------------------------------
; Vue en perspective (15 %). Tables (copiées en RAM de cartouche au début du
; match) : y projeté et échelle x 128, indexés par le y à plat (0..255).
; _proj : A = x à plat, X = y à plat -> _proj_x, _proj_y, _proj_s
; _pscale : A = longueur -> A = longueur x échelle (sans multiplication : on
; ajoute ou retire longueur x |échelle - 128| / 128, sur 5 bits)
; ---------------------------------------------------------------------------
.export _psh_x, _psh_y
.export _proj, _proj2, _pscale, _persp, _proj_x, _proj_y, _proj_s, _proj_ram
.import popa
.segment "WBSS"
_psh_x:   .res 4         ; position projetée de chaque joueur (spr_person)
_psh_y:   .res 4
_proj_ram: .res 512 + 640   ; y projeté (256), m = échelle x 128 - 128 (256), produits (20 x 32)
.segment "CODE"
; void __fastcall__ proj2(u8 x, u8 y) : pour le C (y dans A, x sur la pile)
_proj2:
    tax
    jsr popa
_proj:
    pha
    lda _proj_ram,x
    sta _proj_y
    lda _proj_ram+256,x
    sta _proj_s
    pla
    sec
    sbc #128
    bcc @left
    jsr _pscale             ; à droite du centre : 128 + écart x échelle
    clc
    adc #128
    bcc :+
    lda #255
:   sta _proj_x
    rts
@left:
    eor #$FF                ; à gauche : 128 - écart x échelle
    clc
    adc #1
    jsr _pscale
    sta pj_v
    lda #128
    sec
    sbc pj_v
    bcs :+
    lda #0
:   sta _proj_x
    rts

_pscale:
    ; résultat = v ± round(v x |m| / 128), lu dans la table des produits
    ; (_proj_ram + 512 + |m| x 32 + v / 8) ; _proj_s : octet tassé (gen_assets)
    sta pj_v0
    lsr a
    lsr a
    lsr a
    sta pj_v                ; v / 8 (0..31)
    lda _proj_s
    and #$E0
    ora pj_v
    clc
    adc #<(_proj_ram + 512)
    sta pj_lo
    lda _proj_s
    and #3
    adc #>(_proj_ram + 512)
    sta pj_hi
    ldy #0
    lda _proj_s
    and #4
    bne @neg
    lda (pj_lo),y
    clc
    adc pj_v0
    bcc :+
    lda #255
:   rts
@neg:
    lda (pj_lo),y
    sta pj_m
    lda pj_v0
    sec
    sbc pj_m
    bcs :+
    lda #0
:   rts
