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
inclz4 _SPRITES, "chr/sprites.lz4"
inclz4 _MAP_SPLASH, "map/splash.lz4"
