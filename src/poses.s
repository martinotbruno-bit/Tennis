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
.import _oam_buf, _ball_base, _proj, _sl_ox, _sl_oy, _ice_tile
.import _pose_set, _pose_fb, _swap_on, _pl_btn
.importzp _persp, _proj_x, _proj_y

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
P_BPREP = 22
P_BIMPACT = 23
P_BFOLLOW = 24
P_BSLICE = 25
P_BVOLLEY = 26
SH_SLICE = 1
PAD_B = $40
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
dive_t:    .res 4       ; plongeon en cours (trames)
dive_f:    .res 4       ; son miroir (balle à gauche)
fbv:       .res 1
dive_x:    .res 8       ; position du plongeur (16 bits) : il vole, puis reste au sol
dive_y:    .res 8
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
:   lda dive_t,x
    beq :+
    dec dive_t,x
    jsr dive_step           ; plongeon : élan, vol, réception, au sol, relevé
:
    ; nouvelle frappe : impact pour le frappeur, split-step pour l'adversaire
    lda _rally_hits
    cmp ss_hits,x
    bne :+
    jmp @norc
:   sta ss_hits,x
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
    ; balle frappée loin du corps (20 px ou plus sur le côté) : plongeon
    ; (voir dive_step)
    jsr calcd
    ldx pp
    lda dxs
    clc
    adc #19
    cmp #39
    bcc @norc
    lda #31
    sta dive_t,x
    lda dxs
    rol a                   ; retenue = signe : balle à gauche -> miroir
    lda #0
    rol a
    sta dive_f,x
    txa
    asl a
    tay
    lda _plx,y
    sta dive_x,y
    lda _plx+1,y
    sta dive_x+1,y
    lda _ply,y
    sta dive_y,y
    lda _ply+1,y
    sta dive_y+1,y
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
    jsr fallback            ; jeu de tuiles pas encore chargé : pose de secours
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
    bcc :+
    rts
:   lda _slide_t,x
    bne :+
    sta _sl_ox,x            ; effet fini : plus de trace
    jmp @nx
:   stx pp
    txa
    asl a
    tay
    PXAY _ply
    sta tmp2                ; pieds (y à plat)
    PXAY _plx               ; A = x à plat
    ldy _persp
    beq :+
    ldx tmp2
    jsr _proj               ; vue en perspective
    lda _proj_y
    sta tmp2
    lda _proj_x
:   ldx _oam_i
    sec
    sbc #4
    sta _oam_buf+3,x
    lda tmp2
    sec
    sbc #6 + 1
    sta _oam_buf,x
    jsr fxtile              ; poussière (terre, sable) ou éclats (glace)
    sta _oam_buf+1,x
    lda #2
    sta _oam_buf+2,x
    txa
    clc
    adc #4
    sta _oam_i
    ; trace au départ de la glissade (terre battue, glace), début de l'effet
    ldx pp
    lda _slide_t,x
    cmp #5
    bcc @notr
    lda _cfg+CFG_SURF
    beq @tr
    cmp #5
    bne @notr
@tr:
    lda _sl_oy,x
    sta tmp2
    lda _sl_ox,x
    beq @notr               ; (pas de départ connu : glissade de frappe)
    ldy _persp
    beq :+
    ldx tmp2
    jsr _proj
    lda _proj_y
    sta tmp2
    lda _proj_x
:   ldx _oam_i
    sec
    sbc #4
    sta _oam_buf+3,x
    lda tmp2
    sec
    sbc #6 + 1
    sta _oam_buf,x
    lda _cfg+CFG_SURF
    beq :+
    lda _ice_tile           ; glace : rayure blanche
    bne :++
:   lda #6                  ; terre : petit nuage
:   sta _oam_buf+1,x
    lda #2
    sta _oam_buf+2,x
    txa
    clc
    adc #4
    sta _oam_i
@notr:
    ldx pp
@nx:
    inx
    jmp @sl

; plongeon du joueur X, une fois par trame (dive_t vient d'être décrémenté) :
;   30-28 élan (fente) ; 27-21 vol : le joueur avance de 1,25 px par trame
;   vers la balle (9 px), corps à l'horizontale, décollé du sol (pl_jump :
;   6 px au plus haut) ; 18 réception (poussière, éclats ou sable) ;
;   17-10 au sol ; 9-6 relevé.
; Jusqu'au relevé, le joueur ne peut pas bouger : sa position est celle du
; plongeon (le prix d'une balle rattrapée).
dive_step:
    lda dive_t,x
    cmp #27
    bne :+
    lda #10
    sta _pl_jump,x          ; envol : 6 px au plus haut, retombe en 10 trames
:   lda dive_t,x
    cmp #21
    bcc @nofly
    cmp #28
    bcs @nofly
    txa
    asl a
    tay
    lda dive_f,x
    bne @dl
    clc
    lda dive_x,y
    adc #80
    sta dive_x,y
    lda dive_x+1,y
    adc #0
    sta dive_x+1,y
    cmp #>(208 * 64)
    bcc @nofly
    lda #<(208 * 64)
    sta dive_x,y
    lda #>(208 * 64)
    sta dive_x+1,y
    bne @nofly
@dl:
    sec
    lda dive_x,y
    sbc #80
    sta dive_x,y
    lda dive_x+1,y
    sbc #0
    sta dive_x+1,y
    cmp #>(48 * 64)
    bcs @nofly
    lda #<(48 * 64)
    sta dive_x,y
    lda #>(48 * 64)
    sta dive_x+1,y
@nofly:
    lda dive_t,x
    cmp #18
    bne @noland
    lda #0
    sta _sl_ox,x            ; (pas de trace de glissade)
    lda _cfg+CFG_SURF
    beq @land
    cmp #4
    beq @land
    cmp #5
    bne @noland
@land:
    lda #10
    sta _slide_t,x          ; réception : poussière, sable ou éclats
@noland:
    lda dive_t,x
    cmp #6
    bcc @free
    txa
    asl a
    tay
    lda dive_x,y
    sta _plx,y
    lda dive_x+1,y
    sta _plx+1,y
    lda dive_y,y
    sta _ply,y
    lda dive_y+1,y
    sta _ply+1,y
@free:
    rts

; pose A (avec ses drapeaux) affichable ? Si elle fait partie d'un jeu de
; tuiles par phase qui n'est pas entièrement chargé (swap_on, extras.c),
; on montre sa pose de secours (pose_fb), avec les mêmes drapeaux
fallback:
    sta fbv
    and #31
    tay
    lda _pose_set,y
    beq @ok
    cmp _swap_on
    beq @ok
    lda fbv
    and #$E0
    ora _pose_fb,y
    rts
@ok:
    lda fbv
    rts

; tuile de l'effet de glissade du joueur pp : éclats sur glace et grains de
; sable (15),
; sinon petit nuage puis nuage plus large (6, 7)
fxtile:
    lda _cfg+CFG_SURF
    cmp #4                  ; plage : grains de sable clairs (la poussière
    beq :+                  ; beige se perdrait sur le sable jaune)
    cmp #5
    bne :++
:   lda _ice_tile
    rts
:   lda #6
    ldy pp
    cmp _slide_t,y
    adc #0                  ; (retenue : slide_t <= 6 -> 7)
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
    ; plongeon (dive_t de 30 à 1) : élan en fente (30-28), vol puis au sol
    ; (27-10), relevé en fente (9-6) ; ensuite le jeu reprend
    lda dive_t,x
    cmp #6
    bcc @nodive2
    lda dive_f,x
    sta _flip_cache,x
    lda dive_t,x
    cmp #28
    bcs @dreach
    cmp #10
    bcc @dreach
    lda #P_DIVE
    rts
@dreach:
    lda #P_REACH
    rts
@nodive2:
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
    ; revers (balle à gauche) : à deux mains, sauf le coupé (à une main :
    ; le geste du coup droit vu en miroir)
    ldy _pl_flip,x
    beq @fimp
    cmp #SH_SLICE
    beq @fimp
    lda #P_BIMPACT
    rts
@fimp:
    lda #P_IMPACT
    rts
@volley:
    ldy _pl_flip,x
    beq :+
    lda #P_BVOLLEY          ; volée de revers, deux mains
    rts
:   lda #P_VOLLEY
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
    ldy _pl_flip,x          ; fin du revers à deux mains (sauf coupé)
    beq @ffol
    cmp #SH_SLICE
    beq @ffol
    lda #P_BFOLLOW
    rts
@ffol:
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
    adc #12
    cmp #25
    bcc @noreach
    lda #P_REACH
    jmp @slide
@noreach:
    lda _bz+1
    bmi @low
    cmp #6                  ; bz >= 24 px avant le rebond : smash armé
    bcc @nosmash            ; (geste du service)
    ldy _bounces
    bne @low
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
    lda _pl_btn,x           ; revers coupé (B) : à une main, raquette haute
    cmp #PAD_B
    bne @bk2
    lda #P_BSLICE
    rts
@bk2:
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
    cmp #4                  ; plage : gerbe de sable
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
    lda _persp              ; vue en perspective : main projetée
    beq :+
    ldx _dr_y
    lda _dr_x
    jsr _proj
    lda _proj_x
    sta _dr_x
    lda _proj_y
    sta _dr_y
    ldx pp
:   lda _frame_cnt
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
    lda #P_BPREP            ; préparation du revers
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

; ---------------------------------------------------------------------------
; void ump_draw(void) : petit arbitre de chaise, assis de profil sur la
; chaise du décor (x 32-47, y 112-143, centrée sur le filet), tourné vers le court. Tête 11 (regard
; vers le bas du court) ou 12 (vers le haut), selon la balle ; buste 13 au
; repos ou coupe + 0 bras tendu, avec la main (coupe + 1) ; bras levé (coupe
; + 2) sur un let (il s'agite) et à la fin du match (il salue). Appelé aussi
; pendant les ralentis. En assembleur : peu coûteux à chaque trame.
.export _ump_draw
.import _trophy_tile, _ump_princess
.importzp tmp4
UMP_X = 34                  ; au bord du court, contre le poteau du filet
UMP_Y = 116                 ; arbitre et chaise centrés sur le filet

.segment "BANK3"
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

