PROJECT_NAME = GGJ2X
ROM = $(PROJECT_NAME).nes

CC = tools/cc65/bin/cc65
AS = tools/cc65/bin/ca65
LD = tools/cc65/bin/ld65

C65FLAGS += \
	-t nes -Oirs \
	--register-space 16 \
	-I ext/pixler/lib \

ASMINC = \
	-I ext/pixler/lib \

SRC = \
	src/main.c \

ASM = \
	src/data.s \
	src/misc.s \
	audio/audio.s \
	ext/famitone5/famitone5.s \

OBJS = \
	$(SRC:.c=.o) \
	$(ASM:.s=.o) \

CHR = \
	chr/0.png \

SONGS = \
	audio/after_the_rain.txt \

default: $(ROM)
rom: $(ROM)

PX_LIB_PATH = ext/pixler/lib
PX_LIB = $(PX_LIB_PATH)/px.lib
$(PX_LIB):
	make CC65_ROOT=tools/cc65 -C $(PX_LIB_PATH)

run-mac: rom
	open -a Nestopia $(ROM)

run-linux: rom
	mesen $(ROM)
#	nestopia -w -l 1 -n -s 2 -t $(ROM)

run-win: rom
	../Mesen/Mesen.exe $(ROM)

$(ROM): ld65.cfg $(OBJS) $(PX_LIB)
	$(LD) -C ld65.cfg --dbgfile $(ROM:.nes=.dbg) $(OBJS) $(PX_LIB) nes.lib -m link.log -o $@

%.s: %.c
	$(CC) -g $(C65FLAGS) $< --add-source $(INCLUDE) -o $@

%.s %.o: %.c
	tools/cc65/bin/cl65 -c -g $(C65FLAGS) $(INCLUDE) $< -o $@

%.o: %.s
	$(AS) -g $< $(ASMINC) -o $@

%.chr: %.png
	tools/png2chr $< $@

%.lz4: %.chr
	tools/lz4x -f9 $< $@

%.bin: %.tmx
	python ext/pixler/tools/tmx2bin.py $< $@

%.lz4: %.bin
	tools/lz4x -f9 $< $@

src/data.o: $(CHR:.png=.lz4) map/splash.lz4

tiles: chr/0.chr
	tools/chr2png "1D 00 10 20" chr/0.chr chr/0-pal0.png
	tools/chr2png "1D 06 16 26" chr/0.chr chr/0-pal1.png
	tools/chr2png "1D 09 19 29" chr/0.chr chr/0-pal2.png
	tools/chr2png "1D 01 11 21" chr/0.chr chr/0-pal3.png

audio/sounds.s: audio/sounds.nsf
	tools/nsf2data5 $< -ca65 -ntsc

audio/%.s: audio/%.txt
	tools/text2vol5 -ca65 $<

audio/audio.o: $(SONGS:.txt=.s) audio/sounds.s

tools:
	echo foobar

clean:
	-rm $(OBJS) $(CHR:.png=.chr) $(CHR:.png=.lz4)
	-rm map/splash.bin map/splash.lz4
	-rm $(SONGS:.txt=.s)
.phony: default rom tiles clean
