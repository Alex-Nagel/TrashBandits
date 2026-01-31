.macro inclz4 symbol, file
	.export symbol
	symbol:
		.incbin file, 8
		.word 0 ; terminator
.endmacro

.segment "PRG0"

.export _GAME_PALETTE
_GAME_PALETTE:
	; .byte

inclz4 _CHR0, "chr/0.lz4"
inclz4 _CHR_DUMP, "chr/dump.lz4"
inclz4 _CHR_SPRITES, "chr/sprites.lz4"

inclz4 _MAP_SPLASH, "map/splash.lz4"
inclz4 _MAP_DUMP, "map/dump.lz4"

.export _PAL_DUMP
_PAL_DUMP:
	.incbin "chr/dump.pal"
	
	.byte $17, $3D, $20, $1D
	.byte $17, $08, $28, $1D
	.byte $17, $09, $39, $1D
	.byte $17, $2C, $1D, $16
