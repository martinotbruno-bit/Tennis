; Royal Martinot — dérapages selon la surface (banque 3, appelé par
; match_tick via bank_call après les déplacements de tous les joueurs).
;
; Le moteur déplace les joueurs comme avant (contrôle humain, IA) ; ici, le
; déplacement voulu de la trame (position - position de la trame précédente)
; passe par la surface :
;   - glace : inertie. La vitesse rejoint le déplacement voulu d'un quart de
;     l'écart par trame : départ progressif, et le joueur continue sur sa
;     lancée (environ 6 px) quand il s'arrête ou change de sens ;
;   - terre battue : à l'arrêt et au demi-tour, glissade (le joueur garde
;     les trois quarts de sa vitesse à chaque trame, environ 6 px ; version
;     2.43, 2 px avant) ;
;   - plage : pas de glissade, mais un départ lourd (la vitesse monte de
;     50/64 px par trame au plus, comme dans le sable) ; arrêt net ;
;   - autres surfaces : rien ne change.
; Seulement pendant l'échange et le point joué. Une téléportation (mise en
; place au service) remet la vitesse à zéro. En double, chaque IA ne bouge
; qu'une trame sur deux (au double de sa vitesse) : ses trames de repos sont
; ignorées et son accélération dans le sable est doublée.
; Arrêt brusque ou demi-tour : poussière (terre, plage) ou éclats de glace
; aux pieds, et sur terre et glace, une petite trace à l'endroit du départ
; de la glissade (pose_fx, src/poses.s).

.export _slip_tick, _sl_ox, _sl_oy
.import _plx, _ply, _ms, _cfg, _npl, _slide_t
.importzp _frame_cnt

CFG_CTRL = 2
CFG_SURF = 14
CTRL_CPU = 2
MS_RALLY = 2
MS_DEAD = 3

.segment "WBSS"
sp_x:   .res 8          ; position de la trame précédente (16 bits)
sp_y:   .res 8
sv_x:   .res 8          ; vitesse (16 bits, 1/64 px par trame)
sv_y:   .res 8
_sl_ox: .res 4          ; départ de la glissade (pixels, pour la trace)
_sl_oy: .res 4
kind:   .res 1          ; 0 rien, 1 terre, 2 sable, 3 glace
dbl:    .res 1          ; IA de double (trame de mouvement)
pi:     .res 1          ; joueur
pi2:    .res 1          ; joueur x 2
evt:    .res 1
P:      .res 2          ; position
Q:      .res 2          ; position précédente
V:      .res 2          ; vitesse
VO:     .res 2          ; vitesse avant
D:      .res 2          ; déplacement voulu
E:      .res 2

.segment "BANK3"

_slip_tick:
    lda _cfg+CFG_SURF
    ldx #0
    cmp #0
    bne :+
    ldx #1                  ; terre battue
:   cmp #4
    bne :+
    ldx #2                  ; plage
:   cmp #5
    bne :+
    ldx #3                  ; glace
:   stx kind
    ldx #0
@loop:
    cpx _npl
    bcc :+
    rts
:   stx pi
    txa
    asl a
    sta pi2
    ; hors échange, autre surface : on suit le joueur, vitesse nulle
    lda kind
    beq @reset
    lda _ms
    cmp #MS_RALLY
    beq @act
    cmp #MS_DEAD
    beq @act
@reset:
    jsr follow
    jmp @next
@act:
    ; IA de double : ne bouge qu'une trame sur deux
    lda #0
    sta dbl
    lda _npl
    cmp #4
    bne @single
    ldx pi
    lda _cfg+CFG_CTRL,x
    cmp #CTRL_CPU
    bne @single
    lda _frame_cnt
    eor pi
    and #1
    beq :+
    jmp @next               ; trame de repos : rien ne bouge
:   inc dbl
@single:
    lda #0
    sta evt
    ; --- axe x
    ldy pi2
    lda _plx,y
    sta P
    lda _plx+1,y
    sta P+1
    lda sp_x,y
    sta Q
    lda sp_x+1,y
    sta Q+1
    lda sv_x,y
    sta V
    lda sv_x+1,y
    sta V+1
    jsr axis
    bcc :+
    jmp @tele
:   ldy pi2
    lda P
    sta _plx,y
    lda P+1
    sta _plx+1,y
    lda V
    sta sv_x,y
    lda V+1
    sta sv_x+1,y
    ; --- axe y
    lda _ply,y
    sta P
    lda _ply+1,y
    sta P+1
    lda sp_y,y
    sta Q
    lda sp_y+1,y
    sta Q+1
    lda sv_y,y
    sta V
    lda sv_y+1,y
    sta V+1
    jsr axis
    bcs @tele
    ldy pi2
    lda P
    sta _ply,y
    lda P+1
    sta _ply+1,y
    lda V
    sta sv_y,y
    lda V+1
    sta sv_y+1,y
    jsr clampxy
    ; arrêt brusque ou demi-tour : poussière, éclats, trace
    lda evt
    beq @save
    ldx pi
    lda _slide_t,x
    bne @save
    lda #8
    sta _slide_t,x
    ldy pi2
    lda _plx+1,y            ; pixels = octet haut de la position << 2
    sta E+1
    lda _plx,y
    asl a
    rol E+1
    asl a
    rol E+1
    lda E+1
    sta _sl_ox,x
    lda _ply+1,y
    sta E+1
    lda _ply,y
    asl a
    rol E+1
    asl a
    rol E+1
    lda E+1
    sta _sl_oy,x
@save:
    jsr follow_pos
    jmp @next
@tele:
    jsr follow
@next:
    ldx pi
    inx
    jmp @loop

; position précédente = position actuelle, vitesse nulle
follow:
    ldy pi2
    lda #0
    sta sv_x,y
    sta sv_x+1,y
    sta sv_y,y
    sta sv_y+1,y
follow_pos:
    ldy pi2
    lda _plx,y
    sta sp_x,y
    lda _plx+1,y
    sta sp_x+1,y
    lda _ply,y
    sta sp_y,y
    lda _ply+1,y
    sta sp_y+1,y
    rts

; un axe : P (voulu), Q (précédent), V (vitesse) -> P (réel), V ;
; retenue à 1 : téléportation
axis:
    lda V
    sta VO
    lda V+1
    sta VO+1
    ; D = P - Q
    sec
    lda P
    sbc Q
    sta D
    lda P+1
    sbc Q+1
    sta D+1
    ; téléportation : |D| > 320 (5 px)
    bpl @dpos
    lda D
    cmp #<(-320)
    lda D+1
    sbc #>(-320)
    bvc :+
    eor #$80
:   bpl @ok
    sec
    rts
@dpos:
    lda D
    cmp #<321
    lda D+1
    sbc #>321
    bvc :+
    eor #$80
:   bmi @ok
    sec
    rts
@ok:
    lda kind
    cmp #3
    bne :+
    jmp @ice
:   cmp #2
    bne :+
    jmp @sand
:
    ; --- terre battue : glissade à l'arrêt et au demi-tour (le joueur garde
    ; les trois quarts de sa vitesse à chaque trame, environ 6 px)
    lda D
    ora D+1
    beq @glide
    lda D+1
    eor V+1
    bpl @take               ; même sens : il suit la croix
    jsr bigv                ; demi-tour lancé (|V| >= 48) : il glisse d'abord
    bcc @take
@glide:
    ; V = V - V/4 (décalage arithmétique)
    lda V+1
    cmp #$80
    ror a
    sta E+1
    lda V
    ror a
    sta E
    lda E+1
    cmp #$80
    ror E+1
    ror E
    sec
    lda V
    sbc E
    sta V
    lda V+1
    sbc E+1
    sta V+1
    jsr small0              ; |V| < 16 : arrêt
    jmp @apply
@take:
    lda D
    sta V
    lda D+1
    sta V+1
    jmp @apply
@sand:
    lda D
    ora D+1
    bne :+
    sta V                   ; arrêt net
    sta V+1
    jmp @apply
:   ; E = D - V, borné à ±50 (±100 pour une IA de double)
    sec
    lda D
    sbc V
    sta E
    lda D+1
    sbc V+1
    sta E+1
    ldx #50
    lda dbl
    beq :+
    ldx #100
:   stx tmpacc
    lda E+1
    bmi @eneg
    bne @eclampp
    lda E
    cmp tmpacc
    bcc @eadd
@eclampp:
    lda tmpacc
    sta E
    lda #0
    sta E+1
    beq @eadd
@eneg:
    cmp #$FF
    bne @eclampn
    lda #0
    sec
    sbc tmpacc              ; -acc (octet bas)
    cmp E
    bcc @eadd               ; -acc < E : dans les bornes
    beq @eadd
@eclampn:
    lda #0
    sec
    sbc tmpacc
    sta E
    lda #$FF
    sta E+1
@eadd:
    clc
    lda V
    adc E
    sta V
    lda V+1
    adc E+1
    sta V+1
    jmp @apply
@ice:
    ; V += (D - V) / 4
    sec
    lda D
    sbc V
    sta E
    lda D+1
    sbc V+1
    sta E+1
    cmp #$80
    ror E+1
    ror E
    lda E+1
    cmp #$80
    ror E+1
    ror E
    clc
    lda V
    adc E
    sta V
    lda V+1
    adc E+1
    sta V+1
    lda D
    ora D+1
    bne @apply
    jsr small0              ; plus de poussée et presque arrêté : arrêt
@apply:
    ; P = Q + V
    clc
    lda Q
    adc V
    sta P
    lda Q+1
    adc V+1
    sta P+1
    ; événement : vitesse d'avant >= 48 et (arrêt ou sens contraire)
    lda VO+1
    bmi @voneg
    bne @vobig
    lda VO
    cmp #48
    bcc @noevt
@vobig:
    lda D+1
    bmi @evt                ; sens contraire
    ora D
    beq @evt                ; arrêt
    bne @noevt
@voneg:
    cmp #$FF
    bne @vobign
    lda VO
    cmp #<(-47)
    bcs @noevt
@vobign:
    lda D+1
    bpl @evt                ; arrêt ou sens contraire (D >= 0)
    bmi @noevt
@evt:
    lda #1
    sta evt
@noevt:
    clc
    rts

; retenue à 1 si |V| >= 48
bigv:
    lda V+1
    bmi @neg
    bne @yes
    lda V
    cmp #48
    rts
@neg:
    cmp #$FF
    bne @yes
    lda V
    cmp #<(-47)             ; V <= -48 : octet bas <= $D0
    bcc @yes
    clc
    rts
@yes:
    sec
    rts

; |V| < 16 : V = 0
small0:
    lda V+1
    beq @pos
    cmp #$FF
    bne @no
    lda V
    cmp #<(-15)
    bcc @no
    bcs @zero
@pos:
    lda V
    cmp #16
    bcs @no
@zero:
    lda #0
    sta V
    sta V+1
@no:
    rts

; bornes du joueur (comme clamp_player) : x 48..208, y de son côté du filet
clampxy:
    ldy pi2
    ; x
    lda _plx+1,y
    cmp #>(48 * 64)
    bcs :+
    lda #<(48 * 64)
    sta _plx,y
    lda #>(48 * 64)
    sta _plx+1,y
:   lda _plx+1,y
    cmp #>(208 * 64)
    bcc :+
    lda #<(208 * 64)
    sta _plx,y
    lda #>(208 * 64)
    sta _plx+1,y
:   ; y : côté du filet d'après la position précédente
    lda sp_y+1,y
    cmp #>(132 * 64)
    bcs @bottom
    ; haut : 30 à 126 (comparaison sur 16 bits : 126 x 64 = $1F80)
    lda _ply,y
    cmp #<(126 * 64 + 1)
    lda _ply+1,y
    sbc #>(126 * 64 + 1)
    bcc :+
    lda #<(126 * 64)
    sta _ply,y
    lda #>(126 * 64)
    sta _ply+1,y
:   lda _ply+1,y
    cmp #>(30 * 64)
    bcs :+
    lda #<(30 * 64)
    sta _ply,y
    lda #>(30 * 64)
    sta _ply+1,y
:   rts
@bottom:
    ; bas : 142 à 228
    lda _ply+1,y
    cmp #>(142 * 64)
    bcs :+
    lda #<(142 * 64)
    sta _ply,y
    lda #>(142 * 64)
    sta _ply+1,y
:   lda _ply+1,y
    cmp #>(228 * 64)
    bcc :+
    lda #<(228 * 64)
    sta _ply,y
    lda #>(228 * 64)
    sta _ply+1,y
:   rts

.segment "WBSS"
tmpacc: .res 1
