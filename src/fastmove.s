; Royal Martinot — déplacement d'un joueur vers un point (version 2.59).
;
; void move_toward(u8 p, s16 tx, s16 ty, u8 speed)   (banque fixe)
;
; Réécriture en assembleur de la fonction C du moteur (game.c) : appelée à
; chaque trame pour chaque joueur de l'ordinateur, elle coûtait environ 500
; cycles en C (tableaux 16 bits indexés) ; ici, moins de 150. Même calcul :
;   dx = tx - plx[p], dy = ty - ply[p] ;
;   en diagonale (|dx| et |dy| > 2 px) : vitesse x 3/4 sur chaque axe ;
;   sur chaque axe : on avance de la vitesse vers la cible, ou on s'y pose
;   si elle est plus près ; pl_moving[p] = 1 si le joueur a bougé.
; Appel cc65 (fastcall) : speed dans A ; sur la pile C : ty (sp+0), tx
; (sp+2), p (sp+4) ; 5 octets à retirer.

.export _move_toward
.import _plx, _ply, _pl_moving, incsp5
.importzp sp, ptr1, ptr2, tmp1, tmp2, tmp3

.segment "CODE"

; C = 1 si |v| > 128 (2 px) ; v : A (octet bas), Y (octet haut)
abs_gt2px:
        cpy #$80
        bcs @neg
        cpy #0
        bne @yes
        cmp #129                ; v >= 129 ?
        rts
@neg:   cpy #$FF
        bne @yes
        cmp #$80                ; v <= -129 (octet bas < $80) ?
        bcc @yes
        clc
        rts
@yes:   sec
        rts

_move_toward:
        sta tmp1                ; s : vitesse
        ldy #4
        lda (sp),y
        sta tmp2                ; p
        asl a
        tax                     ; X = 2p
        ldy #2                  ; dx = tx - plx[p]
        sec
        lda (sp),y
        sbc _plx,x
        sta ptr1
        iny
        lda (sp),y
        sbc _plx+1,x
        sta ptr1+1
        ldy #0                  ; dy = ty - ply[p]
        sec
        lda (sp),y
        sbc _ply,x
        sta ptr2
        iny
        lda (sp),y
        sbc _ply+1,x
        sta ptr2+1
        ; diagonale : s = (s + 2s) / 4
        lda ptr1
        ldy ptr1+1
        jsr abs_gt2px
        bcc @nodiag
        lda ptr2
        ldy ptr2+1
        jsr abs_gt2px
        bcc @nodiag
        lda tmp1
        asl a
        sta tmp3
        lda #0
        rol a                   ; A:tmp3 = 2s
        tay
        lda tmp3
        clc
        adc tmp1
        sta tmp3
        tya
        adc #0                  ; A:tmp3 = 3s (10 bits)
        lsr a
        ror tmp3
        lsr a
        ror tmp3
        lda tmp3
        sta tmp1                ; s = 3s / 4
@nodiag:
        ldy tmp2
        lda #0
        sta _pl_moving,y
        ; ---- axe x ----
        lda ptr1+1
        bmi @xneg
        bne @xplus              ; dx >= 256 > s
        lda tmp1
        cmp ptr1                ; s < dx ?
        bcs @xset
@xplus: clc                     ; plx += s
        lda _plx,x
        adc tmp1
        sta _plx,x
        bcc :+
        inc _plx+1,x
:       jmp @xmoved
@xneg:  clc                     ; dx < -s  <=>  dx + s < 0
        lda ptr1
        adc tmp1
        lda ptr1+1
        adc #0
        bpl @xset
        sec                     ; plx -= s
        lda _plx,x
        sbc tmp1
        sta _plx,x
        bcs @xmoved
        dec _plx+1,x
@xmoved:
        lda #1
        ldy tmp2
        sta _pl_moving,y
        jmp @yaxis
@xset:  ldy #2                  ; plx = tx
        lda (sp),y
        sta _plx,x
        iny
        lda (sp),y
        sta _plx+1,x
        ; ---- axe y ----
@yaxis: lda ptr2+1
        bmi @yneg
        bne @yplus
        lda tmp1
        cmp ptr2
        bcs @yset
@yplus: clc
        lda _ply,x
        adc tmp1
        sta _ply,x
        bcc :+
        inc _ply+1,x
:       jmp @ymoved
@yneg:  clc
        lda ptr2
        adc tmp1
        lda ptr2+1
        adc #0
        bpl @yset
        sec
        lda _ply,x
        sbc tmp1
        sta _ply,x
        bcs @ymoved
        dec _ply+1,x
@ymoved:
        lda #1
        ldy tmp2
        sta _pl_moving,y
        jmp incsp5
@yset:  ldy #0
        lda (sp),y
        sta _ply,x
        iny
        lda (sp),y
        sta _ply+1,x
        jmp incsp5

; ---------------------------------------------------------------------------
; void draw_order(void) (version 2.66) : pour match_draw, position à l'écran
; de chaque joueur (dr_xs, dr_ys = (v << 2) >> 8, octet) et ordre de dessin
; (dr_ord : du plus bas à l'écran au plus haut, tri par insertion stable).
; En C, ces deux boucles coûtaient environ 1 500 cycles par trame en double.
.export _draw_order, _dr_ord, _dr_xs, _dr_ys
.import _npl

.segment "WBSS"
_dr_ord: .res 4
_dr_xs:  .res 4
_dr_ys:  .res 4

.segment "CODE"
_draw_order:
        ldx #0
@pos:   txa
        sta _dr_ord,x
        asl a
        tay
        lda _ply,y              ; y : bits 6 à 13
        lsr a
        lsr a
        lsr a
        lsr a
        lsr a
        lsr a
        sta tmp1
        lda _ply+1,y
        asl a
        asl a
        ora tmp1
        sta _dr_ys,x
        lda _plx,y              ; x : idem
        lsr a
        lsr a
        lsr a
        lsr a
        lsr a
        lsr a
        sta tmp1
        lda _plx+1,y
        asl a
        asl a
        ora tmp1
        sta _dr_xs,x
        inx
        cpx _npl
        bne @pos
        ; tri : i = 1 .. npl - 1
        ldy #1
@outer: cpy _npl
        bcs @done
        lda _dr_ord,y
        sta tmp2                ; k
        tax
        lda _dr_ys,x
        sta tmp3                ; ys[k]
        tya
        tax                     ; j = i
@inner: txa
        beq @put
        lda _dr_ord-1,x         ; ord[j - 1]
        sta tmp1
        stx ptr1                ; (j)
        tax
        lda _dr_ys,x
        ldx ptr1
        cmp tmp3                ; ys[ord[j-1]] < ys[k] ?
        bcs @put
        lda tmp1
        sta _dr_ord,x           ; ord[j] = ord[j - 1]
        dex
        jmp @inner
@put:   lda tmp2
        sta _dr_ord,x
        iny
        bne @outer
@done:  rts

; ---------------------------------------------------------------------------
; u8 vbuf_raw(u16 adr, const u8 *data, u8 len) (version 2.66 ; en C avant,
; environ 60 cycles par octet, ici 14) : ajoute [adr, len, données] au tampon
; VRAM de la trame ; 0 si le budget de la trame (VBUF_MAX) serait dépassé.
; Pile C : data (sp+0), adr (sp+2) ; len dans A.
.export _vbuf_raw
.import _vram_buf, incsp4
.importzp _vram_len
VBUF_MAX = 96

_vbuf_raw:
        sta tmp1                ; len
        clc
        adc _vram_len
        bcs @full
        adc #4
        bcs @full
        cmp #VBUF_MAX + 1
        bcs @full
        ldy #0
        lda (sp),y
        sta ptr1
        iny
        lda (sp),y
        sta ptr1+1              ; data
        ldx _vram_len
        iny
        iny
        lda (sp),y              ; adr (octet haut)
        sta _vram_buf,x
        dey
        lda (sp),y
        sta _vram_buf+1,x
        lda tmp1
        sta _vram_buf+2,x
        inx
        inx
        inx
        ldy #0
        lda tmp1
        beq @end
@cp:    lda (ptr1),y
        sta _vram_buf,x
        inx
        iny
        cpy tmp1
        bne @cp
@end:   lda #$FF
        sta _vram_buf,x
        stx _vram_len
        lda #1
        ldx #0
        jmp incsp4
@full:  lda #0
        tax
        jmp incsp4
