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

#define SECONDS_BETWEEN_GOAL_CHANGE 15

#define SPARKLE_TICKS_PER_FRAME 4
#define SPARKLE_TICK_FRAMES 5

#define DUMPSTER_TOP_BOUND 64
#define DUMPSTER_BOTTOM_BOUND 160
#define DUMPSTER_1_RIGHT_BOUND 64
#define DUMPSTER_2_LEFT_BOUND 180

#define GOAL_TRASH_SCORE 5
#define NORMAL_TRASH_SCORE 2

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

void fade_to_black(const u8* palette, u8 delay){
	darken(palette, 0);
	px_wait_frames(delay);
	darken(palette, 1);
	px_wait_frames(delay);
	darken(palette, 2);
	px_wait_frames(delay);
	darken(palette, 3);
	px_wait_frames(delay);
	darken(palette, 4);
}

void meta_spr(u8 x, u8 y, u8 pal, const u8* data);

static const u8 TRASH_BAG0_META[] = {
	0, 0, 0x40, 0,
	8, 0, 0x41, 0,
	0, 8, 0x50, 0,
	8, 8, 0x51, 0,
	128,
};

static const u8 TRASH_BAG1_META[] = {
	0, 0, 0x42, 0,
	8, 0, 0x43, 0,
	0, 8, 0x52, 0,
	8, 8, 0x53, 0,
	128,
};

static const u8 TRASH_BAG2_META[] = {
	0, 0, 0x44, 0,
	8, 0, 0x45, 0,
	0, 8, 0x54, 0,
	8, 8, 0x55, 0,
	128,
};

static const u8* TRASH_BAG_ANIM[] = {
	TRASH_BAG0_META,
	TRASH_BAG1_META,
	TRASH_BAG2_META,
	TRASH_BAG1_META
};

static const u8 POOF1_META[] = {
	0, 0, 0x20, 0,
	8, 0, 0x21, 0,
	0, 8, 0x30, 0,
	8, 8, 0x31, 0,
	128,
};

static const u8 POOF2_META[] = {
	0, 0, 0x22, 0,
	8, 0, 0x23, 0,
	0, 8, 0x32, 0,
	8, 8, 0x33, 0,
	128,
};

static const u8 POOF3_META[] = {
	0, 0, 0x24, 0,
	8, 0, 0x25, 0,
	0, 8, 0x34, 0,
	8, 8, 0x35, 0,
	128,
};

static const u8* POOF_ANIM[] = {
	POOF1_META,
	POOF2_META,
	POOF3_META,
	POOF2_META,
	POOF1_META,
};

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

static const u8 RETICLE_META[] = {
	0, 0, 0x60, 0,
	8, 0, 0x61, 0,
	0, 8, 0x70, 0,
	8, 8, 0x71, 0,
	128,
};

static const u8 SAD_FACE_META[] = {
	0, 0, 0x0A, 0,
	8, 0, 0x0B, 0,
	0, 8, 0x1A, 0,
	8, 8, 0x1B, 0,
	128,
};

static const u8 HAPPY_FACE_META[] = {
	0, 0, 0x0C, 0,
	8, 0, 0x0D, 0,
	0, 8, 0x1C, 0,
	8, 8, 0x1D, 0,
	128,
};

static u8 TRASH_PAL[] = {2, 1, 3, 2};

#define MAX_TRASH 8
struct {
	struct {
		u8 type[MAX_TRASH];
		u16 x[MAX_TRASH];
		u16 y[MAX_TRASH];
		
		// counts down to 0 during the fall animation
		u8 fall_anim[MAX_TRASH];
		u8 throw_anim[MAX_TRASH];
		// Direction that the trash is currently being thrown, if being thrown
		u8 throw_dir[MAX_TRASH];
		// 0 if on ground, 1 if player 1 is picking it up, 2 for player 2
		u8 player[MAX_TRASH];
	} trash;
} ARENA;

struct Player {
	u16 x;
	u16 y;
	u8 player_direction;

	int score;
	u8 anim_ticks;

	bool hands_free;
	u8 selected;

	u8 goal_trash_type;

	u8 sparkle_anim_ticks;
	u8 sparkle_x;
	u8 sparkle_y;
};

struct Player player1;
struct Player player2;

static void add_score_player1(u8 score_increase);
static void add_score_player2(u8 score_increase);

static void drop_trash(u8 idx){
	ix = rand()%8;
	ARENA.trash.x[idx] = 256*16*(ix + 4);
	ARENA.trash.y[idx] = 256*16*(idx + 4);
	ARENA.trash.type[idx] = rand()%4;
	ARENA.trash.fall_anim[idx] = 160 + (rand() % 64);
	ARENA.trash.player[idx] = 0;
}

static void init_arena(void){
	u8 i;
	for(i = 0; i < MAX_TRASH; i++){
		drop_trash(i);
	}
}

#define THROW_TICKS 10

static void update_arena(void){
	bool matching = false;
	// TODO need initial trash
	// TODO need timers for player goals
	// TODO need timers for trash drops?
	//   on a timer after player pickups?
	u8 idx;
	for(idx = 0; idx < MAX_TRASH; idx++){
		if(ARENA.trash.fall_anim[idx]){
			ARENA.trash.fall_anim[idx]--;
		}
			
		if(ARENA.trash.throw_anim[idx]){
			// Move in thrown direction
			if      (ARENA.trash.throw_dir[idx] == 0)      ARENA.trash.x[idx] -= 4*256; // Left
			else if (ARENA.trash.throw_dir[idx] == 1) ARENA.trash.x[idx] += 4*256; // Right
			else if (ARENA.trash.throw_dir[idx] == 2) ARENA.trash.y[idx] += 4*256; // Down
			else if (ARENA.trash.throw_dir[idx] == 3) ARENA.trash.y[idx] -= 4*256; // Up

			ARENA.trash.y[idx] += 80*(THROW_TICKS - ARENA.trash.throw_anim[idx]);
			ARENA.trash.throw_anim[idx]--;
			
			// if ticks < num check for player collision
			
			if(ARENA.trash.throw_anim[idx] == 0){
				// snap to nearest grid
				ARENA.trash.x[idx] = (ARENA.trash.x[idx] + 8*256) & 0xF000;
				ARENA.trash.y[idx] = (ARENA.trash.y[idx] + 8*256) & 0xF000;
				
				// check landing area and award points
				if (ARENA.trash.y[idx] / 256 < DUMPSTER_BOTTOM_BOUND && ARENA.trash.y[idx] / 256 > DUMPSTER_TOP_BOUND){
					// drop_trash(idx);
					if (ARENA.trash.x[idx] / 256 < DUMPSTER_1_RIGHT_BOUND){
						matching = ARENA.trash.type[idx] == player1.goal_trash_type;

						// Create sparkle effect
						player1.sparkle_anim_ticks = SPARKLE_TICKS_PER_FRAME * SPARKLE_TICK_FRAMES;
						player1.sparkle_x = ARENA.trash.x[idx] / 256;
						player1.sparkle_y = ARENA.trash.y[idx] / 256;

						drop_trash(idx);

						// Check for matching goal
						if (matching) add_score_player1(GOAL_TRASH_SCORE);
						else		  add_score_player1(NORMAL_TRASH_SCORE);
					} else if (ARENA.trash.x[idx] / 256 > DUMPSTER_2_LEFT_BOUND){
						bool matching = ARENA.trash.type[idx] == player2.goal_trash_type;
						drop_trash(idx);

						// Check for matching goal
						if (matching) add_score_player2(GOAL_TRASH_SCORE);
						else		  add_score_player2(NORMAL_TRASH_SCORE);
					}
				}
				
				// respawn trash thrown off the bottom of the screen
				if(ARENA.trash.y[idx]/256 > 224){
					drop_trash(idx);
				}
			}
		}
		
		if(ARENA.trash.player[idx] == 1){
			ARENA.trash.x[idx] = player1.x -  8*256;
			ARENA.trash.y[idx] = player1.y - 24*256;
		}
		
		if(ARENA.trash.player[idx] == 2){
			ARENA.trash.x[idx] = player2.x -  8*256;
			ARENA.trash.y[idx] = player2.y - 24*256;
		}
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

static void draw_arena(){
	static u8 idx0 = 0;
	
	for(idx = 0; idx < MAX_TRASH; idx++){
		if(ARENA.trash.fall_anim[idx0]){
			if(ARENA.trash.y[idx0]/256 > ARENA.trash.fall_anim[idx0]){
				meta_spr(ARENA.trash.x[idx0]/256, ARENA.trash.y[idx0]/256 - ARENA.trash.fall_anim[idx0], 1, TRASH_BAG_ANIM[px_ticks/4 % 4]);
			}
		} else {
			meta_spr(ARENA.trash.x[idx0]/256, ARENA.trash.y[idx0]/256, TRASH_PAL[ARENA.trash.type[idx0]], TRASH_METAS[ARENA.trash.type[idx0]]);
		}
		
		idx0++;
		if(idx0 == MAX_TRASH) idx0 = 0;
	}
	
	idx0++;
	if(idx0 == MAX_TRASH) idx0 = 0;
}

static void draw_sparkles(){
	if (player1.sparkle_anim_ticks > 0){
		meta_spr(player1.sparkle_x, player1.sparkle_y, 1, POOF_ANIM[player1.sparkle_anim_ticks / SPARKLE_TICKS_PER_FRAME % SPARKLE_TICKS_PER_FRAME]);
		player1.sparkle_anim_ticks--;
	}
}

// Unfortunately didn't get to retain the ascii tiles
// Need to translate the digits and ':' into tiles
char text_buffer[16];
static draw_num(u8 x, u8 y){
	iy = strlen(text_buffer);
	for(idx = 0; idx < iy; idx++){
		if(text_buffer[idx] == ':'){
			text_buffer[idx] = 0x7F;
		} else {
			text_buffer[idx] = text_buffer[idx] - ('0' - 0x75);
		}
	}
	
	px_buffer_blit(NT_ADDR(0, x, y), text_buffer, iy);
}

// Adding 1 point is equivalent to adding 100 points in the ui. Maximums in-game score: 999900
static void add_score_player1(u8 score_increase){
	player1.score += score_increase;
	sprintf(text_buffer, "%04d00", player1.score);
	draw_num(2, 26);
}

// Adding 1 point is equivalent to adding 100 points in the ui. Maximums in-game score: 999900
static void add_score_player2(u8 score_increase){
	player2.score += score_increase;
	sprintf(text_buffer, "%04d00", player2.score);
	draw_num(24, 26);
	
}


#define JOY_DPAD_MASK (JOY_UP_MASK | JOY_DOWN_MASK | JOY_LEFT_MASK | JOY_RIGHT_MASK)
#define PLAYER_TICKS_PER_FRAME 8

static void update_player_movement(){
	u16 original_1x = player1.x;
	u16 original_1y = player1.y;
	u16 speed1 = player1.hands_free ? 1*256 : 3*256/4;
	u16 original_2x = player2.x;
	u16 original_2y = player2.y;
	u16 speed2 = player2.hands_free ? 1*256 : 3*256/4;

	if(JOY_LEFT (pad1.value)) { player1.x -= speed1; player1.player_direction = DIR_LEFT; }
	if(JOY_RIGHT(pad1.value)) { player1.x += speed1; player1.player_direction = DIR_RIGHT; }
	if(JOY_DOWN (pad1.value)) { player1.y += speed1; player1.player_direction = DIR_DOWN; }
	if(JOY_UP   (pad1.value)) { player1.y -= speed1; player1.player_direction = DIR_UP; }
	// if(JOY_BTN_A(pad1.value)) { add_score_player1(1); } // TODO Delete, right now just a test for score
	if(pad1.value & JOY_DPAD_MASK){
		player1.anim_ticks++;
		if(player1.anim_ticks/PLAYER_TICKS_PER_FRAME == 3) player1.anim_ticks = 0;
	} else {
		player1.anim_ticks = PLAYER_TICKS_PER_FRAME;
	}

	// Clamp player movement never goes oob
	if (player1.x/256 < HALF_PLAYER_SIZE) { player1.x = 256*HALF_PLAYER_SIZE; }
	if (player1.y/256 < HALF_PLAYER_SIZE) { player1.y = 256*HALF_PLAYER_SIZE; }
	
	if (player1.x/256 > SCREEN_RES_X - HALF_PLAYER_SIZE) { player1.x = 256*(SCREEN_RES_X - HALF_PLAYER_SIZE); }
	if (player1.y/256 > SCREEN_RES_Y - HALF_PLAYER_SIZE) { player1.y = 256*(SCREEN_RES_Y - HALF_PLAYER_SIZE); }

	
	// check for dumpster collision
	if (player1.y/256 < DUMPSTER_BOTTOM_BOUND + 8 && player1.y/256 > DUMPSTER_TOP_BOUND + 16){
		if (player1.x/256 < DUMPSTER_1_RIGHT_BOUND + 8){
			player1.x = original_1x;
			player1.y = original_1y;
		} else if (player1.x/256 > DUMPSTER_2_LEFT_BOUND + 8){
			player1.x = original_1x;
			player1.y = original_1y;
		}
	}
	
	if (JOY_BTN_A(pad1.press) && player1.selected != ~0){
		ARENA.trash.player[player1.selected] = 1;
		player1.hands_free = false;
	}
	
	if(JOY_BTN_A(pad1.release)){
		ARENA.trash.throw_anim[player1.selected] = THROW_TICKS;
		ARENA.trash.throw_dir[player1.selected] = player1.player_direction;
		ARENA.trash.player[player1.selected] = 0;
		player1.hands_free = true;
		player1.selected = ~0;
	}
	
	if(JOY_LEFT (pad2.value)) { player2.x -= speed2; player2.player_direction = DIR_LEFT; }
	if(JOY_RIGHT(pad2.value)) { player2.x += speed2; player2.player_direction = DIR_RIGHT; }
	if(JOY_DOWN (pad2.value)) { player2.y += speed2; player2.player_direction = DIR_DOWN; }
	if(JOY_UP   (pad2.value)) { player2.y -= speed2; player2.player_direction = DIR_UP; }
	// if(JOY_BTN_A(pad2.value)) { add_score_player2(1); } // TODO Delete, right now just a test for score
	if(pad2.value & JOY_DPAD_MASK){
		player2.anim_ticks++;
		if(player2.anim_ticks/PLAYER_TICKS_PER_FRAME == 3) player2.anim_ticks = 0;
	} else {
		player2.anim_ticks = PLAYER_TICKS_PER_FRAME;
	}
	
	// Clamp player movement never goes oob
	if (player2.x/256 < HALF_PLAYER_SIZE) { player2.x = 256*HALF_PLAYER_SIZE; }
	if (player2.y/256 < HALF_PLAYER_SIZE) { player2.y = 256*HALF_PLAYER_SIZE; }

	if (player2.x/256 > SCREEN_RES_X - HALF_PLAYER_SIZE) { player2.x = 256*(SCREEN_RES_X - HALF_PLAYER_SIZE); }
	if (player2.y/256 > SCREEN_RES_Y - HALF_PLAYER_SIZE) { player2.y = 256*(SCREEN_RES_Y - HALF_PLAYER_SIZE); }

	
	// check for dumpster collision
	if (player2.y/256 < DUMPSTER_BOTTOM_BOUND + 8 && player2.y/256 > DUMPSTER_TOP_BOUND + 16){
		if (player2.x/256 < DUMPSTER_1_RIGHT_BOUND + 8){
			player2.x = original_2x;
			player2.y = original_2y;
		} else if (player2.x/256 > DUMPSTER_2_LEFT_BOUND + 8){
			player2.x = original_2x;
			player2.y = original_2y;
		}
	}
	
	if (JOY_BTN_A(pad2.press) && player2.selected != ~0){
		ARENA.trash.player[player2.selected] = 2;
		player2.hands_free = false;
	}
	
	if(JOY_BTN_A(pad2.release)){
		ARENA.trash.throw_anim[player2.selected] = THROW_TICKS;
		ARENA.trash.throw_dir[player2.selected] = player2.player_direction;
		ARENA.trash.player[player2.selected] = 0;
		player2.hands_free = true;
		player2.selected = ~0;
	}
}

static void update_trash_goals(){
	// Changes each player's trash goal to a random trash type
	// Currently can be the same as before and same as other person, may want to change that
	player1.goal_trash_type = rand() % 4;
	player2.goal_trash_type = rand() % 4;
}

static void update_reticles(){
	u8 playerx;
	u8 playery;
	u8 trash1_global_x;
	u8 trash1_global_y;
	u8 trash2_global_x;
	u8 trash2_global_y;
	u8 trash_x;
	u8 trash_y;
	u8 grid1_x;
	u8 grid1_y;
	u8 grid2_x;
	u8 grid2_y;
	bool reticle_1_active = false;
	bool reticle_2_active = false;

	// Draw player1 reticle
	playerx = player1.x/256;
	playery = player1.y/256;

	if (player1.player_direction == DIR_LEFT) playerx -= 16;
	if (player1.player_direction == DIR_RIGHT) playerx += 16;
	if (player1.player_direction == DIR_DOWN) playery += 16;
	if (player1.player_direction == DIR_UP) playery -= 16;

	grid1_x = playerx / 16;
	grid1_y = playery / 16;

	trash1_global_x = playerx - (playerx % 16);
	trash1_global_y = playery - (playery % 16);

	// meta_spr(x, y, 0, RETICLE_META);
	
	// Draw player2 reticle
	playerx = player2.x/256;
	playery = player2.y/256;

	if (player2.player_direction == DIR_LEFT) playerx -= 16;
	if (player2.player_direction == DIR_RIGHT) playerx += 16;
	if (player2.player_direction == DIR_DOWN) playery += 16;
	if (player2.player_direction == DIR_UP) playery -= 16;

	grid2_x = playerx / 16;
	grid2_y = playery / 16;

	trash2_global_x = playerx - (playerx % 16);
	trash2_global_y = playery - (playery % 16);

	// meta_spr(x, y, 0, RETICLE_META);

	if(player1.hands_free) player1.selected = ~0;
	if(player2.hands_free) player2.selected = ~0;
	
	// Check for pickup
	for(idx = 0; idx < MAX_TRASH; idx++){
		if(ARENA.trash.fall_anim[idx] > 0) continue;
		// allow player to catch?
		// if(ARENA.trash.throw_anim[idx] > 0) continue;
		
		trash_x = ARENA.trash.x[idx] / (16*256);
		trash_y = ARENA.trash.y[idx] / (16*256);

		if (trash_x == grid1_x && trash_y == grid1_y && ARENA.trash.player[idx] == 0 && player1.hands_free){
			if(px_ticks % 2 == 0 && !reticle_1_active) {
				meta_spr(trash1_global_x, trash1_global_y, 0, RETICLE_META);
				reticle_1_active = true;
			}
			player1.selected = idx;
		}

		if (trash_x == grid2_x && trash_y == grid2_y && ARENA.trash.player[idx] == 0 && player2.hands_free){
			if(px_ticks % 2 == 1 && !reticle_2_active) {
				meta_spr(trash2_global_x, trash2_global_y, 0, RETICLE_META);
				reticle_2_active = true;
			}
			player2.selected = idx;
		}
	}
}

u8* change_goal_seconds_timer;

u8* minutes_timer;
u8* seconds_timer;
u8* frames_timer;

// Returns true when game should continue, false when game is over
static bool update_game_timer(){
	if (frames_timer == 0){
		
		if (seconds_timer == 0)
		{
			if (minutes_timer == 0)
			{
				return false;
			}
			minutes_timer--;
			seconds_timer = 60;
		}

		seconds_timer--;
		change_goal_seconds_timer--;
		frames_timer = FPS;
		
		sprintf(text_buffer, "%d:%02d", minutes_timer, seconds_timer);
		draw_num(14, 2);

		if (change_goal_seconds_timer == 0) {
			change_goal_seconds_timer = SECONDS_BETWEEN_GOAL_CHANGE;
			update_trash_goals();
		}
	}
	frames_timer--;
	
	return true;
}


static void game_over_screen();

static void game_run(void){
	px_ppu_sync_disable();{
		// Decompress the tileset into character memory.
		px_lz4_to_vram(CHR_ADDR(0, 0), CHR_DUMP);
		px_lz4_to_vram(CHR_ADDR(1, 0), CHR_SPRITES);
		// Set which tiles to use for the background and sprites.
		px_bg_table(0);
		px_spr_table(1);
		// Load the tilemap into nametable 0.
		px_lz4_to_vram(NT_ADDR(0, 0, 0), MAP_DUMP);
		PX.scroll_x = -2;
	} px_ppu_sync_enable();
	
	// music_play(0);
	
	fade_from_black(PAL_DUMP, 4);
	
	init_arena();

	// Set initial player positions. Might want to change later
	player1.x = 256*80;
	player1.y = 256*120;
	player1.hands_free = true;
	player1.player_direction = DIR_RIGHT;
	player1.score = 0;
	player1.selected = ~0;
	player1.sparkle_anim_ticks = 0;
	// triggers a redraw
	add_score_player1(0);
	
	player2.x = 256*(SCREEN_RES_X - 80);
	player2.y = 256*120;
	player2.hands_free = true;
	player2.player_direction = DIR_LEFT;
	player2.score = 0;
	player2.selected = ~0;
	player2.sparkle_anim_ticks = 0;
	// triggers a redraw
	add_score_player2(0);

	// Length of a round, change if needed (Assumes that minutes are < 10 and seconds < 60)
	minutes_timer = 1;
	seconds_timer = 30;
	frames_timer = 60; // Keep this as 60
	change_goal_seconds_timer = SECONDS_BETWEEN_GOAL_CHANGE;

	update_trash_goals();
	
	while(true){
		// px_profile_start();
		update_arena();
		
		read_gamepads();
		update_player_movement();
		if (!update_game_timer()) break;
		
		// Draw player sprites
		meta_spr(player1.x/256, player1.y/256, 0, PLAYER_ANIMS[player1.player_direction][player1.anim_ticks/PLAYER_TICKS_PER_FRAME]);
		meta_spr(player2.x/256, player2.y/256, 0, PLAYER_ANIMS[player2.player_direction][player2.anim_ticks/PLAYER_TICKS_PER_FRAME]);

		draw_sparkles();
		
		// Draw goal trash sprites (may want to change position / flicker later, but if in corners don't have to worry about that as much)
		meta_spr(16, 188, TRASH_PAL[player1.goal_trash_type], TRASH_METAS[player1.goal_trash_type]);
		meta_spr(224, 188, TRASH_PAL[player2.goal_trash_type], TRASH_METAS[player2.goal_trash_type]);
		
		update_reticles();
		draw_arena();

		
		px_spr_end();
		// px_profile_end();
		px_wait_nmi();
	}
	
	game_over_screen();
}

// 0 is a tie, 1 is player 1, 2 is player 2
static u8 get_winner(){
	if (player1.score > player2.score) return 1;
	if (player1.score < player2.score) return 2;

	return 0;
}

static void game_over_screen(){
	px_ppu_sync_disable();{
		// Decompress the tileset into character memory.
		px_lz4_to_vram(CHR_ADDR(0, 0), CHR0);
		// Set which tiles to use for the background and sprites.
		px_bg_table(0);
		px_spr_table(0);
		
		// clear the screen
		px_addr(NT_ADDR(0, 0, 0));
		px_fill(1024, 0);
		PX.scroll_x = 0;
	} px_ppu_sync_enable();

	px_spr_clear();
	
	{
		u8 winner = get_winner();
		// Draw who winner is
		char buffer[10];
		if (winner == 0)      sprintf(buffer, "Tie! Both Win!");
		else if (winner == 1) sprintf(buffer, "Player 1 Wins!");
		else if (winner == 2) sprintf(buffer, "Player 2 Wins!");
		else                  sprintf(buffer, "ERROR! ? Wins!");
		px_buffer_blit(NT_ADDR(0, 9, 6), buffer, strlen(buffer));
		
		// Draw player 1 score
		sprintf(buffer, "Player 1:");
		px_buffer_blit(NT_ADDR(0, 4, 14), buffer, strlen(buffer));
		
		sprintf(buffer, "%04d00", player1.score);
		px_buffer_blit(NT_ADDR(0, 4, 16), buffer, strlen(buffer));
		
		// Draw player 2 score
		sprintf(buffer, "Player 2:");
		px_buffer_blit(NT_ADDR(0, 19, 14), buffer, strlen(buffer));

		sprintf(buffer, "%04d00", player2.score);
		px_buffer_blit(NT_ADDR(0, 22, 16), buffer, strlen(buffer));

		
	}
	
	while (true)
	{
		u8 winner = get_winner();
		read_gamepads();
		
		if (JOY_START(pad1.value) || JOY_START(pad2.value)) {
			game_run();
		}

		
		// Show happy/sad faces for winner/loser
		if       (winner == 0){ // Tie
			meta_spr(56, 152, 0, HAPPY_FACE_META);
			meta_spr(184, 152, 0, HAPPY_FACE_META);
		}else if (winner == 1){ // 1 wins
			meta_spr(56, 152, 0, HAPPY_FACE_META);
			meta_spr(184, 152, 0, SAD_FACE_META);
		}else if (winner == 2){ // 2 wins
			meta_spr(56, 152, 0, SAD_FACE_META);
			meta_spr(184, 152, 0, HAPPY_FACE_META);
		}

		
		px_spr_end();
		px_wait_nmi();
	}
}

static void title_screen(){
	px_ppu_sync_disable();{
		// Decompress the tileset into character memory.
		px_lz4_to_vram(CHR_ADDR(0, 0), CHR_TITLE);
		// Set which tiles to use for the background and sprites.
		px_bg_table(0);
		px_spr_table(0);
		
		px_lz4_to_vram(NT_ADDR(0, 0, 0), MAP_TITLE);
		PX.scroll_x = 0;
	} px_ppu_sync_enable();

	px_spr_clear();
	
	fade_from_black(PAL_TITLE, 4);
	
	while (true){
		read_gamepads();
		if (JOY_START(pad1.value) || JOY_START(pad2.value)) break;
		// scramble the seed
		rand_seed++;
		
		px_spr_end();
		px_wait_nmi();
	}
	
	fade_to_black(PAL_TITLE, 4);
	game_run();
}

static void igda_screen(){
	px_ppu_sync_disable();{
		// Decompress the tileset into character memory.
		px_lz4_to_vram(CHR_ADDR(0, 0), CHR_IGDA);
		// Set which tiles to use for the background and sprites.
		px_bg_table(0);
		px_spr_table(0);
		
		px_lz4_to_vram(NT_ADDR(0, 0, 0), MAP_IGDA);
		PX.scroll_x = 0;
	} px_ppu_sync_enable();

	px_spr_clear();
	
	fade_from_black(PAL_IGDA, 4);
	
	while (true){
		read_gamepads();
		if (JOY_START(pad1.value) || JOY_START(pad2.value)) break;
		// scramble the seed
		rand_seed++;
		
		px_spr_end();
		px_wait_nmi();
	}
	
	fade_to_black(PAL_IGDA, 4);
	title_screen();
}

void main(void){
	// Set up CC65 joystick driver.
	joy_install(nes_stdjoy_joy);
	
	// Not using bank switching, but a good idea to set a reliable value at boot.
	px_uxrom_select(0);
	
	// Black out the palette.
	for(idx = 0; idx < 32; idx++) px_buffer_set_color(idx, 0x1D);
	px_wait_nmi();
	
	// music_init(&MUSIC);
	sound_init(&SOUNDS);
	// music_play(0);
	
	rand_seed = 0x7A3B;
	px_debug_hex_addr = NT_ADDR(0, 3, 3);
	
	// Jump to the splash screen state.
	igda_screen();
	// game_run();
	
}
