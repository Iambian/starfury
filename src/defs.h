#ifndef __defs_cfile__
#define __defs_cfile__

#include <stdint.h>
#include <graphx.h>		//Provides typedefs for graphx objects

#define VERSION_MAIN 0
#define VERSION_SUB 01
#define FILE_VERSION 3

/* DEFINES FOR GAME STATE */
#define GS_STATE_MASK 0xF0

#define GS_TITLE 0x00

#define GS_SHIPSELECT 0x10

#define GS_EDITOR 0x20
#define GS_EDITSHIP (GS_EDITOR|1)
#define GS_EDITCOLOR (GS_EDITOR|2)
#define GS_EDITBLOCKS (GS_EDITOR|3)

#define GS_STAGESELECT 0x30

#define GS_STAGESTART 0x40

#define GS_STAGERUNNING 0x50

#define GS_STAGEEND 0x60

#define GS_GAMEOVER 0xE0
#define GS_GAMEOVERDYING (GS_GAMEOVER|1)
#define GS_GAMEOVERDISPLAY (GS_GAMEOVER|2)

/* DEFINES FOR COLORS */
#define COLOR_DARKER (0<<6)
#define COLOR_DARK (1<<6)
#define COLOR_LIGHT (2<<6)
#define COLOR_LIGHTER (3<<6)

#define COLOR_RED (3<<4)
#define COLOR_MAROON (2<<4)
#define COLOR_LIME (3<<2)
#define COLOR_GREEN (2<<2)
#define COLOR_BLUE (3<<0)
#define COLOR_NAVY (2<<0)

#define COLOR_MAGENTA (COLOR_RED|COLOR_BLUE)
#define COLOR_PURPLE (COLOR_MAROON|COLOR_NAVY)
#define COLOR_YELLOW (COLOR_RED|COLOR_LIME)
#define COLOR_CYAN (COLOR_LIME|COLOR_BLUE)
#define COLOR_WHITE (COLOR_RED|COLOR_BLUE|COLOR_LIME)
#define COLOR_GRAY (COLOR_MAROON|COLOR_GREEN|COLOR_NAVY)
#define COLOR_DARKGRAY ((1<<4)|(1<<2)|(1<<0))
#define COLOR_BLACK 0

#define TRANSPARENT_COLOR (COLOR_LIGHTER|COLOR_MAGENTA)

#define DEFAULT_COLOR COLOR_GREEN

/* DEFINES FOR VARIOUS SIZES */

#define TEMPBLOCK_MAX_W 32
#define TEMPBLOCK_MAX_H 32
#define PREVIEWBLOCK_MAX_W 48
#define PREVIEWBLOCK_MAX_H 48

#define MAX_FIELD_OBJECTS 512
#define MAX_ENEMY_OBJECTS 32


/* DEFINES FOR CONSTANT BLOCK STATS */

#define GRID_LEVEL_1 1
#define GRID_LEVEL_2 2
#define GRID_LEVEL_3 3
//Rotation is done clockwise by 90 degrees apiece. HFLIP is OR'd onto rotation
#define ROT_0	0
#define ROT_1	1
#define ROT_2	2
#define ROT_3	3
#define HFLIP	4



/* ENUMS AND TYPEDEFS */

enum BKT { FILLER=0,COMMAND=1,ENGINE=2,WEAPON=4,SPECIAL=8,ENERGYSOURCE=16 };
//enum DIR { UP=0,UPRIGHT,RIGHT,DOWNRIGHT,DOWN,DOWNLEFT,LEFT,UPLEFT };
enum BLPT { BP_BASIC=1,BP_INTERMEDIATE=2,BP_ADVANCED=4,BP_EXTRA=8 };
enum ESTAT { EDIT_SELECT=1,COLOR_SELECT=2,FILE_SELECT=4,NOW_PAINTING=8,
			 PICKING_COLOR=16,OTHER_CONFIRM=32,QUIT_CONFIRM=128};
enum PANEL { PAN_INV=1,PAN_LIM=2,PAN_RIGHT=4,PAN_CURSEL=8,PAN_STATUS=16,
			 PAN_GRID=32,PAN_FULLSCREEN=64,PAN_PREVIEW=128};
enum FIELDOBJ {FOB_PBUL=1,FOB_EBUL=2,FOB_ITEM=4,FOB_EFFECT=16};
//


typedef union fp168_union {
	int fp;
	struct {
		uint8_t fpart;
		int16_t ipart;
	} p;
} fp168;


/* Used to represent the location of every block on the ship construction grid */
typedef struct gridblock_obj_struct {
	uint8_t block_id;    //0 = null, 1-255 = blockID
	uint8_t x;           //0-11 X position on blockgrid
	uint8_t y;           //0-11 Y position on blockgrid
	uint8_t color;       //0b00rrggbb r=0-3 red, g=0-3 green, b=0-3 blue
	uint8_t orientation; //0b00000frr r=0-3 number of rotates, f=is_horiz_flip
} gridblock_obj;

/* Used to define a collection of blocks that would form up a ship */
typedef struct blueprint_obj_struct {
	uint8_t gridlevel; //A gridlevel of 0 indicates that this was deleted.
	uint8_t numblocks;
	char name[17];
	gridblock_obj *blocks;
} blueprint_obj;


/* Used internally to represent the properties of every block in the game */
typedef struct blockprop_obj_struct {
	gfx_sprite_t *sprite;  	//Address to sprite object
	uint8_t type;			//Block type, uses BLOCKTYPE enum but need uint8_t
	uint8_t cost;           //
	uint8_t w;				//width, in blocks. usually 1.
	uint8_t h;				//height, in blocks. usually 1.
	uint8_t hp;				//HP value that this piece gives the ship.
	uint8_t atk;			//Attack value that this piece provides
	uint8_t def;			//Defense value that this piece provides
	uint8_t agi;			//Affects how fast the ship moves around onscreen
	uint8_t spd;			//Controls how fast the ship moves through the stage
	char *name;         	//Display name of object in question
} blockprop_obj;

typedef struct field_obj_struct {
	uint8_t id;
	uint8_t flag;  //Should use FIELDOBJ enum for populating this
	uint8_t counter;
	uint8_t power;
	unsigned int data;
	fp168 x,y,dx,dy;
	void (*fMov)(struct field_obj_struct *fobj);
} field_obj;
typedef struct enemy_data_struct {
	uint8_t id;
	int hp;
	uint8_t atk,def;
	gfx_sprite_t *sprite;
	uint8_t *moveScript;
	void (*fShoot)(struct enemy_obj_struct *eobj,uint8_t angle);
} enemy_data;
typedef struct enobj_loop_struct { uint8_t *ptr,cnt; } enobj_loop;
typedef struct enemy_obj_struct {
	uint8_t id;
	int hp;
	uint8_t atk,def;
	gfx_sprite_t *sprite;
	uint8_t *moveScript;
	void (*fShoot)(struct enemy_obj_struct *eobj,uint8_t angle);
	
	fp168 x,y;
	uint8_t hbx,hby; //Trickery needs to be done here. x dimension is half-res
	uint8_t hbw,hbh; //to keep in uint8_t and to improve performance (on ASM write)
	int timer,data1,data2;
	uint8_t loopdepth;
	enobj_loop s_loop[4];
	uint8_t s_acc,s_rad,s_ang,s_dang;
	uint8_t s_circy;
	int s_circx;
	uint8_t hitcounter; //increment by 2, invert on every nonzero even
} enemy_obj;
typedef struct weapon_obj_struct {
	uint8_t xoffset,yoffset;
	uint8_t fire_direction;
	uint8_t cooldown;
	uint8_t cooldown_on_firing; //Use this to set cooldown after firing
	uint8_t power;              //Strength of the bullet that it fires
	void (*fShot)(struct weapon_obj_struct *wobj);
} weapon_obj;











#endif