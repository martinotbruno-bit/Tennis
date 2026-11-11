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
.importzp _meta_x, _meta_y, _meta_tile, _meta_attr, _meta_t1, _meta_t2, _oam_i
.import _person_map, _person_row, _prof_map, _char_female
.import _marg_map, _marg_prof_l, _marg_prof_r
.import _sfx_step
.import _ms_timer, _confetti_tile
.export _win_fx
.import incsp8
.importzp sp, tmp1, tmp2, tmp3

POSE_RUN1 = 1
POSE_RUN2 = 2
POSE_LIFT = 16          ; drapeau : personnage relevé d'1 px
NPOSE     = 11
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

    ; drapeau POSE_LIFT : 1 px plus haut (rebond d'attente, foulée) ; la
    ; position mémorisée pour la course reste celle du sol
    lda ppose
    and #POSE_LIFT
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
    beq :+
    txa
    beq :+
    dex
:   stx _meta_y

    ; tuiles : person_map[person_row[(femme << 1) | face] + pose * 3 + rangée]
    ldx pc
    lda _char_female,x
    asl a
    ora pfront
    tax
    lda ppose
    asl a
    clc
    adc ppose
    adc _person_row,x
    tay
    lda _person_map,y
    sta ptile
    lda _person_map+1,y
    sta pt1
    lda _person_map+2,y
    sta pt2

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
    ; profil : prof_map[(femme ? 6 : 0) + (course B ? 3 : 0) + rangée]
    lda #0
    ldy _char_female,x
    beq @pm
    lda #6
@pm:
    ldy ppose
    cpy #POSE_RUN2
    bne @pa
    clc
    adc #3
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
    lda ppose
    cmp #POSE_RUN1
    beq @prof
    cmp #POSE_RUN2
    beq @prof
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
    beq @tobass
    dec _mel_timer
    beq @mnext
@tobass:
    jmp @bass
@mnext:
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
    ; $4007 à chaque note : relance identique pour toutes les notes (ne le
    ; réécrire que quand l'octet change faisait un clic sur certaines notes
    ; seulement, entendu comme des ratés)
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
; void ump_draw(void) : petit arbitre de chaise, assis de profil sur la
; chaise du décor (x 32-47, y 112-143, centrée sur le filet), tourné vers le court. Tête 11 (regard
; vers le bas du court) ou 12 (vers le haut), selon la balle ; buste 13 au
; repos ou coupe + 0 bras tendu, avec la main (coupe + 1) ; bras levé (coupe
; + 2) sur un let (il s'agite) et à la fin du match (il salue). Appelé aussi
; pendant les ralentis. En assembleur : peu coûteux à chaque trame.
.export _ump_draw
.import _ms, _end_reason, _ump_arm, _trophy_tile, _by, _ump_princess
.importzp _frame_cnt
.importzp tmp4
UMP_X = 34                  ; au bord du court, contre le poteau du filet
UMP_Y = 116                 ; arbitre et chaise centrés sur le filet

.segment "CODE"
_ump_draw:
    lda #11                 ; tête : regard vers le bas du court
    ldy _by+1
    cpy #$21                ; by < NET_Y * FP (132 * 64 = $2100) : moitié du haut
    bcs @hd
    lda #12
@hd:
    sta tmp4
    lda #13
    sta tmp1                ; buste
    lda #0
    sta tmp2                ; 3e sprite : 0 aucun, 1 main, 2 bras levé
    lda _ms
    cmp #4                  ; MS_OVER : bras levé qui salue
    beq @up
    cmp #3                  ; MS_DEAD
    bne @draw
    lda _end_reason
    cmp #3                  ; END_LET : bras levé qui s'agite
    beq @up
    lda _ump_arm            ; faute, balle dehors : bras tendu
    beq @draw
    lda _ms_timer
    cmp #16
    bcc @draw
    lda _trophy_tile
    sta tmp1
    lda #1
    sta tmp2
    bne @draw
@up:
    lda #2
    sta tmp2
@draw:
    ldx _oam_i
    lda #UMP_Y - 1          ; tête (le PPU affiche une ligne plus bas)
    sta _oam_buf,x
    lda tmp4
    sta _oam_buf+1,x
    lda #2
    sta _oam_buf+2,x
    lda #UMP_X
    sta _oam_buf+3,x
    inx
    inx
    inx
    inx
    lda #UMP_Y + 8 - 1      ; buste
    sta _oam_buf,x
    lda tmp1
    sta _oam_buf+1,x
    lda #2
    sta _oam_buf+2,x
    lda #UMP_X
    sta _oam_buf+3,x
    inx
    inx
    inx
    inx
    lda tmp2
    beq @end
    cmp #1
    bne @raise
    lda #UMP_Y + 8 - 1      ; main du bras tendu
    sta _oam_buf,x
    lda _trophy_tile
    clc
    adc #1
    sta _oam_buf+1,x
    lda #UMP_X + 8
    sta _oam_buf+3,x
    jmp @attr
@raise:
    lda _frame_cnt          ; s'agite / salue : monte et descend d'un pixel
    lsr a
    lsr a
    lsr a
    and #1
    eor #$FF
    sec
    adc #UMP_Y - 2          ; UMP_Y - 1 - 1 - bob
    sta _oam_buf,x
    lda _trophy_tile
    clc
    adc #2
    sta _oam_buf+1,x
    lda #UMP_X + 2          ; contre l'épaule, devant le visage
    sta _oam_buf+3,x
@attr:
    lda #2
    sta _oam_buf+2,x
    inx
    inx
    inx
    inx
@end:
    ; princesse au repos : baguette magique tenue devant elle (4e tuile)
    lda _ump_princess
    beq @fin
    lda tmp2
    bne @fin
    lda #UMP_Y + 3 - 1
    sta _oam_buf,x
    lda _trophy_tile
    clc
    adc #3
    sta _oam_buf+1,x
    lda #2
    sta _oam_buf+2,x
    lda #UMP_X + 6
    sta _oam_buf+3,x
    inx
    inx
    inx
    inx
@fin:
    stx _oam_i
    rts

; ---------------------------------------------------------------------------
; u8 __fastcall__ pose_of(u8 p) : pose d'un joueur pour la trame (match_draw).
;  - attente : rebond d'1 px toutes les 16 trames (décalé de 8 par joueur) ;
;  - split-step : quand l'adversaire frappe, un joueur à l'arrêt fait un
;    petit saut (pl_jump = 7) puis se réceptionne appuis écartés (7 trames) ;
;  - course en 4 temps : foulées A et B, chacune basse puis relevée d'1 px ;
;  - service en 3 temps : lancer (la balle monte), impact bras tendu (la
;    balle redescend, puis 6 trames après la frappe), accompagnement ;
;  - frappe : coup droit, revers à deux mains (balle à gauche, sprite en
;    miroir) ou extension (balle à plus de 12 px), puis accompagnement.
; En assembleur : appelée 2 ou 4 fois par trame (la version C coûtait
; environ 140 cycles par appel).
; ---------------------------------------------------------------------------
.export _pose_of
.import _pl_state, _pl_timer, _pl_walk, _pl_moving, _pl_flip, _pl_jump
.import _rally_hits, _ms_timer, _hitter, _b_serve, _bx, _plx, _bvz
.import _sc_winner, _pending_winner, _cfg
.importzp _frame_cnt

P_IDLE   = 0
P_RUN1   = 1
P_RUN2   = 2
P_SWING  = 3
P_FOLLOW = 4
P_SERVE  = 5
P_JOY    = 6
P_SAD    = 7
P_BACK   = 8
P_REACH  = 9
P_SRVHIT = 10
MS_RALLY = 2
MS_DEAD  = 3
MS_OVER  = 4
ST_CHARGE = 1
ST_SWING  = 2
ST_TOSS   = 3
END_POINT = 2
REACH_FP  = 13 * 64     ; distance balle - joueur au-delà de 12 px

.segment "WBSS"
ss_hits: .res 4
ss_t:    .res 4
pteam:   .res 1

.segment "CODE"
_pose_of:
    tax                     ; X = p
    and #1
    sta pteam
    ; split-step : compte à rebours, puis détection d'une nouvelle frappe
    lda ss_t,x
    beq :+
    dec ss_t,x
:   lda _rally_hits
    cmp ss_hits,x
    beq @nosplit
    sta ss_hits,x
    lda _ms
    cmp #MS_RALLY
    bne @nosplit
    lda _hitter
    cmp pteam
    beq @nosplit
    lda _pl_state,x
    ora _pl_moving,x
    ora _pl_jump,x
    bne @nosplit
    lda #14
    sta ss_t,x
    lda #7
    sta _pl_jump,x
@nosplit:
    ; fin du match : joie (alternée avec les bras levés) ou déception
    lda _ms
    cmp #MS_OVER
    bne @play
    lda pteam
    cmp _sc_winner
    bne @sad
    lda _ms_timer
    and #16
    bne @serve
@joy:
    lda #P_JOY
    rts
@sad:
    lda #P_SAD
    rts
@serve:
    lda #P_SERVE
    rts
@play:
    ldy _pl_state,x
    cpy #ST_TOSS
    bne @notoss
    ; lancer : bras levés tant que la balle monte, puis impact
    lda _bvz+1
    bmi @srvhit
    ora _bvz
    bne @serve
@srvhit:
    lda #P_SRVHIT
    rts
@notoss:
    cpy #ST_SWING
    bne @noswing
    lda _pl_timer,x
    cmp #7
    bcs @armed
    lda #P_FOLLOW
    rts
@armed:
    lda _b_serve
    beq @shot
    lda _hitter
    cmp pteam
    beq @srvhit
@shot:
    ; extension si |bx - plx| >= 13 px (en virgule fixe, 1 px = 64)
    txa
    asl a
    tay
    sec
    lda _bx
    sbc _plx,y
    sta tmp1
    lda _bx+1
    sbc _plx+1,y
    bpl @dpos
    ; d < 0 : d <= -REACH_FP ?
    cmp #>(-REACH_FP)
    bcc @reach
    bne @fore
    lda tmp1
    cmp #<(-REACH_FP)
    bcc @reach
    beq @reach
    bcs @fore
@dpos:
    cmp #>REACH_FP
    bcc @fore
    bne @reach
    lda tmp1
    cmp #<REACH_FP
    bcs @reach
@fore:
    lda _pl_flip,x
    beq :+
    lda #P_BACK
    rts
:   lda #P_SWING
    rts
@reach:
    lda #P_REACH
    rts
@noswing:
    cpy #ST_CHARGE
    beq @joy2
    lda _pl_moving,x
    beq @still
    ; course : foulée A ou B (pl_walk & 8), relevée d'1 px sur pl_walk & 4
    lda _pl_walk,x
    and #4
    asl a
    asl a                   ; 16 = POSE_LIFT
    sta tmp1
    lda _pl_walk,x
    and #8
    beq :+
    lda #P_RUN1
    ora tmp1
    rts
:   lda #P_RUN2
    ora tmp1
    rts
@still:
    ; point gagné / perdu en match : joie ou déception pendant 1 s
    lda _ms
    cmp #MS_DEAD
    bne @idle
    lda _end_reason
    cmp #END_POINT
    bne @idle
    lda _cfg                ; cfg.mode == GM_MATCH (0)
    bne @idle
    lda _ms_timer
    cmp #60
    bcs @idle
    lda pteam
    cmp _pending_winner
    beq @joy2
    lda #P_SAD
    rts
@idle:
    lda ss_t,x
    beq @bob
    cmp #7
    bcc @joy2               ; réception du split-step, appuis écartés
    lda #P_IDLE
    rts
@joy2:
    lda #P_JOY
    rts
@bob:
    txa
    asl a
    asl a
    asl a
    clc
    adc _frame_cnt
    and #16                 ; 16 = POSE_LIFT
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
