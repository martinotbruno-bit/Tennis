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
