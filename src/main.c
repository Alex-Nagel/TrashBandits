#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

static void drop_trash(u8 x, u8 y, const u8* item, u8 pal){
	// address of the top left corner of the block in the "nametable" (tilemap)
	u16 addr = NT_ADDR(0, 2*x, 2*y);
	// buffer some tile writes for the next vblank
	px_buffer_blit(addr, item + 0, 2);
	px_buffer_blit(addr + 32, item + 2, 2);
	
	// TODO set attr bit
}

static void update_arena(void){
	// TODO need initial trash
	// TODO need timers for player goals
	// TODO need timers for trash drops?
	//   on a timer after player pickups?
	
	static u8 item1[] = {'A', 'A', 'A', 'A'};
	static u8 item2[] = {'B', 'B', 'B', 'B'};
	static u8 item3[] = {'C', 'C', 'C', 'C'};
	static u8 item4[] = {'D', 'D', 'D', 'D'};
	static u8 item5[] = {'E', 'E', 'E', 'E'};
	drop_trash(5, 5, item1, 0);
	drop_trash(6, 5, item2, 0);
	drop_trash(7, 5, item3, 0);
	drop_trash(8, 5, item4, 0);
}

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
		px_lz4_to_vram(NT_ADDR(0, 0, 0), MAP_SPLASH);
	} px_ppu_sync_enable();
	
	// music_play(0);
	
	fade_from_black(PALETTE, 4);

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
		meta_spr(player1.x, player1.y, 2, PLAYER_ANIM_1[player1.player_direction]);
		meta_spr(player2.x, player2.y, 2, PLAYER_ANIM_1[player2.player_direction]);

		// meta_spr(player1.x, player1.y, 2, (animation_FLIP : animation)[(px_ticks / 8) % 2]);
		// meta_spr(player2.x, player2.y, 2, (player_direction_left ? animation_FLIP : animation)[(px_ticks / 8) % 2]);
		update_game_timer();

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
	
	// Jump to the splash screen state.
	splash_screen();
}
