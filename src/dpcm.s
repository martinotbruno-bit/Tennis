; Fichier généré par tools/gen_voice.py — ne pas modifier.
; Échantillons DPCM (voix de l'arbitre, « pok »), banque fixe.
.export _dpcm_addr, _dpcm_len, _dpcm_rate

.segment "DPCM"
.align 64
dpcm_out:
    .byte $FD,$FF,$FF,$FF,$F0,$E1,$E3,$8F,$C7,$70,$E0,$C3,$83,$87,$0F,$1F
    .byte $FE,$78,$86,$03,$1E,$3E,$78,$F8,$E0,$E1,$8F,$67,$70,$C0,$C3,$87
    .byte $0F,$1F,$3C,$FC,$E1,$38,$0C,$F0,$E0,$C3,$87,$0F,$3E,$DC,$F1,$38
    .byte $0C,$F1,$E0,$83,$0F,$1F,$7C,$70,$CB,$E3,$B0,$C6,$07,$1F,$7C,$F0
    .byte $E1,$87,$07,$3C,$0E,$6B,$7C,$F0,$C1,$07,$3F,$F8,$E0,$90,$C7,$E3
    .byte $1A,$1F,$78,$E0,$83,$1F,$7C,$F0,$20,$87,$C3,$1F,$3E,$F0,$C1,$0F
    .byte $7E,$F0,$83,$07,$79,$3C,$F6,$C1,$03,$1F,$F8,$C1,$0F,$7C,$F0,$11
    .byte $1E,$0F,$6F,$F0,$81,$0F,$FC,$E0,$07,$3F,$F0,$0A,$0F,$87,$8F,$F0
    .byte $81,$1F,$FC,$C0,$0F,$7E,$F0,$1F,$3E,$06,$1E,$C0,$0F,$7C,$E0,$07
    .byte $3F,$F0,$03,$FF,$C0,$5F,$80,$0F,$F0,$07,$7E,$F0,$03,$3F,$FE,$00
    .byte $FF,$80,$7F,$01,$7E,$00,$3F,$F0,$C1,$1F,$F8,$C1,$1F,$E0,$1F,$F0
    .byte $0F,$C0,$1F,$E0,$0F,$FC,$E0,$07,$FC,$E0,$1F,$F0,$1F,$F0,$07,$80
    .byte $3F,$C0,$1F,$F8,$83,$1F,$F8,$03,$FE,$00,$FF,$00,$FF,$00,$F8,$0F
    .byte $FC,$03,$FC,$C0,$1F,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA
    .byte $AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA
    .byte $AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$3A,$74
    .byte $F3,$9A,$F0,$51,$C5,$D2,$31,$9D,$C5,$D6,$99,$C2,$31,$35,$8F,$C9
    .byte $C3,$1A,$D3,$96,$2C,$D5,$54,$B7,$45,$36,$6C,$C2,$8E,$7D,$15,$B4
    .byte $A5,$69,$CA,$52,$6B,$69,$1E,$62,$5D,$55,$51,$4B,$3B,$07,$2B,$AA
    .byte $AA,$AA,$AA,$AA,$AA,$AA,$AA,$0A,$00,$00,$00,$50,$55,$55,$55,$55
    .byte $55
.align 64
dpcm_fault:
    .byte $FD,$FF,$FF,$FF,$AB,$EA,$56,$65,$B2,$65,$CC,$A5,$5C,$54,$CB,$6A
    .byte $AC,$0D,$D3,$9A,$55,$26,$CD,$56,$62,$AF,$C8,$B4,$59,$AA,$B2,$5C
    .byte $2A,$75,$2E,$95,$F0,$CA,$45,$4B,$6B,$6E,$22,$EA,$29,$6B,$07,$A5
    .byte $6D,$29,$F4,$4B,$D1,$2A,$C7,$C4,$DD,$62,$55,$2A,$75,$2B,$19,$AB
    .byte $59,$0E,$FC,$59,$B0,$9C,$CA,$85,$AE,$74,$A9,$15,$B3,$A6,$E3,$A4
    .byte $0A,$3E,$3C,$78,$F8,$09,$3E,$78,$78,$F8,$F0,$E1,$C1,$87,$1F,$E0
    .byte $C3,$87,$87,$0F,$1E,$1E,$7C,$F8,$09,$3C,$F8,$F0,$F0,$C1,$C3,$07
    .byte $1F,$7E,$02,$0F,$3E,$3C,$F8,$E0,$C3,$07,$1F,$7E,$02,$1F,$7C,$78
    .byte $F0,$C1,$07,$1F,$7C,$F8,$0B,$78,$F0,$C1,$83,$0F,$7E,$F0,$C1,$07
    .byte $BF,$81,$0F,$3E,$78,$E0,$83,$0F,$7C,$F0,$C1,$4F,$C0,$07,$1F,$7E
    .byte $F0,$E1,$07,$7E,$F0,$C3,$9F,$80,$0F,$7E,$C0,$07,$97,$07,$7E,$82
    .byte $3F,$F8,$0B,$F0,$81,$6F,$E0,$E1,$E5,$85,$0F,$F0,$0F,$FC,$09,$F8
    .byte $81,$FE,$20,$F4,$C3,$17,$03,$FE,$0F,$F8,$17,$E0,$07,$FC,$01,$DA
    .byte $0B,$FE,$04,$C2,$7F,$E0,$BF,$00,$3F,$E0,$0F,$09,$7F,$E0,$17,$35
    .byte $E0,$3F,$F0,$1F,$80,$3F,$C0,$1F,$6C,$6A,$A5,$96,$FA,$01,$FE,$01
    .byte $FE,$01,$F8,$07,$F8,$07,$BE,$48,$37,$C9,$2F,$80,$FF,$01,$FF,$01
    .byte $F0,$0F,$F0,$0F,$78,$A5,$5A,$95,$6E,$02,$FF,$07,$F0,$1F,$00,$FF
    .byte $00,$FE,$C1,$2A,$5D,$A9,$36,$55,$45,$FE,$07,$E0,$3F,$00,$FC,$03
    .byte $FC,$A5,$96,$B4,$93,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA
    .byte $AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA
    .byte $AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$1E,$7C,$F8,$0B,$7C
    .byte $16,$5C,$91,$67,$72,$C5,$0F,$53,$0E,$F1,$C9,$1C,$1D,$5B,$5A,$96
    .byte $96,$94,$96,$43,$F3,$85,$B9,$69,$D1,$A6,$6C,$AA,$8A,$3E,$E2,$4D
    .byte $A8,$3D,$29,$57,$52,$17,$73,$59,$A1,$AA,$AA,$AA,$AA,$AA,$AA,$AA
    .byte $AA,$00,$00,$00,$00,$55,$55,$55,$55,$55,$55,$55,$55,$55,$55,$55
    .byte $55
.align 64
dpcm_let:
    .byte $FD,$FF,$FF,$FF,$5B,$47,$B8,$FF,$03,$00,$FF,$80,$6E,$93,$DC,$A3
    .byte $A0,$FF,$0B,$00,$FF,$00,$DE,$13,$EF,$21,$D0,$FF,$03,$80,$FF,$00
    .byte $F7,$28,$EF,$01,$E4,$FF,$01,$E0,$3F,$80,$7F,$84,$FB,$07,$E0,$7F
    .byte $02,$F0,$0F,$F0,$0F,$61,$FC,$87,$F0,$BF,$00,$BC,$80,$7F,$E2,$08
    .byte $FE,$17,$FC,$27,$80,$27,$F8,$07,$BC,$02,$FD,$06,$FF,$05,$78,$80
    .byte $7F,$E0,$9E,$01,$FE,$C3,$9F,$00,$0F,$F8,$83,$6E,$CF,$80,$7F,$E8
    .byte $13,$E4,$80,$3F,$74,$A6,$3F,$C0,$0F,$7D,$83,$0E,$F8,$03,$5B,$E6
    .byte $83,$FC,$D1,$27,$E8,$80,$1D,$F8,$99,$7D,$22,$7F,$F4,$0B,$34,$E0
    .byte $06,$7C,$5A,$76,$45,$FC,$C1,$37,$E8,$80,$1D,$F2,$E1,$07,$5F,$23
    .byte $7F,$F4,$0B,$30,$E0,$87,$C1,$87,$0F,$3E,$7C,$E0,$8F,$3E,$01,$06
    .byte $FC,$F0,$C3,$0F,$3E,$F8,$E0,$07,$9E,$F8,$13,$E0,$83,$1F,$F8,$E0
    .byte $07,$3F,$F8,$81,$1F,$F4,$C1,$3F,$01,$7E,$E0,$03,$3F,$F0,$03,$7F
    .byte $E0,$07,$FE,$80,$1F,$F0,$C7,$80,$3F,$F0,$03,$7F,$E0,$07,$AE,$AA
    .byte $AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA
    .byte $AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA
    .byte $AA,$AA,$AA,$AA,$AA,$AA,$07,$1F,$FE,$02,$9F,$11,$D7,$E0,$99,$9C
    .byte $35,$CB,$94,$C3,$7C,$32,$46,$C7,$96,$94,$A5,$65,$A5,$AD,$D0,$6E
    .byte $61,$6E,$52,$BC,$29,$9B,$2A,$EA,$8E,$78,$13,$6A,$4F,$CA,$95,$D4
    .byte $C5,$5C,$D6,$70,$A8,$AA,$AA,$AA,$AA,$AA,$AA,$AA,$2A,$00,$00,$00
    .byte $40
.align 64
dpcm_pok:
    .byte $FD,$FF,$7F,$00,$F0,$FF,$00,$F8,$BF,$00,$F8,$5F,$00,$FE,$0F,$00
    .byte $FF,$17,$80,$FD,$07,$48,$7F,$0B,$48,$7F,$13,$C8,$7E,$25,$A4,$BD
    .byte $45,$A4,$BB,$49,$64,$BB,$8A,$54,$B7,$8A,$D4,$B6,$92,$D4,$B6,$92
    .byte $B4,$B6,$94,$B2,$56,$95,$AA,$56,$A5,$AA,$55,$A5,$AA,$55,$A5,$AA
    .byte $55,$A5,$AA,$55,$A5,$AA,$55,$A5,$AA,$55,$A5,$6A,$55,$A9,$6A,$55
    .byte $A9,$2A,$00,$50,$55,$55,$55,$55,$55,$55,$55,$55,$55,$55,$55,$55
    .byte $55

.segment "RODATA"
; $4012 = (adresse - $C000) / 64, $4013 = (longueur - 1) / 16, $4010 = fréquence
_dpcm_addr: .byte <((dpcm_out - $C000) >> 6), <((dpcm_fault - $C000) >> 6), <((dpcm_let - $C000) >> 6), <((dpcm_pok - $C000) >> 6)
_dpcm_len:  .byte 20, 25, 19, 6
_dpcm_rate: .byte 6, 6, 6, 13
