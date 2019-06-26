/*
 *--------------------------------------
 * Program Name: Starfury
 * Author: Rodger (Iambian) Weisman
 * License: 
 * Description: Explore the dangerous cosmos with a loony customizable spaceship
 *--------------------------------------
*/

/* Keep these headers */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

/* Standard headers (recommended) */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External library headers */
#include <debug.h>
#include <intce.h>
#include <keypadc.h>
#include <graphx.h>
#include <decompress.h>
#include <fileioc.h>

#include "defs.h"
#include "bdata.h"
#include "util.h"
#include "dataio.h"
#include "edit.h"
#include "sselect.h"
#include "gfx/out/gui_gfx.h"
#include "menu.h"

/* Put your function prototypes here */





void keywait(void);
void waitanykey(void);
void error(char *msg);

/* Put all your globals here */
gfx_sprite_t *mainsprite;
gfx_sprite_t *altsprite;
gfx_sprite_t *tempblock_scratch;
gfx_sprite_t *tempblock_smallscratch;

//Field objects can include player bullets, enemy bullets, and powerups
enum FIELDOBJ {FOB_PBUL=1,FOB_EBUL=2,FOB_ITEM=4};
typedef struct field_obj_struct {
	uint8_t id;
	uint8_t flag;  //Should use FIELDOBJ enum for populating this
	uint8_t counter;
	uint8_t power;
	unsigned int data;
	fp168 x,y,dx,dy;
	void (*fMov)(struct field_obj_struct *fobj);
} field_obj;
typedef struct enemy_obj_struct {
	uint8_t id;
	fp168 x,y,dx,dy;
	int hp;
	uint8_t armor;
	uint8_t hbx,hby; //Trickery needs to be done here. x dimension is half-res
	uint8_t hbw,hbh; //to keep in uint8_t and to improve performance (on ASM write)
} enemy_obj;
typedef struct weapon_obj_struct {
	uint8_t xoffset,yoffset;
	uint8_t fire_direction;
	uint8_t cooldown;
	uint8_t cooldown_on_firing; //Use this to set cooldown after firing
	uint8_t power;              //Strength of the bullet that it fires
	void (*fShot)(struct weapon_obj_struct *wobj);
} weapon_obj;

field_obj  empty_fobj;
enemy_obj  empty_eobj;
weapon_obj empty_wobj;

/* Globals and defines that will be moved out to a new file once done testing */
uint8_t maintimer;
field_obj *fobjs;
enemy_obj *eobjs;
weapon_obj *wobjs;

fp168 playerx,playery;
int player_curhp;  //Maximums and other stats located in bpstats struct




void main(void) {
	kb_key_t kc,kd;
	fp168 tempfp;
	int tempint;
	gridblock_obj *gbo;
	blockprop_obj *bpo;
	uint8_t i,k,update_flags,tx,ty,t,limit;
	uint8_t tt;

    gfx_Begin(gfx_8bpp);
	gfx_SetDrawBuffer();
	//setup_palette();
	fn_Setup_Palette();
	gfx_SetTransparentColor(TRANSPARENT_COLOR);
	gfx_SetTextTransparentColor(TRANSPARENT_COLOR);
	gfx_SetTextBGColor(TRANSPARENT_COLOR);
	mainsprite = gfx_MallocSprite(PREVIEWBLOCK_MAX_W,PREVIEWBLOCK_MAX_H);
	altsprite = gfx_MallocSprite(PREVIEWBLOCK_MAX_W,PREVIEWBLOCK_MAX_H);
	tempblock_scratch = gfx_MallocSprite(PREVIEWBLOCK_MAX_W,PREVIEWBLOCK_MAX_H);
	tempblock_smallscratch = gfx_MallocSprite(PREVIEWBLOCK_MAX_W/2,PREVIEWBLOCK_MAX_H/2);

	
	ti_CloseAll();
	/* Load save file */
	
	/* Initialize game defaults */
	initPlayerData();
	if (!openSaveReader()) error("Can't open/write save file");
	//loadBuiltinBlueprint(0);
	
	/* INITIALIZE DEBUG LOGIC */
	
	//openEditor();  //DEBUGGING: EDITOR
	//shipSelect();  //DEBUGGING: SHIP SELECTION
	
	//Initialize gameplay test environment
	loadBuiltinBlueprint(0);      //Load first builtin blueprint
	drawShipPreview(mainsprite);  //Prerender the ship
	curblueprint = &temp_blueprint;  //idk why this isn't done already but meh
	sumStats();  //bpstats.[hp,power,weight,atk,def,spd,agi,wpn,cmd]
	
	
	//Allocate and initialize object memory
	fobjs = malloc(MAX_FIELD_OBJECTS * (sizeof empty_fobj));
	memset(fobjs,0,MAX_FIELD_OBJECTS * (sizeof empty_fobj));
	eobjs = malloc(MAX_ENEMY_OBJECTS * (sizeof empty_eobj));
	memset(eobjs,0,MAX_ENEMY_OBJECTS * (sizeof empty_eobj));
	if (bpstats.wpn) {
		wobjs = malloc(bpstats.wpn * (sizeof empty_wobj));
		memset(wobjs,0,bpstats.wpn * (sizeof empty_wobj));
	} else 	wobjs = NULL;
	//Initialize weapon memory
	for (k=i=0;i<curblueprint->numblocks;i++) {
		bpo = &blockobject_list[(gbo = &curblueprint->blocks[i])->block_id];
		if (bpo->type & WEAPON) {
			t = 3-curblueprint->gridlevel; //offset
			wobjs[k].xoffset = (gbo->x+t)<<2;
			wobjs[k].yoffset = (gbo->y+t)<<2;
			t = gbo->orientation;
			//collapse hflip to 180 deg orientation for direction and change to DIR enum
			wobjs[k].fire_direction = ((t&4)>>1)^(t&3);
			//wobjs[k].cooldown = 0; //Not necessary -- already zeroed w/ memset
			switch (gbo->block_id) {
				case BLOCK_TUR1: t =  5; tt =  1; break;
				case BLOCK_TUR2: t =  5; tt =  2; break;
				case BLOCK_TUR3: t =  5; tt =  3; break;
				case BLOCK_LAS1: t = 20; tt =  1; break;
				case BLOCK_LAS2: t = 20; tt =  2; break;
				case BLOCK_LAS3: t = 15; tt =  3; break;
				case BLOCK_MIS1: t = 40; tt = 20; break;
				case BLOCK_MIS2: t = 40; tt = 30; break;
				case BLOCK_MIS3: t = 40; tt = 40; break;
				default: t = 10; tt = 0; break;
			}
			wobjs[k].cooldown_on_firing = t;
			wobjs[k].power = bpstats.atk + tt;
		}
	}
	
	
	keywait();
	// I think we have enough object memory allocated and initialized?
	while (1) {
		kb_Scan();
		kd = kb_Data[7];
		kc = kb_Data[1];
		if (kc&kb_Mode) { keywait(); break; }
		
		if (kd&kb_Left) {
			if ((tempint = playerx.fp-(128*bpstats.spd))> 0) {
				playerx.fp = tempint;
			} else playerx.fp = 0;
		}
		if (kd&kb_Right) {
			if ((tempint = playerx.fp+(128*bpstats.spd))< ((320-48)*256)) {
				playerx.fp = tempint;
			} else playerx.fp = ((320-48)*256);
		}
		if (kd&kb_Up) {
			if ((tempint = playery.fp-(128*bpstats.spd))> 0) {
				playery.fp = tempint;
			} else playery.fp = 0;
		}
		if (kd&kb_Down) {
			if ((tempint = playery.fp+(128*bpstats.spd))< ((240-48)*256)) {
				playery.fp = tempint;
			} else playery.fp = ((240-48)*256);
		}
		
		
		gfx_FillScreen(COLOR_BLACK);
		
		gfx_TransparentSprite_NoClip(mainsprite,playerx.p.ipart,playery.p.ipart);
		
		
		
		
		
		gfx_BlitBuffer();
		
	}
	
	//Battle system cleanup
	free(fobjs);
	free(eobjs);
	free(wobjs);
	
	
	
	
	/* Preserve save file and exit */
//	int_Reset();
	gfx_End();
	saveGameData();
	ti_CloseAll();
	exit(0);
}

/* Put other functions here */






/*
void setup_palette(void) {
	int i;
	uint16_t *palette,tempcolor;
	palette = (uint16_t*)0xE30200;  //512 byte palette. arranged 0b_IBBB_BBGG_GGGR_RRRR
	i = 0;
	do {
		//i = 0b_II_RR_GG_BB. First intensity, then color.
		//Process red first. Write to low %000RRII0
		tempcolor = ((i>>5)&(3<<1))|((i>>1)&(3<<3));
		//Process green next. Write to full %000000GG.II000000
		tempcolor |= ((i<<0)&(3<<6))|((i<<6)&(3<<8));
		//Process blue last. Write to last %0BBII000
		tempcolor |= ((i<<5)&(3<<11))|((i<<11)&(3<<13));
		//Then write the color data out to memory
		palette[i] = tempcolor;
	} while (++i<256);
	return;
}
*/




void waitanykey(void) {
	keywait();            //wait until all keys are released
	while (!kb_AnyKey()); //wait until a key has been pressed.
	while (kb_AnyKey());  //make sure key is released before advancing
}	
void keywait(void) {
	while (kb_AnyKey());  //wait until all keys are released
}
void error(char *msg) {
	gfx_End();
	asm_ClrLCDFull();
	os_NewLine();
	os_PutStrFull("Fatal error:");
	os_NewLine();
	os_PutStrFull(msg);
	exit(1);
}
