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
char nameinput[17];

/* Globals and defines that will be moved out to a new file once done testing */






void main(void) {
	kb_key_t kc,kd;
	uint8_t i,update_flags,tx,ty,t,limit;
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

	shipSelect();  //DEBUGGING: SHIP SELECTION
	
	/*
	while (1) {
		kb_Scan();
		kd = kb_Data[7];
		kc = kb_Data[1];
		if (kc&kb_Mode) { keywait(); break; }
		
		gfx_BlitBuffer();
		if (kd|kc) keywait();
		
	}
	*/
	
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
