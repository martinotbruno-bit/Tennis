; Pixel Open — affichage d'un personnage 16 x 24 (banque fixe, assembleur).
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
