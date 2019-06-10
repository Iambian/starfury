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
void initGridArea(uint8_t grlevel);
void drawGridArea(void);
void drawInventory(void);
uint8_t getPrevInvIndex(uint8_t cidx);
uint8_t getNextInvIndex(uint8_t cidx);

void keywait(void);
void waitanykey(void);
void error(char *msg);

/* Put all your globals here */
uint8_t *inventory; //256 entry wide, indexed by position. Set from save file l8r
blueprint_obj *curblueprint;

gfx_sprite_t *buildarea;		//Up to 96x96 upscaled by 2 during render.
gfx_sprite_t *tempblock_grid; 	//Up to 32x32 (4x4 block)
gfx_sprite_t *tempblock_inv;    //Up to 32x32 (4x4 block)
gfx_sprite_t *tempblock_scratch;	//Up to 32x32 (4x4 block)
uint8_t gridlevel;        //Allowed values 1,2,3. 0 is ignored
uint8_t curcolor;         //Currently selected color (keep intensity 0)
uint8_t curindex;         //Index of currently selected object





void main(void) {
	kb_key_t kc,kd;
	uint8_t i;
	
    gfx_Begin(gfx_8bpp);
	gfx_SetDrawBuffer();
	//setup_palette();
	fn_Setup_Palette();
	gfx_SetTransparentColor(TRANSPARENT_COLOR);
	/* Load save file */
	
	/* Insert game logic here */
	curcolor = DEFAULT_COLOR;
	buildarea = NULL; /*Unsure if this is BSS. Added to ensure value if not */
	tempblock_grid = gfx_MallocSprite(TEMPBLOCK_MAX_W,TEMPBLOCK_MAX_H);
	tempblock_inv  = gfx_MallocSprite(TEMPBLOCK_MAX_W,TEMPBLOCK_MAX_H);
	tempblock_scratch = gfx_MallocSprite(TEMPBLOCK_MAX_W,TEMPBLOCK_MAX_H);
	
	/* INITIALIZE DEBUG LOGIC */
	inventory = malloc(256);
	for (i=2;i<20;i++) inventory[i] = 5;  //skip cmd unit 1 for testing
	initGridArea(3);  //max size. Sets gridlevel.
	curindex = getNextInvIndex(getPrevInvIndex(0));
	curblueprint = &basic_blueprint;
	
	/* Start game */
	while (1) {
		kb_Scan();
		kd = kb_Data[7];
		kc = kb_Data[1];
		if (kc&kb_Mode) { keywait(); break; }
		if (kd&kb_Down) { curindex = getNextInvIndex(curindex); keywait(); }
		if (kd&kb_Up)   { curindex = getPrevInvIndex(curindex); keywait(); }
		
		//maintenance
		gfx_FillScreen(COLOR_BLACK);
		//inventory bar
		drawInventory();
		//preview
		gfx_SetColor(COLOR_BLUE|COLOR_DARKER);
		gfx_Rectangle_NoClip(0,164,64,64);
		//status bar
		gfx_SetColor(COLOR_GRAY|COLOR_LIGHTER);
		gfx_FillRectangle_NoClip(0,164+64,320,12);
		//cursel block stats
		gfx_SetColor(COLOR_DARKGRAY|COLOR_DARKER);
		gfx_FillRectangle_NoClip(64,(164+64-12),(320-64),12);
		//ship grid build area
		drawGridArea();
//		gfx_SetColor(COLOR_WHITE|COLOR_DARKER);
//		gfx_FillRectangle_NoClip(64+6,24,192,192);
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
	fn_FillSprite(buildarea,TRANSPARENT_COLOR);  //temp color. set to 0 later
	
}

void drawGridArea(void) {
	int sx,tx;
	uint8_t offset,sy,ty,cols,gridy,gridx,limit;
	uint8_t xoff,yoff,i,orientation,spriteid;
	gfx_sprite_t *srcsprite;
	offset = 32-((gridlevel-1)<<4);
	sx = 64+6+offset;
	sy = 24+offset;
	limit = (gridlevel<<1)+6;  //8,10,12
	gfx_SetColor(COLOR_GRAY|COLOR_LIGHTER);
	for (gridy=0;gridy<limit;gridy++) {
		for (gridx=0;gridx<limit;gridx++) {
			gfx_Rectangle_NoClip(sx+(gridx*16),sy+(gridy*16),16,16);
		}
	}
	if (curblueprint) {
		xoff = yoff = gridlevel-curblueprint->gridlevel;
		for (i=curblueprint->numblocks;i;i--) {
			orientation = curblueprint->blocks[i].orientation;
			spriteid = curblueprint->blocks[i].block_id;
			srcsprite = blockobject_list[spriteid].sprite;
			switch (orientation&3) {
				case ROT_0:
					memcpy(tempblock_scratch,srcsprite,34);
					break;
				case ROT_1:
					gfx_RotateSpriteC(srcsprite,tempblock_scratch);
					break;
				case ROT_2:
					gfx_RotateSpriteHalf(srcsprite,tempblock_scratch);
					break;
				case ROT_3:
					gfx_RotateSpriteCC(srcsprite,tempblock_scratch);
					break;
				default: //Can't happen. There's only four possible values.
					break;
			}
			if (orientation&HFLIP) {
				gfx_FlipSpriteX(tempblock_scratch,tempblock_grid);
			} else {
				memcpy(tempblock_grid,tempblock_scratch,34);
			}
			gridx = curblueprint->blocks[i].x-xoff;
			gridy = curblueprint->blocks[i].y-yoff;
			fn_DrawNestedSprite(tempblock_grid,buildarea,8*gridx,8*gridy);
		}
	}
	gfx_ScaledTransparentSprite_NoClip(buildarea,sx,sy,2,2);
}

//No prototype. pos=[0-3], invidx = index to inventory slot
void drawSmallInvBox(uint8_t pos,uint8_t invidx) {
	uint8_t i,nw,nh,nx,ny;
	unsigned int temp;
	static uint8_t smallinv_y_lut[] = {1+6,1+6+28,58+48+1+6,58+48+1+28+6};
	blockprop_obj *blockinfo;
	gfx_sprite_t *srcsprite;
	/* Setup draw area */
	tempblock_inv->width = tempblock_inv->height = 16;
	fn_FillSprite(tempblock_inv,TRANSPARENT_COLOR);
	/* Grab properties of block in question */
	blockinfo = &blockobject_list[invidx];
	dbg_sprintf(dbgout,"block object %i, address %x\n",invidx,blockinfo->sprite);
	if (!(srcsprite = blockinfo->sprite)) return;  //Cancel on NULL sprites
	tempblock_scratch->height = tempblock_scratch->width = 16;
	if ((srcsprite->width > 16) && (srcsprite->width > srcsprite->height)) {
		/*Resize wrt width only if width is more than 16 AND more than height */
		/* w/h=16/H to 16*h/w=H */
		nh = ((unsigned int)srcsprite->height * 16)/srcsprite->width;
		tempblock_scratch->height = nh;
		gfx_ScaleSprite(srcsprite,tempblock_scratch);
		srcsprite = tempblock_scratch; //set pointer to scratch to unify writeback
	} else if (srcsprite->height > 16) {
		/* Otherwise resize wrt height. Test against width and height unnecessary */
		/* w/h = W/16 to 16*w/h=W */
		nw = ((unsigned int)srcsprite->width * 16)/srcsprite->height;
		tempblock_scratch->width = nw;
		gfx_ScaleSprite(srcsprite,tempblock_scratch);
		srcsprite = tempblock_scratch; //set pointer to scratch to unify writeback
	} else {
		memcpy(tempblock_scratch,srcsprite,srcsprite->width*srcsprite->height+2);
		if (tempblock_scratch->width > 16 || tempblock_scratch->height>16) {
			dbg_sprintf(dbgerr,"Sprite copy failure.");
			return;
		}
	}
	//Use tempblock_inv to draw the sprite after centering it.
	//... but for now, just blind copy it.
	fn_FillSprite(tempblock_inv,TRANSPARENT_COLOR);
	nx = (16-tempblock_scratch->width)/2;
	ny = (16-tempblock_scratch->height)/2;
	fn_DrawNestedSprite(tempblock_scratch,tempblock_inv,nx,ny);
	//memcpy(tempblock_inv,tempblock_scratch,tempblock_scratch->width*tempblock_scratch->height+2);
	/* Paint sprite to currently chosen color, then render it */
	//fn_PaintSprite(tempblock_inv,curcolor);
	ny = smallinv_y_lut[pos&3];
	nx = 8;
	gfx_TransparentSprite_NoClip(tempblock_inv,nx,ny);
	//Print other stats according to the diagram
	
	
	
	return;
}

void drawInventory(void) {
	uint8_t i,y,idx,nw,nh,nx,ny;
	unsigned int temp;
	blockprop_obj *blockinfo;
	gfx_sprite_t *srcsprite;
	int x;
	/* Set background */
	gfx_SetColor(COLOR_GRAY|COLOR_LIGHTER);
	gfx_FillRectangle_NoClip(0+6,0,64-6-6,164); //full bar
	gfx_SetColor(COLOR_GRAY|COLOR_LIGHT);
	gfx_FillRectangle_NoClip(0,58,64,48); //cur select
	/* Draw surrounding inventory objects */
	drawSmallInvBox(1,idx = getPrevInvIndex(curindex));
	drawSmallInvBox(0,getPrevInvIndex(idx));
	drawSmallInvBox(2,idx = getNextInvIndex(curindex));
	drawSmallInvBox(3,getNextInvIndex(idx));
	/* Draw currently selected inventory object */
	tempblock_scratch->width = tempblock_scratch->height = 32;
	tempblock_inv->width = tempblock_inv->height = 32;
	fn_FillSprite(tempblock_scratch,TRANSPARENT_COLOR);
	blockinfo = &blockobject_list[curindex];
//	dbg_sprintf(dbgout,"Start: %i\n",curindex);
	if (!(srcsprite = blockinfo->sprite)) return;  //Do not continue if NULL spr
//	dbg_sprintf(dbgout,"\nStop.");
	if (srcsprite->width>16 || srcsprite->height>16) {
		/* Larger than 16. Keep original sprite dimensions and nest into scratch */
		memcpy(tempblock_scratch,srcsprite,srcsprite->width*srcsprite->height+2);
	} else {
		/* Otherwise, double size to scratch then write into inv */
		tempblock_scratch->width = srcsprite->width*2;
		tempblock_scratch->height = srcsprite->height*2;
		gfx_ScaleSprite(srcsprite,tempblock_scratch);
	}
	/* Center tempblock_scratch into tempblock_inv and then color it. */
	//Or not yet...
	fn_FillSprite(tempblock_inv,TRANSPARENT_COLOR);
	nx = (32-tempblock_scratch->width)/2;
	ny = (32-tempblock_scratch->height)/2;
	fn_DrawNestedSprite(tempblock_scratch,tempblock_inv,nx,ny);
	//memcpy(tempblock_inv,tempblock_scratch,tempblock_scratch->width*tempblock_scratch->height+2);
	fn_PaintSprite(tempblock_inv,COLOR_GREEN);
	gfx_TransparentSprite_NoClip(tempblock_inv,2,(58+8));
	//Print other stats according to the diagram
	
	
	
	return;
}

uint8_t getPrevInvIndex(uint8_t cidx) {
	uint8_t i=0;
	do {
		--cidx;
		if (inventory[cidx]) break;
	} while (++i);
	return cidx;
}
uint8_t getNextInvIndex(uint8_t cidx) {
	uint8_t i=0;
	do {
		++cidx;
		if (inventory[cidx]) break;
	} while (++i);
	return cidx;
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
