; Royal Martinot — poses des joueurs (banque 3, appelée par match_draw via
; bank_call avant de dessiner les joueurs). En assembleur : la version C
; coûtait environ 2 000 cycles par trame en double.
;  - attente : rebond d'1 px ; split-step quand l'adversaire frappe ;
;  - préparation : coup droit ou revers armé pendant que la balle arrive ;
;  - frappe : armé (coup droit, revers, extension, plongeon, volée, smash),
;    impact, puis accompagnement propre au coup (lob, amortie, volée) ;
;  - course en 4 temps, pas chassés près de la balle, glissade sur terre
;    battue et sur glace ;
;  - service : rebonds de la balle avant de servir ; receveur genoux fléchis
;    qui se balance ;
;  - après le point : joie propre au personnage (pirouette de Margaux),
;    colère après une faute, mains sur les genoux après un long échange.

.export _poses_all, _pose_now, _slide_t, _dr_on, _dr_x, _dr_y, _dr_h
.import _pl_state, _pl_timer, _pl_walk, _pl_moving, _pl_flip, _pl_jump
.import _flip_cache, _rally_hits, _hitter, _hitter_p, _b_serve, _b_live
.import _last_shot, _ms, _ms_timer, _bx, _by, _bz, _bvy, _bvz, _plx, _ply
.import _bounces, _sc_winner, _sc_server, _sc_swap, _srv_slot, _cfg, _npl
.import _end_reason, _ump_arm, _tired, _pending_winner, _sfx_bounce
.importzp _frame_cnt, tmp1, tmp2, _oam_i
.import _oam_buf, _ball_base

; poses (gfx.h)
P_IDLE = 0
P_RUN1 = 1
P_RUN2 = 2
P_SWING = 3
P_FOLLOW = 4
P_SERVE = 5
P_JOY = 6
P_SAD = 7
P_BACK = 8
P_REACH = 9
P_SRVHIT = 10
P_PREP = 11
P_IMPACT = 12
P_VOLLEY = 13
P_DROP = 14
P_DRIBBLE = 15
P_FIST = 16
P_ANGRY = 17
P_TIRED = 18
P_RUN3 = 19
P_RUN4 = 20
P_DIVE = 21
LIFT = 32
DOWN = 64
ST_CHARGE = 1
ST_SWING = 2
ST_TOSS = 3
MS_SERVE = 0
MS_TOSS = 1
MS_RALLY = 2
MS_DEAD = 3
MS_OVER = 4
END_POINT = 2
SH_LOB = 2
SH_DROP = 3
SH_VOLLEY = 6
SH_SMASH = 7
CFG_MODE = 0
CFG_DOUBLES = 1
CFG_CHR = 6
CFG_SURF = 14

.segment "WBSS"
_pose_now: .res 4
_slide_t:  .res 4
_dr_on:    .res 1
_dr_x:     .res 1
_dr_y:     .res 1
_dr_h:     .res 1
ss_hits:   .res 4
ss_t:      .res 4
imp_t:     .res 4
imp_k:     .res 4
pp:        .res 1       ; joueur
pt:        .res 1       ; équipe (p & 1)
pst:       .res 1       ; état du joueur
srv:       .res 1       ; joueur au service
bxp:       .res 1       ; balle en pixels
byp:       .res 1
dxs:       .res 1       ; balle - joueur (pixels, signé)
dys:       .res 1

.segment "RODATA3"
run4:   .byte P_RUN1, P_RUN3 | LIFT, P_RUN2, P_RUN4 | LIFT
drib_h: .byte 10, 9, 8, 6, 5, 4, 3, 1, 0, 1, 3, 4, 5, 6, 8, 9
mod3:   .byte 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1

.segment "BANK3"

; A = pixel (valeur 16 bits en virgule fixe 1/64 : octet haut de v << 2)
.macro PXA addr
    lda addr+1
    sta tmp1
    lda addr
    asl a
    rol tmp1
    asl a
    rol tmp1
    lda tmp1
.endmacro
.macro PXAY addr
    lda addr+1,y
    sta tmp1
    lda addr,y
    asl a
    rol tmp1
    asl a
    rol tmp1
    lda tmp1
.endmacro

_poses_all:
    lda #0
    sta _dr_on
    ; joueur au service
    lda _sc_server
    ldx _cfg+CFG_DOUBLES
    beq :+
    tax
    lda _srv_slot,x
    asl a
    clc
    adc _sc_server
:   sta srv
    PXA _bx
    sta bxp
    PXA _by
    sta byp
    ldx #0
@loop:
    cpx _npl
    bcc @body
    jmp pose_fx
@body:
    stx pp
    txa
    and #1
    sta pt
    lda _pl_state,x
    sta pst
    ; compteurs
    lda ss_t,x
    beq :+
    dec ss_t,x
:   lda imp_t,x
    beq :+
    dec imp_t,x
:   lda _slide_t,x
    beq :+
    dec _slide_t,x
:
    ; nouvelle frappe : impact pour le frappeur, split-step pour l'adversaire
    lda _rally_hits
    cmp ss_hits,x
    beq @norc
    sta ss_hits,x
    lda _hitter_p
    cmp pp
    bne @notme
    lda _rally_hits
    beq @norc
    lda _b_serve
    beq @imp
    lda #$FF                ; service : accompagnement normal
    sta imp_k,x
    bne @norc
@imp:
    lda #5
    sta imp_t,x
    lda _last_shot
    sta imp_k,x
    jmp @norc
@notme:
    lda _ms
    cmp #MS_RALLY
    bne @norc
    lda _hitter
    cmp pt
    beq @norc
    lda pst
    ora _pl_moving,x
    ora _pl_jump,x
    bne @norc
    lda #14
    sta ss_t,x
    lda #7
    sta _pl_jump,x
@norc:
    ; en double, chaque joueur est recalculé une trame sur deux (les
    ; compteurs avancent à chaque trame) : moitié moins de calcul
    lda _npl
    cmp #4
    bne @calc
    txa
    eor _frame_cnt
    and #1
    bne @next
@calc:
    ; miroir par défaut : celui de la frappe
    ldy #0
    lda pst
    cmp #ST_SWING
    bne :+
    ldy _pl_flip,x
:   tya
    sta _flip_cache,x
    jsr choose
    ldx pp
    sta _pose_now,x
@next:
    ldx pp
    inx
    jmp @loop

; balle qui rebondit avant le service et poussière des glissades (sprites
; écrits avant ceux des joueurs : devant eux)
pose_fx:
    lda _dr_on
    beq @nodr
    ldx _oam_i
    lda _dr_y
    sec
    sbc #4 + 1
    sec
    sbc _dr_h
    sta _oam_buf,x
    lda _ball_base
    sta _oam_buf+1,x
    lda #3
    sta _oam_buf+2,x
    lda _dr_x
    sec
    sbc #3
    sta _oam_buf+3,x
    txa
    clc
    adc #4
    sta _oam_i
@nodr:
    ldx #0
@sl:
    cpx _npl
    bcs @done
    lda _slide_t,x
    beq @nx
    stx pp
    txa
    asl a
    tay
    ldx _oam_i
    lda #6                  ; poussière : petit nuage puis nuage plus large
    ldy pp
    cmp _slide_t,y
    adc #0                  ; (retenue : slide_t <= 6 -> 7)
    sta _oam_buf+1,x
    lda pp
    asl a
    tay
    PXAY _ply
    sec
    sbc #6 + 1
    sta _oam_buf,x
    PXAY _plx
    sec
    sbc #4
    sta _oam_buf+3,x
    lda #2
    sta _oam_buf+2,x
    txa
    clc
    adc #4
    sta _oam_i
    ldx pp
@nx:
    inx
    bne @sl
@done:
    rts

; écarts balle - joueur (pixels, signés) : seulement quand ils servent
calcd:
    txa
    asl a
    tay
    PXAY _plx
    sta tmp2
    lda bxp
    sec
    sbc tmp2
    sta dxs
    PXAY _ply
    sta tmp2
    lda byp
    sec
    sbc tmp2
    sta dys
    rts

; choisit la pose du joueur pp (X = pp), résultat dans A
choose:
    lda _ms
    cmp #MS_OVER
    bne @notover
    lda pt
    cmp _sc_winner
    bne @sad
    lda _ms_timer
    jmp celebrate
@sad:
    lda #P_SAD
    rts
@notover:
    lda pst
    cmp #ST_TOSS
    bne @nottoss
    lda _bvz+1
    bmi @srvhit
    ora _bvz
    beq @srvhit
    lda #P_SERVE
    rts
@srvhit:
    lda #P_SRVHIT
    rts
@nottoss:
    cmp #ST_SWING
    beq @swing
    jmp @notswing
@swing:
    lda imp_t,x
    beq @noimp
    ; impact : geste propre au coup
    lda imp_k,x
    cmp #SH_VOLLEY
    beq @volley
    cmp #SH_SMASH
    beq @srvhit
    cmp #SH_DROP
    beq @drop
    lda #P_IMPACT
    rts
@volley:
    lda #P_VOLLEY
    rts
@drop:
    lda #P_DROP
    rts
@noimp:
    lda _pl_timer,x
    cmp #7
    bcs @armed
    ; accompagnement
    lda imp_k,x
    cmp #SH_LOB
    beq @srvhit             ; la raquette finit vers le ciel
    cmp #SH_DROP
    beq @drop
    cmp #SH_VOLLEY
    beq @volley
    lda #P_FOLLOW
    rts
@armed:
    lda _b_serve
    beq @shot
    lda _hitter
    cmp pt
    beq @srvhit
@shot:
    jsr calcd
    lda dxs
    clc
    adc #20
    cmp #40
    bcc @nodive
    lda #P_DIVE
    jmp @slide
@nodive:
    lda dxs
    clc
    adc #12
    cmp #25
    bcc @noreach
    lda #P_REACH
    jmp @slide
@noreach:
    lda _bz+1
    bmi @low
    cmp #10                 ; bz >= 40 px : smash armé
    bcc @nosmash
    lda #P_SERVE
    rts
@nosmash:
    cmp #1                  ; balle en l'air (> 4 px) avant le rebond : volée
    bcc @low
    lda _bounces
    bne @low
    lda #P_VOLLEY
    rts
@low:
    lda _pl_flip,x
    beq :+
    lda #P_BACK
    rts
:   lda #P_SWING
    rts
@slide:
    ; glissade sur terre battue et sur glace (en mouvement)
    tay
    lda _pl_moving,x
    beq @sl
    lda _cfg+CFG_SURF
    beq @sl1
    cmp #5
    bne @sl
@sl1:
    lda #8
    sta _slide_t,x
@sl:
    tya
    rts

@notswing:
    cmp #ST_CHARGE
    bne @notcharge
    lda #P_JOY
    rts
@notcharge:
    lda _pl_moving,x
    beq @still
    ; pas chassés : la balle arrive et elle est tout près
    lda _ms
    cmp #MS_RALLY
    bne @run
    lda _hitter
    cmp pt
    beq @run
    jsr calcd
    lda dxs
    clc
    adc #23
    cmp #47
    bcs @run
    lda dys
    clc
    adc #30
    cmp #61
    bcc @run
    lda _pl_walk,x
    and #4
    beq :+
    lda #P_JOY
    rts
:   lda #P_IDLE
    rts
@run:
    lda _pl_walk,x
    lsr a
    lsr a
    and #3
    tay
    lda run4,y
    rts

@still:
    ; après le point (match) : joie, fatigue, colère ou déception
    lda _ms
    cmp #MS_DEAD
    bne @notdead
    lda _end_reason
    cmp #END_POINT
    bne @notdead
    lda _cfg+CFG_MODE
    bne @notdead
    lda _ms_timer
    cmp #60
    bcs @notdead
    lda pt
    cmp _pending_winner
    bne @loser
    lda _ms_timer
    jmp celebrate
@loser:
    lda _tired
    cmp #5
    bcc :+
    lda #P_TIRED
    rts
:   lda _ump_arm
    beq @sad2
    lda _ms_timer           ; faute : la raquette tape le sol
    and #8
    beq @sad2
    lda #P_ANGRY
    rts
@sad2:
    lda #P_SAD
    rts

@notdead:
    lda _cfg+CFG_MODE
    beq :+
    jmp @nosrv
:   lda _ms
    cmp #MS_SERVE
    bne @notserve
    cpx srv
    bne @recv
    ; deux rebonds de la balle, puis elle reste dans la main
    lda #1
    sta _dr_on
    txa
    asl a
    tay
    PXAY _plx
    sec
    sbc #4
    sta _dr_x
    PXAY _ply
    sta _dr_y
    lda _frame_cnt
    and #63
    cmp #32
    bcc :+
    lda #10
    bne @dh
:   pha
    and #15
    tay
    lda drib_h,y
    sta _dr_h
    pla
    cmp #8
    beq @bnc
    cmp #24
    bne @dd
@bnc:
    jsr _sfx_bounce
    jmp @dd
@dh:
    sta _dr_h
@dd:
    lda #P_DRIBBLE
    rts
@notserve:
    cmp #MS_TOSS
    bne @nosrv
@recv:
    ; receveur : genoux fléchis, il se balance d'un pied sur l'autre
    lda pt
    cmp _sc_server
    beq @nosrv
    txa
    asl a
    asl a
    asl a
    clc
    adc _frame_cnt
    and #16
    beq :+
    lda #P_JOY | DOWN
    rts
:   lda #P_JOY
    rts
@nosrv:
    lda ss_t,x
    beq @noss
    cmp #7
    bcs :+
    lda #P_JOY
    rts
:   lda #P_IDLE
    rts
@noss:
    ; préparation : la balle arrive vers le joueur, à moins de 40 px
    lda _ms
    cmp #MS_RALLY
    bne @bob
    lda _hitter
    cmp pt
    beq @bob
    lda _b_live
    beq @bob
    jsr calcd
    lda dys
    clc
    adc #39
    cmp #79
    bcs @bob
    ; la balle va vers le joueur : vers le bas (bvy > 0) pour le joueur du bas
    lda pt
    eor _sc_swap            ; côté : 0 bas, 1 haut
    tay
    lda _bvy+1
    bmi @up
    ora _bvy
    beq @bob
    cpy #0
    bne @bob
    beq @prep
@up:
    cpy #0
    beq @bob
@prep:
    lda dxs
    bpl :+
    lda #1
    sta _flip_cache,x
    lda #P_BACK
    rts
:   lda #P_PREP
    rts
@bob:
    txa
    asl a
    asl a
    asl a
    clc
    adc _frame_cnt
    and #16
    beq :+
    lda #P_IDLE | LIFT
    rts
:   lda #P_IDLE
    rts

; joie du gagnant (X = joueur, A = minuterie) : propre au personnage ;
; Margaux fait une pirouette (miroir qui alterne)
celebrate:
    sta tmp1
    lda _cfg+CFG_CHR,x
    cmp #16
    bne @notm
    lda tmp1
    lsr a
    lsr a
    and #1
    sta _flip_cache,x
    lda #P_JOY
    rts
@notm:
    tay
    lda mod3,y
    beq @c0
    cmp #1
    beq @c1
    lda tmp1                ; poing serré, il sautille
    and #8
    asl a
    asl a
    ora #P_FIST
    rts
@c0:
    lda tmp1
    and #16
    beq :+
    lda #P_FIST
    rts
:   lda #P_JOY
    rts
@c1:
    lda tmp1
    and #16
    beq :+
    lda #P_SERVE            ; bras levés
    rts
:   lda #P_JOY
    rts
