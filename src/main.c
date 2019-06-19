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
#include "gfx/out/gui_gfx.h"

/* Put your function prototypes here */

uint8_t getShipData(uint8_t sidx);
uint8_t prevShipIndex(uint8_t sidx);
uint8_t nextShipIndex(uint8_t sidx);
void renderShipFile(uint8_t pos,uint8_t index);  //pos: 0,1,2. index is shipselidx




void keywait(void);
void waitanykey(void);
void error(char *msg);

/* Put all your globals here */
gfx_sprite_t *mainsprite;
gfx_sprite_t *altsprite;
gfx_sprite_t *tempblock_scratch;
gfx_sprite_t *tempblock_smallscratch;

/* Globals and defines that will be moved out to a new file once done testing */
uint8_t shipselidx;



#define STRI_TOPY (20+64+4+((64-32)/2))
#define STRI_BTMY (20+64+4+((64-32)/2)+32)
#define STRI_MIDY (20+64+4+((64-32)/2)+32-16)
#define STRI_LX (64-32)
#define STRI_RX (64+128+64+(64-32))




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
	loadBuiltinBlueprint(0);
	
	/* INITIALIZE DEBUG LOGIC */
	
	//openEditor();  //DEBUGGING: EDITOR

	
	//Initialize selector render
	shipselidx = 0;  //You will always have a starting blueprint
	
	
	
	
	
	while (1) {
		kb_Scan();
		kd = kb_Data[7];
		kc = kb_Data[1];
		gfx_FillScreen(COLOR_BLACK);
		if (kc&kb_Mode) { keywait(); break; }
		
		if (kd == kb_Up) {
			if (shipselidx) shipselidx = prevShipIndex(shipselidx);
		}
		if (kd == kb_Down) {
			if (0xFF != nextShipIndex(shipselidx))
				shipselidx = nextShipIndex(shipselidx);
		}
		
		
		
		gfx_SetColor(0x56); //A faded blue
		gfx_FillTriangle(STRI_LX,STRI_TOPY,STRI_LX,STRI_BTMY,STRI_LX+24,STRI_MIDY); //L
		gfx_FillTriangle(STRI_RX,STRI_TOPY,STRI_RX,STRI_BTMY,STRI_RX-24,STRI_MIDY); //R
		gfx_SetColor(0xD6); //A faded blue
		gfx_FillTriangle(STRI_LX+2,STRI_TOPY+4,STRI_LX+2,STRI_BTMY-4,STRI_LX+24-4,STRI_MIDY);
		gfx_FillTriangle(STRI_RX-2,STRI_TOPY+4,STRI_RX-2,STRI_BTMY-4,STRI_RX-24+4,STRI_MIDY);
		
		
		
		
		/*
		gfx_SetColor(0x56); //A faded blue
		gfx_FillRectangle(64,20,(128+64),64);              //Opt 1
		gfx_SetColor(0x65); //A faded red
		gfx_FillRectangle(64,(20+64+4),(128+64),64);       //Opt 2
		gfx_FillRectangle(64,(20+64+4+64+4),(128+64),64);  //Opt 3
		*/
		renderShipFile(1,0);
		
		
		
		
		gfx_BlitBuffer();
		if (kd|kc) keywait();
		
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


//Writes to temp_blueprint and temp_bpgrid when called
//Returns 0 if success, something else if failure
uint8_t getShipData(uint8_t sidx) {
	uint8_t i;
	if (sidx<8) {
		//Detect internal blueprint
		if ((1<<sidx)&gamedata.blueprints_owned) {
			loadBuiltinBlueprint(sidx);
		} else return 1;
	} else {
		//Find external blueprint
		loadBlueprint(sidx-8);
		if (!temp_blueprint.gridlevel) return 1;
	}
	return 0;
}

//Returns 0xFF if object not found, otherwise returns something between 0 and 13
uint8_t prevShipIndex(uint8_t sidx) {
	while ((--sidx) != 0xFF) {
		if (!getShipData(sidx)) return sidx;
	}
	return 0xFF;
}

//Returns 0xFF if object not found, otherwise returns something between 0 and 13
uint8_t nextShipIndex(uint8_t sidx) {
	while ((++sidx) < (8+gamedata.custom_blueprints_owned)) {
		if (!getShipData(sidx)) return sidx;
	}
	return 0xFF;
}


// No prototype. Bundle with renderShipFile
void rsf_textline(int x,uint8_t *y, char *s, int stat) {
	gfx_SetTextXY(x+(128+2),*y);
	gfx_PrintString(s);
	gfx_PrintChar(' ');
	gfx_PrintUInt(stat,3);
	*y += 10;
}
//pos: 0,1,2. 1 is the middle option
void renderShipFile(uint8_t pos,uint8_t index) {
	uint8_t i,ybase,colorbase;
	int xbase;
	
	if (getShipData(index)) return;
	curblueprint = &temp_blueprint;
	curblueprint->blocks = temp_bpgrid;
	
	
	xbase = 64;
	ybase = 20+pos*(64+4);
	
	if (index<8) colorbase = 0xE5; //A faded red, lighter
	else         colorbase = 0xD6; //A faded blue, lighter
	
	//Draw outline
	gfx_SetColor(colorbase);
	gfx_Rectangle_NoClip(xbase,ybase,(128+64),64);
	gfx_SetColor(colorbase & ((1<<6)|0x3F)); //light
	gfx_Rectangle_NoClip(xbase+1,ybase+1,(128+64-2),(64-2));
	
	gfx_SetColor(((colorbase>>1)&0x15) | (2<<6)); //darkshift, lighter
	gfx_FillRectangle_NoClip(xbase+2,ybase+2,(128+64-4),(64-4));
	//Draw blueprint name
	gfx_SetTextFGColor(COLOR_WHITE);
	gfx_PrintStringXY(curblueprint->name,xbase+4,ybase+2);
	//Draw preview
	drawShipPreview(mainsprite);
	gfx_TransparentSprite_NoClip(mainsprite,xbase+8,ybase+14);
	

	sumStats();
	ybase += 3;
	rsf_textline(xbase+8,&ybase,"HP",bpstats.hp);
	rsf_textline(xbase+8,&ybase,"PW",bpstats.power);
	rsf_textline(xbase,&ybase,"ATK",bpstats.atk);
	rsf_textline(xbase,&ybase,"DEF",bpstats.def);
	rsf_textline(xbase,&ybase,"SPD",bpstats.spd);
	rsf_textline(xbase,&ybase,"AGI",bpstats.agi);
	
	
	
	
	
	
}














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
