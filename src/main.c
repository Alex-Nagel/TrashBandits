#include <stdlib.h>
#include <string.h>

#include "pixler.h"
#include "common.h"

#define BG_COLOR 0x31
static const u8 PALETTE[] = {
	BG_COLOR, 0x00, 0x10, 0x20,
	BG_COLOR, 0x06, 0x16, 0x26,
	BG_COLOR, 0x09, 0x19, 0x29,
	BG_COLOR, 0x01, 0x11, 0x21,
	
	BG_COLOR, 0x00, 0x10, 0x20,
	BG_COLOR, 0x06, 0x16, 0x26,
	BG_COLOR, 0x09, 0x19, 0x29,
	BG_COLOR, 0x01, 0x11, 0x21,
};

#define DIR_LEFT 0
#define DIR_RIGHT 1
#define DIR_DOWN 2
#define DIR_UP 3

#define HALF_PLAYER_SIZE 8
#define SCREEN_RES_X 256
#define SCREEN_RES_Y 240

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

static const u8 TRASH0_META[] = {
	0, 0, 0xC4, 0,
	8, 0, 0xC5, 0,
	0, 8, 0xC6, 0,
	8, 8, 0xC7, 0,
	128,
};

static const u8 TRASH1_META[] = {
	0, 0, 0xC8, 0,
	8, 0, 0xC9, 0,
	0, 8, 0xCA, 0,
	8, 8, 0xCB, 0,
	128,
};

static u8* TRASH_METAS[] = {
	TRASH0_META,
	TRASH1_META,
};

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
		drop_trash(rand()%10, i, i&1);
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
		meta_spr(ARENA.trash.x[idx], ARENA.trash.y[idx], 0, TRASH_METAS[ARENA.trash.type[idx]]);
	}
}

static const u8 PLAYER_LEFT_META[] = {
	 0, -8, 0xD8, 0 | PX_SPR_FLIPX,
	-8, -8, 0xD9, 0 | PX_SPR_FLIPX,
	 0,  0, 0xEA, 0 | PX_SPR_FLIPX,
	-8,  0, 0xEB, 0 | PX_SPR_FLIPX,
	128,
};

static const u8 PLAYER_RIGHT_META[] = {
	-8, -8, 0xD8, 0,
	 0, -8, 0xD9, 0,
	-8,  0, 0xEA, 0,
	 0,  0, 0xEB, 0,
	128,
};

static const u8 PLAYER_DOWN_META[] = {
	-8, -8, 0xD0, 0,
	 0, -8, 0xD1, 0,
	-8,  0, 0xD2, 0,
	 0,  0, 0xD3, 0,
	128,
};

static const u8 PLAYER_UP_META[] = {
	-8, -8, 0xC4, 0,
	 0, -8, 0xC5, 0,
	-8,  0, 0xC6, 0,
	 0,  0, 0xC7, 0,
	128,
};


static const u8* PLAYER_ANIM_1[] = {
	PLAYER_LEFT_META,
	PLAYER_RIGHT_META,
	PLAYER_DOWN_META,
	PLAYER_UP_META,
};

struct Player {
	u8 x;
	u8 y;
	u8 player_direction;
};

struct Player player1;
struct Player player2;

bool player_direction_left = false;

static void update_player_movement(){
	if(JOY_LEFT (pad1.value)) { player1.x -= 1; player1.player_direction = DIR_LEFT; }
	if(JOY_RIGHT(pad1.value)) { player1.x += 1; player1.player_direction = DIR_RIGHT; }
	if(JOY_DOWN (pad1.value)) { player1.y += 1; player1.player_direction = DIR_DOWN; }
	if(JOY_UP   (pad1.value)) { player1.y -= 1; player1.player_direction = DIR_UP; }

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

static void splash_screen(void){
	px_ppu_sync_disable();{
		// Load the splash tilemap into nametable 0.
		px_lz4_to_vram(NT_ADDR(0, 0, 0), MAP_SPLASH);
	} px_ppu_sync_enable();
	
	// music_play(0);
	
	fade_from_black(PALETTE, 4);
	
	init_arena();

	// Set initial player positions. Might want to change later
	player1.x = 64;
	player1.y = 120;
	player1.player_direction = DIR_RIGHT;
	
	player2.x = SCREEN_RES_X - 64;
	player2.y = 120;
	player2.player_direction = DIR_LEFT;
	
	while(true){
		update_arena();
		
		read_gamepads();
		update_player_movement();
		
		// Draw player sprites
		meta_spr(player1.x, player1.y, 2, PLAYER_ANIM_1[player1.player_direction]);
		meta_spr(player2.x, player2.y, 2, PLAYER_ANIM_1[player2.player_direction]);

		// meta_spr(player1.x, player1.y, 2, (animation_FLIP : animation)[(px_ticks / 8) % 2]);
		// meta_spr(player2.x, player2.y, 2, (player_direction_left ? animation_FLIP : animation)[(px_ticks / 8) % 2]);
		
		draw_arena();
		px_spr_end();
		px_wait_nmi();
	}
	
	splash_screen();
}

void main(void){
	// Set up CC65 joystick driver.
	joy_install(nes_stdjoy_joy);
	
	// Set which tiles to use for the background and sprites.
	px_bg_table(0);
	px_spr_table(0);
	
	// Not using bank switching, but a good idea to set a reliable value at boot.
	px_uxrom_select(0);
	
	// Black out the palette.
	for(idx = 0; idx < 32; idx++) px_buffer_set_color(idx, 0x1D);
	px_wait_nmi();
	
	// Decompress the tileset into character memory.
	px_lz4_to_vram(CHR_ADDR(0, 0), CHR0);
	
	music_init(&MUSIC);
	sound_init(&SOUNDS);
	music_play(0);
	
	rand_seed = 0x7A3B;
	px_debug_hex_addr = NT_ADDR(0, 3, 3);
	
	// Jump to the splash screen state.
	splash_screen();
}
