#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "pixler.h"
#include "common.h"

#define DIR_LEFT 0
#define DIR_RIGHT 1
#define DIR_DOWN 2
#define DIR_UP 3

#define HALF_PLAYER_SIZE 8
#define SCREEN_RES_X 256
#define SCREEN_RES_Y 240

#define FPS 60

Gamepad pad1, pad2;

void read_gamepads(void){
	pad1.prev = pad1.value;
	pad1.value = joy_read(0);
	pad1.press = pad1.value & (pad1.value ^ pad1.prev);
	pad1.release = pad1.prev & (pad1.value ^ pad1.prev);
	
	pad2.prev = pad2.value;
	pad2.value = joy_read(1);
	pad2.press = pad2.value & (pad2.value ^ pad2.prev);
	pad2.release = pad2.prev & (pad2.value ^ pad2.prev);
}

void wait_noinput(void){
	while(joy_read(0) || joy_read(1)) px_wait_nmi();
}

static void darken(register const u8* palette, u8 shift){
	for(idx = 0; idx < 32; idx++){
		ix = palette[idx];
		ix -= shift << 4;
		if(ix > 0x40 || ix == 0x0D) ix = 0x1D;
		px_buffer_set_color(idx, ix);
	}
}

void fade_from_black(const u8* palette, u8 delay){
	darken(palette, 4);
	px_wait_frames(delay);
	darken(palette, 3);
	px_wait_frames(delay);
	darken(palette, 2);
	px_wait_frames(delay);
	darken(palette, 1);
	px_wait_frames(delay);
	darken(palette, 0);
}

void meta_spr(u8 x, u8 y, u8 pal, const u8* data);

static const u8 TRASH_APPLE_META[] = {
	0, 0, 0x00, 0,
	8, 0, 0x01, 0,
	0, 8, 0x10, 0,
	8, 8, 0x11, 0,
	128,
};

static const u8 TRASH_BANANA_META[] = {
	0, 0, 0x02, 0,
	8, 0, 0x03, 0,
	0, 8, 0x12, 0,
	8, 8, 0x13, 0,
	128,
};

static const u8 TRASH_TIRE_META[] = {
	0, 0, 0x04, 0,
	8, 0, 0x05, 0,
	0, 8, 0x14, 0,
	8, 8, 0x15, 0,
	128,
};

static const u8 TRASH_FISH_META[] = {
	0, 0, 0x06, 0,
	8, 0, 0x07, 0,
	0, 8, 0x16, 0,
	8, 8, 0x17, 0,
	128,
};

static u8* TRASH_METAS[] = {
	TRASH_APPLE_META,
	TRASH_BANANA_META,
	TRASH_TIRE_META,
	TRASH_FISH_META,
};

static u8 TRASH_PAL[] = {2, 1, 3, 2};

#define MAX_TRASH 10
struct {
	struct {
		int count;
		u8 type[MAX_TRASH];
		u8 x[MAX_TRASH];
		u8 y[MAX_TRASH];
	} trash;
	
	// horizontal spans with trash in them
	bool occupied[MAX_TRASH];
} ARENA;

static void drop_trash(u8 x, u8 y, u8 type){
	idx = ARENA.trash.count;
	ARENA.trash.x[idx] = 16*(x + (16 - 10)/2);
	ARENA.trash.y[idx] = 16*(y + (15 - MAX_TRASH)/2);
	ARENA.trash.type[idx] = type;
	ARENA.trash.count++;
	ARENA.occupied[y] = true;
}

static void init_arena(void){
	u8 i;
	for(i = 0; i < MAX_TRASH; i++){
		drop_trash(rand()%10, i, i%4);
	}
}

static void update_arena(void){
	// TODO need initial trash
	// TODO need timers for player goals
	// TODO need timers for trash drops?
	//   on a timer after player pickups?
}

static void draw_arena(){
	for(idx = 0; idx < ARENA.trash.count; idx++){
		meta_spr(ARENA.trash.x[idx], ARENA.trash.y[idx], TRASH_PAL[ARENA.trash.type[idx]], TRASH_METAS[ARENA.trash.type[idx]]);
	}
}

static const u8 PLAYER_LEFT0_META[] = {
	-8, -8, 0x26, 0,
	 0, -8, 0x27, 0,
	-8,  0, 0x36, 0,
	 0,  0, 0x37, 0,
	128,
};

static const u8 PLAYER_LEFT1_META[] = {
	-8, -8, 0x28, 0,
	 0, -8, 0x29, 0,
	-8,  0, 0x38, 0,
	 0,  0, 0x39, 0,
	128,
};

static const u8 PLAYER_LEFT2_META[] = {
	-8, -8, 0x2A, 0,
	 0, -8, 0x2B, 0,
	-8,  0, 0x3A, 0,
	 0,  0, 0x3B, 0,
	128,
};

static const u8 PLAYER_RIGHT0_META[] = {
	 0, -8, 0x26, PX_SPR_FLIPX,
	-8, -8, 0x27, PX_SPR_FLIPX,
	 0,  0, 0x36, PX_SPR_FLIPX,
	-8,  0, 0x37, PX_SPR_FLIPX,
	128,
};

static const u8 PLAYER_RIGHT1_META[] = {
	 0, -8, 0x28, PX_SPR_FLIPX,
	-8, -8, 0x29, PX_SPR_FLIPX,
	 0,  0, 0x38, PX_SPR_FLIPX,
	-8,  0, 0x39, PX_SPR_FLIPX,
	128,
};

static const u8 PLAYER_RIGHT2_META[] = {
	 0, -8, 0x2A, PX_SPR_FLIPX,
	-8, -8, 0x2B, PX_SPR_FLIPX,
	 0,  0, 0x3A, PX_SPR_FLIPX,
	-8,  0, 0x3B, PX_SPR_FLIPX,
	128,
};

static const u8 PLAYER_UP0_META[] = {
	-8, -8, 0x46, 0,
	 0, -8, 0x47, 0,
	-8,  0, 0x56, 0,
	 0,  0, 0x57, 0,
	128,
};

static const u8 PLAYER_UP1_META[] = {
	-8, -8, 0x48, 0,
	 0, -8, 0x49, 0,
	-8,  0, 0x58, 0,
	 0,  0, 0x59, 0,
	128,
};

static const u8 PLAYER_UP2_META[] = {
	-8, -8, 0x4A, 0,
	 0, -8, 0x4B, 0,
	-8,  0, 0x5A, 0,
	 0,  0, 0x5B, 0,
	128,
};

static const u8 PLAYER_DOWN0_META[] = {
	-8, -8, 0x66, 0,
	 0, -8, 0x67, 0,
	-8,  0, 0x76, 0,
	 0,  0, 0x77, 0,
	128,
};

static const u8 PLAYER_DOWN1_META[] = {
	-8, -8, 0x68, 0,
	 0, -8, 0x69, 0,
	-8,  0, 0x78, 0,
	 0,  0, 0x79, 0,
	128,
};

static const u8 PLAYER_DOWN2_META[] = {
	-8, -8, 0x6A, 0,
	 0, -8, 0x6B, 0,
	-8,  0, 0x7A, 0,
	 0,  0, 0x7B, 0,
	128,
};

static const u8* PLAYER_LEFT_ANIM[] = {PLAYER_LEFT0_META, PLAYER_LEFT1_META, PLAYER_LEFT2_META};
static const u8* PLAYER_RIGHT_ANIM[] = {PLAYER_RIGHT0_META, PLAYER_RIGHT1_META, PLAYER_RIGHT2_META};
static const u8* PLAYER_DOWN_ANIM[] = {PLAYER_DOWN0_META, PLAYER_DOWN1_META, PLAYER_DOWN2_META};
static const u8* PLAYER_UP_ANIM[] = {PLAYER_UP0_META, PLAYER_UP1_META, PLAYER_UP2_META};

static const u8** PLAYER_ANIMS[] = {
	PLAYER_LEFT_ANIM,
	PLAYER_RIGHT_ANIM,
	PLAYER_DOWN_ANIM,
	PLAYER_UP_ANIM,
};

struct Player {
	u8 x;
	u8 y;
	u8 player_direction;
	u8 anim_ticks;
};

struct Player player1;
struct Player player2;

bool player_direction_left = false;

#define JOY_DPAD_MASK (JOY_UP_MASK | JOY_DOWN_MASK | JOY_LEFT_MASK | JOY_RIGHT_MASK)
#define PLAYER_TICKS_PER_FRAME 4

static void update_player_movement(){
	if(JOY_LEFT (pad1.value)) { player1.x -= 1; player1.player_direction = DIR_LEFT; }
	if(JOY_RIGHT(pad1.value)) { player1.x += 1; player1.player_direction = DIR_RIGHT; }
	if(JOY_DOWN (pad1.value)) { player1.y += 1; player1.player_direction = DIR_DOWN; }
	if(JOY_UP   (pad1.value)) { player1.y -= 1; player1.player_direction = DIR_UP; }
	if(pad1.value & JOY_DPAD_MASK){
		player1.anim_ticks++;
		if(player1.anim_ticks/PLAYER_TICKS_PER_FRAME == 3) player1.anim_ticks = 0;
	} else {
		player1.anim_ticks = 0;
	}

	// Clamp player movement never goes oob
	if (player1.x < HALF_PLAYER_SIZE) { player1.x = HALF_PLAYER_SIZE; }
	if (player1.y < HALF_PLAYER_SIZE) { player1.y = HALF_PLAYER_SIZE; }
	
	if (player1.x > SCREEN_RES_X - HALF_PLAYER_SIZE) { player1.x = SCREEN_RES_X - HALF_PLAYER_SIZE; }
	if (player1.y > SCREEN_RES_Y - HALF_PLAYER_SIZE) { player1.y = SCREEN_RES_Y - HALF_PLAYER_SIZE; }
	
	if(JOY_LEFT (pad2.value)) { player2.x -= 1; player2.player_direction = DIR_LEFT; }
	if(JOY_RIGHT(pad2.value)) { player2.x += 1; player2.player_direction = DIR_RIGHT; }
	if(JOY_DOWN (pad2.value)) { player2.y += 1; player2.player_direction = DIR_DOWN; }
	if(JOY_UP   (pad2.value)) { player2.y -= 1; player2.player_direction = DIR_UP; }
	
	// Clamp player movement never goes oob
	if (player2.x < HALF_PLAYER_SIZE) { player2.x = HALF_PLAYER_SIZE; }
	if (player2.y < HALF_PLAYER_SIZE) { player2.y = HALF_PLAYER_SIZE; }

	if (player2.x > SCREEN_RES_X - HALF_PLAYER_SIZE) { player2.x = SCREEN_RES_X - HALF_PLAYER_SIZE; }
	if (player2.y > SCREEN_RES_Y - HALF_PLAYER_SIZE) { player2.y = SCREEN_RES_Y - HALF_PLAYER_SIZE; }
}

u8* minutes_timer;
u8* seconds_timer;
u8* frames_timer;

static void update_game_timer(){
	char buffer[10];
	// u8 visible_minutes;
	// u8 visible_seconds;

	frames_timer--;
	if (frames_timer == 0){
		
		if (seconds_timer == 0)
		{
			if (minutes_timer == 0)
			{
				// TODO End the game and go to game over screen
				return;
			}
			minutes_timer--;
			seconds_timer = 60;
		}

		seconds_timer--;
		frames_timer = FPS;
	}

	// Draw the timer
	if (seconds_timer < 10) sprintf(buffer, "%d:0%d", minutes_timer, seconds_timer);
	else 					sprintf(buffer, "%d:%d", minutes_timer, seconds_timer);
	
	px_buffer_blit(NT_ADDR(0, 14, 2), buffer, strlen(buffer));
}

static void splash_screen(void){
	px_ppu_sync_disable();{
		// Load the splash tilemap into nametable 0.
		px_lz4_to_vram(NT_ADDR(0, 0, 0), MAP_DUMP);
	} px_ppu_sync_enable();
	
	// music_play(0);
	
	fade_from_black(PAL_DUMP, 4);
	
	init_arena();

	// Set initial player positions. Might want to change later
	player1.x = 64;
	player1.y = 120;
	player1.player_direction = DIR_RIGHT;
	
	player2.x = SCREEN_RES_X - 64;
	player2.y = 120;
	player2.player_direction = DIR_LEFT;

	// Length of a round, change if needed (Assumes that minutes are < 10 and seconds < 60)
	minutes_timer = 2;
	seconds_timer = 30;
	frames_timer = 60; // Keep this as 60
	
	while(true){
		update_arena();
		
		read_gamepads();
		update_player_movement();
		
		// Draw player sprites
		meta_spr(player1.x, player1.y, 0, PLAYER_ANIMS[player1.player_direction][player1.anim_ticks/PLAYER_TICKS_PER_FRAME]);
		meta_spr(player2.x, player2.y, 0, PLAYER_ANIMS[player2.player_direction][0]);
		
		update_game_timer();
		draw_arena();
		
		px_spr_end();
		px_wait_nmi();
	}
	
	splash_screen();
}

void main(void){
	// Set up CC65 joystick driver.
	joy_install(nes_stdjoy_joy);
	
	// Not using bank switching, but a good idea to set a reliable value at boot.
	px_uxrom_select(0);
	
	// Black out the palette.
	for(idx = 0; idx < 32; idx++) px_buffer_set_color(idx, 0x1D);
	px_wait_nmi();
	
	// Decompress the tileset into character memory.
	px_lz4_to_vram(CHR_ADDR(0, 0), CHR_DUMP);
	px_lz4_to_vram(CHR_ADDR(1, 0), CHR_SPRITES);
	
	// Set which tiles to use for the background and sprites.
	px_bg_table(0);
	px_spr_table(1);
	
	music_init(&MUSIC);
	sound_init(&SOUNDS);
	music_play(0);
	
	rand_seed = 0x7A3B;
	px_debug_hex_addr = NT_ADDR(0, 3, 3);
	
	// Jump to the splash screen state.
	splash_screen();
}
