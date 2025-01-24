.include "zeropage.inc"
.include "pixler.inc"

.export px_uxrom_bank
.export px_uxrom_select, _px_uxrom_select

.bss
	; the current set bank value
	px_uxrom_bank: .res 1

.rodata

	bank_selector: .byte 0, 1, 2, 3, 4, 5, 6, 7

.code

.proc px_uxrom_select
	; to set the uxrom bank, you just write a value to ROM (or maybe just PRG_MAIN?)
	; to avoid bus conflicts, you want to write a value that's already there
	; that's what this weird lookup table is for
	lda bank_selector, y
	sta bank_selector, y
	; store the current value
	sty px_uxrom_bank
	rts
.endproc

.proc _px_uxrom_select ; u8 bank
	and #$07
	tay
	
	jmp px_uxrom_select
.endproc

.segment "PRG0"
.segment "PRG1"
.segment "PRG2"
.segment "PRG3"
.segment "PRG4"
.segment "PRG5"
.segment "PRG6"
