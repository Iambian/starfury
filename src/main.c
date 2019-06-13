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
#include "gfx/out/gui_gfx.h"

/* Put your function prototypes here */
void setup_palette(void);
void initGridArea(uint8_t grlevel);
void drawGridArea(void);
void drawInventory(void);
void drawPreview(void);
void drawCurSelBar(void);
void drawPowerLimit(void);
void drawRightBar(void);

void drawAndSetupTextBox(int x, uint8_t y, uint8_t w, uint8_t h, uint8_t bgcolor, uint8_t fgcolor);
void setMinimalInventory(void);  //Edits inventory to make all owned blueprints buildable
uint8_t getPrevInvIndex(uint8_t cidx);
uint8_t getNextInvIndex(uint8_t cidx);
void drawSpriteAsText(gfx_sprite_t *sprite);

void keywait(void);
void waitanykey(void);
void error(char *msg);

/* Put all your globals here */
uint8_t *inventory; //256 entry wide, indexed by position. Set from save file l8r
blueprint_obj *curblueprint;

blueprint_obj temp_blueprint;
gridblock_obj *temp_blueprint_grid;

gfx_sprite_t *buildarea;		//Up to 96x96 upscaled by 2 during render.
gfx_sprite_t *tempblock_grid; 	//Up to 32x32 (4x4 block)
gfx_sprite_t *tempblock_inv;    //Up to 32x32 (4x4 block)
gfx_sprite_t *tempblock_scratch;	//Up to 48x48 (4x4 block)
uint8_t gridlevel;        //Allowed values 1,2,3. 0 is ignored
uint8_t curcolor;         //Currently selected color (keep intensity 0)
uint8_t curindex;         //Index of currently selected object
uint8_t cursorx;		//- If these two are greater than 11, assume that	
uint8_t cursory;		//- the cursor is focused on the inventory bar
uint8_t edit_status;    //Bitfield. See enum ESTAT
gridblock_obj selected_object;  //X,Y at -1,-1 if obj is in inventory
uint8_t onhover_offset; //Offset in gridblock_obj arr for on-hover
uint8_t select_offset;  //Offset in gridblock_obj arr for selected object

typedef struct gamedata_struct {
	uint8_t blueprints_owned;  //A bit mask
	uint8_t temp;
	
} gamedata_t;
gamedata_t gamedata;


void main(void) {
	kb_key_t kc,kd;
	uint8_t i;
	
    gfx_Begin(gfx_8bpp);
	gfx_SetDrawBuffer();
	//setup_palette();
	fn_Setup_Palette();
	gfx_SetTransparentColor(TRANSPARENT_COLOR);
	gfx_SetTextTransparentColor(TRANSPARENT_COLOR);
	gfx_SetTextBGColor(TRANSPARENT_COLOR);
	/* Load save file */
	
	/* Initialize game defaults */
	cursorx = cursory = -1;
	curcolor = DEFAULT_COLOR;
	buildarea = NULL; /*Unsure if this is BSS. Added to ensure value if not */
	tempblock_grid = gfx_MallocSprite(TEMPBLOCK_MAX_W,TEMPBLOCK_MAX_H);
	tempblock_inv  = gfx_MallocSprite(TEMPBLOCK_MAX_W,TEMPBLOCK_MAX_H);
	tempblock_scratch = gfx_MallocSprite(PREVIEWBLOCK_MAX_W,PREVIEWBLOCK_MAX_H);
	//These temps are used during editing. Never edit a built-in.
	temp_blueprint_grid = malloc((sizeof basic_blueprint.blocks[0])*256);
	temp_blueprint.blocks = temp_blueprint_grid;
	gamedata.blueprints_owned = BP_BASIC;
	
	
	/* INITIALIZE DEBUG LOGIC */
	inventory = malloc(256);
	curblueprint = &basic_blueprint;
	for (i=2;i<20;i++) inventory[i] = 5;  //skip cmd unit 1 for testing
	setMinimalInventory();
	initGridArea(3);  //max size. Sets gridlevel.
	curindex = getNextInvIndex(getPrevInvIndex(0));
	
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
		drawPreview();
		//status bar
		gfx_SetColor(COLOR_GRAY|COLOR_LIGHTER);
		gfx_FillRectangle_NoClip(0,240-12,320,12);
		//cursel block stats
		drawCurSelBar();
		//ship grid build area
		drawGridArea();
//		gfx_SetColor(COLOR_WHITE|COLOR_DARKER);
//		gfx_FillRectangle_NoClip(64+6,24,192,192);
		//Limit field
		drawPowerLimit();
		//color and stats field
		drawRightBar();
		
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
		for (i=0;i<curblueprint->numblocks;i++) {
			orientation = curblueprint->blocks[i].orientation;
			spriteid = curblueprint->blocks[i].block_id;
			srcsprite = blockobject_list[spriteid].sprite;
			tempblock_scratch->width = srcsprite->width;
			tempblock_scratch->height = srcsprite->height;
			switch (orientation&3) {
				case ROT_0:
					memcpy(tempblock_scratch,srcsprite,srcsprite->width*srcsprite->height+2);
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
				gfx_FlipSpriteY(tempblock_scratch,tempblock_grid);
			} else {
				memcpy(tempblock_grid,tempblock_scratch,tempblock_scratch->width*tempblock_scratch->height+2);
			}
			gridx = curblueprint->blocks[i].x+xoff;
			gridy = curblueprint->blocks[i].y+yoff;
			fn_PaintSprite(tempblock_grid,curblueprint->blocks[i].color);
			fn_DrawNestedSprite(tempblock_grid,buildarea,8*gridx,8*gridy);
		}
	}
	gfx_ScaledTransparentSprite_NoClip(buildarea,sx,sy,2,2);
}

//No prototype. used by inv renderer
void drawInvCount(int nx, uint8_t ny, uint8_t invidx) {
	uint8_t count,i;
	gfx_SetTextFGColor(COLOR_MAROON);
	gfx_SetTextXY(nx+16+8,ny+4);
	for (i=count=0;i<curblueprint->numblocks;++i) {
		if (curblueprint->blocks[i].block_id == invidx) ++count;
	}
	gfx_PrintUInt(count,3);
	gfx_SetTextFGColor(COLOR_BLACK);
	gfx_SetTextXY(nx+16+8,ny+4+10);
	gfx_PrintUInt(inventory[invidx],3);
}


//No prototype. pos=[0-3], invidx = index to inventory slot
void drawSmallInvBox(uint8_t pos,uint8_t invidx) {
	uint8_t i,nw,nh,nx,ny,count;
	unsigned int temp;
	static uint8_t smallinv_y_lut[] = {1+6,1+6+28,58+48+1+6,58+48+1+28+6};
	blockprop_obj *blockinfo;
	gfx_sprite_t *srcsprite;
	/* Setup draw area */
	tempblock_inv->width = tempblock_inv->height = 16;
	fn_FillSprite(tempblock_inv,TRANSPARENT_COLOR);
	/* Grab properties of block in question */
	blockinfo = &blockobject_list[invidx];
	//dbg_sprintf(dbgout,"block object %i, address %x\n",invidx,blockinfo->sprite);
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
	drawInvCount(nx,ny,invidx);
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
	fn_FillSprite(tempblock_inv,TRANSPARENT_COLOR);
	nx = (32-tempblock_scratch->width)/2;
	ny = (32-tempblock_scratch->height)/2;
	fn_DrawNestedSprite(tempblock_scratch,tempblock_inv,nx,ny);
	//memcpy(tempblock_inv,tempblock_scratch,tempblock_scratch->width*tempblock_scratch->height+2);
	fn_PaintSprite(tempblock_inv,COLOR_GREEN);
	gfx_TransparentSprite_NoClip(tempblock_inv,2,(58+8+4));
	//Print other stats according to the diagram
	drawInvCount(16-6+2,58+8+6,curindex);
	gfx_SetTextFGColor(COLOR_BLACK);
	gfx_PrintStringXY(blockobject_list[curindex].name,1,60);
	
	
	return;
}

void drawPreview(void) {
	uint8_t ny;
	int nx;
	/* buildarea is at most 96x96. Must shrink by half and render into 64x64 area */
	gfx_SetColor(COLOR_BLUE|COLOR_DARKER);
	gfx_Rectangle_NoClip(0,164,64,64-12);
	gfx_SetColor(COLOR_BLACK);
	gfx_FillRectangle_NoClip((0+1),(164+1),(64-2),(64-12-2));
	if ((buildarea->width>96) || !buildarea->width) return;
	if ((buildarea->height>96) || !buildarea->height) return;
	//+8 if 96w, +12 if 80w, +16 if 64w. Do: (128-w)/4
	nx = (128-buildarea->width)/4+0;
	ny = (((64-12)*2)-buildarea->height)/4+164;
	gfx_RotatedScaledTransparentSprite_NoClip(buildarea,nx,ny,0,32);
}



void drawCurSelBar(void) {
	uint8_t i,blocktype,y,w,cofg,cobg;
	int x;
	char *s;
	gfx_sprite_t *spr;
	gridblock_obj *tgbo;
	blockprop_obj *tbpo;
	
	blocktype = 0;
	if (edit_status&EDIT_SELECT) {
		//Show status of the object currently in selection
		blocktype = selected_object.block_id;
	} else {
		//or show status of item under the cursor
		if ((cursorx|cursory)&(1<<7)) {
			//If the cursor is over the inventory
			blocktype = curindex;
		} else {
			//or if the cursor is over the grid
			tgbo = curblueprint->blocks;
			for (i=0;i<curblueprint->numblocks;i++) {
				if (cursorx>=tgbo[i].x && cursorx<tgbo[i].x+blockobject_list[tgbo[i].block_id].w) {
					if (cursory>=tgbo[i].y && cursorx<tgbo[i].y+blockobject_list[tgbo[i].block_id].h) {
						blocktype = tgbo[i].block_id;
						break;
					}
				}
			}
		}
	}
	//Move the following inside of if condition once render is verified correct
	gfx_SetColor(COLOR_DARKGRAY|COLOR_DARKER);
	if (!blocktype) {
		gfx_FillRectangle_NoClip(0,(164+64-12),(320-64),12);
	} else {
		tbpo = &blockobject_list[blocktype];
		gfx_FillRectangle_NoClip(0,(164+64-12),(64+14+8),12);
		//Setup x,y,w and then draw blocktype
		x = 64+7+7+8; y = 164+64-12; w = 36;
		gfx_SetTextFGColor(COLOR_WHITE|COLOR_LIGHTER);
		gfx_PrintStringXY(tbpo->name,2,(y+2));
		drawAndSetupTextBox(x,y,w,12,COLOR_GRAY,COLOR_WHITE|COLOR_LIGHTER);
		x += w;
		s = "";
		switch (tbpo->type) {
			case FILLER:  s="FILL"; break;
			case COMMAND: s="COMM"; break;
			case ENGINE:  s="ENGN"; break;
			case WEAPON:  s="WEPN"; break;
			case SPECIAL: s="SPCL"; break;
			case ENERGYSOURCE: s="PSRC"; break;
			default: break;
		}
		gfx_PrintString(s);
		//Draw WT XXX (or PW XXX if a command module)
		w = (46-8);
		if (tbpo->type == COMMAND) {
			spr = (gfx_sprite_t*)spr_pwr_data; cofg = COLOR_YELLOW; cobg = COLOR_BLUE;
		} else {
			spr = (gfx_sprite_t*)spr_wt_data; cofg = COLOR_YELLOW; cobg = COLOR_BLACK;
		}
		drawAndSetupTextBox(x,y,w,12,cofg,cobg);
		x += w;
		drawSpriteAsText(spr);
		gfx_PrintUInt(tbpo->cost,3);
		/*Draw HP XXX*/
		w = (46-8);
		drawAndSetupTextBox(x,y,w,12,COLOR_GREEN,COLOR_BLACK);
		x += w;
		drawSpriteAsText((gfx_sprite_t*)spr_hp_data);
		gfx_PrintUInt(tbpo->hp,3);
		/*Draw AT XX*/
		w = (38-8);
		drawAndSetupTextBox(x,y,w,12,COLOR_WHITE,COLOR_BLACK);
		x += w;
		drawSpriteAsText((gfx_sprite_t*)spr_atk_data);
		gfx_PrintUInt(tbpo->atk,2);
		/*Draw DF XX*/
		w = (38-8);
		drawAndSetupTextBox(x,y,w,12,COLOR_BLACK,COLOR_WHITE|COLOR_LIGHTER);
		x += w;
		drawSpriteAsText((gfx_sprite_t*)spr_def_data);
		gfx_PrintUInt(tbpo->def,2);
		/*Draw SP XX*/
		w = (38-8);
		drawAndSetupTextBox(x,y,w,12,COLOR_RED,COLOR_WHITE|COLOR_LIGHTER);
		x += w;
		drawSpriteAsText((gfx_sprite_t*)spr_spd_data);
		gfx_PrintUInt(tbpo->spd,2);
		/*Draw AG XX*/
		w = (38-8);
		drawAndSetupTextBox(x,y,w,12,COLOR_BLUE,COLOR_WHITE|COLOR_LIGHTER);
		x += w;
		drawSpriteAsText((gfx_sprite_t*)spr_agl_data);
		gfx_PrintUInt(tbpo->agi,2);
	}
}

void drawPowerLimit(void) {
	uint8_t i,y,w;
	int x,energy_used,energy_total;
	gridblock_obj *tgbo;
	blockprop_obj *tbpo;
	
	drawAndSetupTextBox((320-64),0,64,24,COLOR_YELLOW,COLOR_BLUE);
	gfx_PrintString(" PWR  LIM");
	
	gfx_SetColor(COLOR_GREEN|COLOR_DARKER);
	gfx_FillRectangle_NoClip((320-63),13,62,10);
	
	tgbo = curblueprint->blocks;
	energy_used = energy_total = 0;
	for (i=0;i<curblueprint->numblocks;i++) {
		tbpo = &blockobject_list[tgbo[i].block_id];
		if (tbpo->type&(COMMAND|ENERGYSOURCE)) {
			energy_total += tbpo->cost;
		} else {
			energy_used += tbpo->cost;
		}
	}
	if (energy_used>energy_total) w=60;
	else w = (60*energy_used)/energy_total;
	gfx_SetColor(COLOR_LIME);
	gfx_FillRectangle_NoClip((320-62),14,w,8);
	gfx_SetTextFGColor(COLOR_BLACK);
	gfx_SetTextXY((320-60),14);
	gfx_PrintUInt(energy_used,3);
	gfx_PrintChar('/');
	gfx_PrintUInt(energy_total,3);
}

void drawRightBar(void) {
	uint8_t i,gridx,gridy,y,y2;
	int total_hp,total_atk,total_def,total_spd,total_agi;
	int x;
	gridblock_obj *tgbo;
	blockprop_obj *tbpo;
	
	gfx_SetColor(COLOR_GRAY|COLOR_DARKER);
	gfx_FillRectangle_NoClip((64+6+192+6),24,48,192);
	if ((edit_status&(COLOR_SELECT|PAINTER_MODE))) {
		//Generate color palette
		for (i=gridy=0;gridy<16;++gridy) {
			for (gridx=0;gridx<4;++gridx,++i) {
				gfx_SetColor(i);
				gfx_FillRectangle_NoClip((64+6+192+6+1)+(12*gridx),(24+1)+(12*gridy),10,10);
			}
		}
	} else {
		//Show ship's total status
		total_hp=total_atk=total_def=total_spd=total_agi=0;
		tgbo = curblueprint->blocks;
		for (i=0;i<curblueprint->numblocks;i++) {
			tbpo = &blockobject_list[tgbo[i].block_id];
			total_hp  += tbpo->hp;
			total_atk += tbpo->atk;
			total_def  += tbpo->def;
			total_spd  += tbpo->spd;
			total_agi  += tbpo->agi;
		}
		x = (64+6+192+6-2); y = 24+41; y2 = y+12;
		
		//Print HP
		drawAndSetupTextBox(x,y,52,22,COLOR_GREEN,COLOR_BLACK);
		gfx_PrintString("HT  PTS");
		gfx_SetTextXY(x+10,y2);
		gfx_PrintUInt(total_hp,4);
		y += 22; y2 += 22;
		//Print ATTACK
		drawAndSetupTextBox(x,y,52,22,COLOR_WHITE,COLOR_BLACK);
		gfx_PrintString("ATTACK");
		gfx_SetTextXY(x+14,y2);
		gfx_PrintUInt(total_atk,3);
		y += 22; y2 += 22;
		//Print ARMOR
		drawAndSetupTextBox(x,y,52,22,COLOR_BLACK,COLOR_WHITE);
		gfx_PrintString("ARMOR ");
		gfx_SetTextXY(x+14,y2);
		gfx_PrintUInt(total_def,3);
		y += 22; y2 += 22;
		//Print SPEED
		drawAndSetupTextBox(x,y,52,22,COLOR_RED,COLOR_WHITE);
		gfx_PrintString("SPEED");
		gfx_SetTextXY(x+14,y2);
		gfx_PrintUInt(total_spd,3);
		y += 22; y2 += 22;
		//Print AGILITY
		drawAndSetupTextBox(x,y,52,22,COLOR_BLUE,COLOR_WHITE);
		gfx_PrintString("AGILTY");
		gfx_SetTextXY(x+14,y2);
		gfx_PrintUInt(total_agi,3);
		//y += 22; y2 += 22;
	}
}




void setMinimalInventory(void) {
	uint8_t i;
	uint8_t cblock,blockcount;
	uint8_t k;
	blueprint_obj *tbp;
	gridblock_obj *tgbo;
	//Iterate over all possible built-in blueprints that may be owned
	for (i=0;i<8;i++) {
		if ((gamedata.blueprints_owned>>i)&1) {
			tbp = builtin_blueprints[i];
			tgbo = tbp->blocks;
			//Iterate over the entire inventory
			for (cblock=1;cblock<255;cblock++) {
				blockcount = 0;
				//Iterate over the blueprint grid to get number of cblock in it.
				for (k=0;k<(tbp->numblocks);k++) {
					if (tgbo[k].block_id == cblock) blockcount++;
				}
				//if (blockcount) dbg_sprintf(dbgout,"Block %i, numbering %i\n",cblock,blockcount);
				//Then set inventory to what we found if there wasn't enough.
				if (inventory[cblock]<blockcount) inventory[cblock]=blockcount;
			}
		}
	}
}

void drawAndSetupTextBox(int x, uint8_t y, uint8_t w, uint8_t h, uint8_t bgcolor, uint8_t fgcolor) {
	uint8_t i,border_color_outer,border_color_inner,fill_color;
	if ((fgcolor&COLOR_DARKER) == COLOR_WHITE) {
		border_color_outer = bgcolor|COLOR_LIGHTER;
		border_color_inner = bgcolor|COLOR_LIGHT;
		fill_color = bgcolor|COLOR_DARKER;
	} else {
		border_color_outer = bgcolor|COLOR_DARKER;
		border_color_inner = bgcolor|COLOR_DARK;
		fill_color = bgcolor|COLOR_LIGHTER;
	}
	gfx_SetColor(border_color_outer);
	gfx_Rectangle_NoClip(x,y,w,h);
	++x; ++y; w-=2; h-=2;
	gfx_SetColor(border_color_inner);
	gfx_Rectangle_NoClip(x,y,w,h);
	++x; ++y; w-=2; h-=2;
	gfx_SetColor(fill_color);
	gfx_FillRectangle_NoClip(x,y,w,h);
	gfx_SetTextFGColor(fgcolor);
	gfx_SetTextXY(x,y);
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

void drawSpriteAsText(gfx_sprite_t *sprite) {
	uint8_t y;
	int x;
	x = gfx_GetTextX();
	y = gfx_GetTextY();
	gfx_TransparentSprite_NoClip(sprite,x,y);
	gfx_SetTextXY(x+sprite->width+2,y);
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
