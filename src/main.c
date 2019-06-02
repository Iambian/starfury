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

/* Put your function prototypes here */
void setup_palette(void);
void keywait(void);
void waitanykey(void);
void initGridArea(uint8_t grlevel);
void drawGridArea(void);

void error(char *msg);

/* Put all your globals here */

gfx_sprite_t *buildarea;  //Up to 96x96 upscaled by 2 during render.
uint8_t gridlevel;        //Allowed values 1,2,3. 0 is ignored






void main(void) {
	kb_key_t kc,kd;
	uint8_t i;
	
    gfx_Begin(gfx_8bpp);
	gfx_SetDrawBuffer();
	setup_palette();
	gfx_SetTransparentColor(TRANSPARENT_COLOR);
	/* Load save file */
	
	/* Insert game logic here */
	buildarea = NULL; /*Unsure if this is BSS. Added to ensure value if not */
	
	
	/* INITIALIZE DEBUG LOGIC */
	initGridArea(3);  //max size
	
	
	
	gfx_MallocSprite(96,96);  //Up to 96x96 (then x2 on render). Can be 
	dbg_sprintf(dbgout,"Data output %i: ",blockobject_list[3].w);
	
	/* Start game */
	while (1) {
		kb_Scan();
		kd = kb_Data[7];
		kc = kb_Data[1];
		if (kc&kb_Mode) { keywait(); break; }
		
		//maintenance
		gfx_FillScreen(COLOR_BLACK);
		//inventory bar
		gfx_SetColor(COLOR_GRAY|COLOR_LIGHTER);
		gfx_FillRectangle_NoClip(0,0,64,164); //full bar
		gfx_SetColor(COLOR_GRAY|COLOR_LIGHT);
		gfx_FillRectangle_NoClip(0,58,64,48); //cur select
		//preview
		gfx_SetColor(COLOR_GRAY|COLOR_DARKER);
		gfx_FillRectangle_NoClip(0,164,64,64);
		//status bar
		gfx_SetColor(COLOR_GRAY|COLOR_LIGHTER);
		gfx_FillRectangle_NoClip(0,164+64,320,12);
		//cursel block stats
		gfx_SetColor(COLOR_DARKGRAY|COLOR_DARKER);
		gfx_FillRectangle_NoClip(64,(164+64-12),(320-64),12);
		//ship grid build area
		gfx_SetColor(COLOR_WHITE|COLOR_DARKER);
		gfx_FillRectangle_NoClip(64+6,24,192,192);
		//Limit field
		gfx_SetColor(COLOR_GRAY|COLOR_LIGHTER);
		gfx_FillRectangle_NoClip((320-64),0,64,24);
		//color and stats field
		gfx_SetColor(COLOR_GRAY|COLOR_DARKER);
		gfx_FillRectangle_NoClip((64+6+192+6),24,48,192);
		
		gfx_BlitBuffer();
		
	}
	/* Preserve save file and exit */
//	int_Reset();
	gfx_End();
//	slot = ti_Open("BkShpDAT","w");
//	ti_Write(&highscore,sizeof highscore, 1, slot);
	ti_CloseAll();
	exit(0);
	
}

/* Put other functions here */


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

void initGridArea(uint8_t grlevel) {
	uint8_t v;
	grlevel &= 3;
	if (!grlevel) ++grlevel;
	gridlevel = grlevel;
	if (buildarea) free(buildarea);
	//1,2,3 (*2) to 2,4,6 (+6) to 8,10,12 (*8) to 64,80,96
	v = ((grlevel<<1)+6)<<3;
	if (!(buildarea = gfx_MallocSprite(v,v))) {
		error("Buildarea malloc fail");
	}
	fn_FillSprite(buildarea,COLOR_RED);  //temp color. set to 0 later
	
}

void drawGridArea(void) {
	int sx,tx;
	uint8_t offset,sy,ty,cols,gridy,gridx,limit;
	offset = 32-((gridlevel-1)<<4);
	sx = 64+4+offset;
	sy = 24+offset;
	limit = (gridlevel<<1)+6;  //8,10,12
	gfx_SetColor(COLOR_GRAY|COLOR_LIGHTER);
	for (gridy=0;gridy<limit;gridy++) {
		for (gridx=0;gridx<limit;gridx++) {
			gfx_Rectangle_NoClip(sx+(gridx*16),sy+(gridy*16),16,16);
		}
	}
	gfx_ScaledTransparentSprite_NoClip(buildarea,64+6+offset,24+offset,2,2);
}


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
