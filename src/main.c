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
void drawStatusBar(void);

void drawAndSetupTextBox(int x, uint8_t y, uint8_t w, uint8_t h, uint8_t bgcolor, uint8_t fgcolor);
void setMinimalInventory(void);  //Edits inventory to make all owned blueprints buildable
uint8_t getPrevInvIndex(uint8_t cidx);
uint8_t getNextInvIndex(uint8_t cidx);
void drawSpriteAsText(gfx_sprite_t *sprite);
void modifyCursorPos(uint8_t *xpos, uint8_t *ypos, kb_key_t dir, uint8_t xlim, uint8_t ylim);
void updateGridArea(void);
uint8_t checkGridCollision(uint8_t xpos, uint8_t ypos);   //Returns block offset in curblueprint
void prepareGridItem(gridblock_obj *gbo);
void adjustCursorWithOrientation(void);


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
uint8_t paintcursorx;
uint8_t paintcursory;
uint8_t edit_status;    //Bitfield. See enum ESTAT
gridblock_obj selected_object;  //X,Y at -1,-1 if obj is in inventory
uint8_t onhover_offset; //Offset in gridblock_obj arr for on-hover
uint8_t select_offset;  //Offset in gridblock_obj arr for selected object

typedef struct gamedata_struct {
	uint8_t blueprints_owned;  //A bit mask
	uint8_t default_color;
	uint8_t temp;
	
} gamedata_t;
gamedata_t gamedata;


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
	gamedata.default_color = COLOR_GREEN;
	
	
	/* INITIALIZE DEBUG LOGIC */
	inventory = malloc(256);
	curblueprint = &basic_blueprint;
	for (i=2;i<20;i++) inventory[i] = 5;  //skip cmd unit 1 for testing
	setMinimalInventory();
	initGridArea(3);  //max size. Sets gridlevel.
	curindex = getNextInvIndex(getPrevInvIndex(0));
	update_flags = 0xFF;  //Update all initial frame
	updateGridArea();

	
	/* Start game */
	while (1) {
		kb_Scan();
		kd = kb_Data[7];
		kc = kb_Data[1];
		
		limit = (gridlevel<<1)+6;  //8,10,12. Place somewhere static. Gridlevel won't change on us
		/* Handle top row buttons */
		if (kc&kb_Graph) {
			if (edit_status&FILE_SELECT) {
				//You pushed QUIT. What do?
			} else {
				if (!(edit_status&EDIT_SELECT)) {
					//Do not allow switching to color if holding a block
					edit_status ^= COLOR_SELECT;
					update_flags |= (PAN_RIGHT|PAN_GRID|PAN_STATUS);
				}
			}
		}
		if (kc&kb_Trace) {
			if (edit_status&FILE_SELECT) {
				//Nothing in this file menu slot. Yet?
			} else {
				if (edit_status&EDIT_SELECT) {
					//Assumes move mode. Rotate CW
					t = selected_object.orientation;
					selected_object.orientation = ((t+1)&3)|(t&0xFC);
					adjustCursorWithOrientation();
				} else if (edit_status&COLOR_SELECT) {
					//Color LOCK
				}
			}
		}
		
		if (kc&kb_Zoom) {
			if (edit_status&FILE_SELECT) {
				//File: RELOAD. Copy from file to buffer.
			} else {
				if (edit_status&EDIT_SELECT) {
					//Assumes move mode. Flip
					selected_object.orientation ^= HFLIP;
				} else if (edit_status&COLOR_SELECT) {
					//Color all with selected color
					for (i=0;i<curblueprint->numblocks;i++) {
						curblueprint->blocks[i].color = curcolor;
					}
				}
			}
		}
		
		if (kc&kb_Window) {
			if (edit_status&FILE_SELECT) {
				//File: SAVE. Write current buffer out to file.
			} else {
				if (edit_status&EDIT_SELECT) {
					//Assumes move mode. Rotate CCW
					t = selected_object.orientation;
					selected_object.orientation = ((t-1)&3)|(t&0xFC);
					adjustCursorWithOrientation();
				} else if (edit_status&COLOR_SELECT) {
					//Change to grid with color select mode. edit_status: PICKING_COLOR
				}
			}
		}
		
		if (kc&kb_Yequ) {
			edit_status ^= FILE_SELECT;
			update_flags |= PAN_STATUS;
		}
		
		if (kc&(kb_Graph|kb_Trace|kb_Zoom|kb_Window|kb_Yequ)) update_flags |= PAN_GRID;
		
		/* Handle command buttons */
		if (kb_Data[6]&kb_Clear) { keywait(); break; } /* DEBUG: INSTANT QUIT */
		if (kc&kb_2nd) {
			if (edit_status&FILE_SELECT) {
				//Check if there's any blocking menus. Otherwise do nothing.
				
			} else if (edit_status&COLOR_SELECT) {
				//Running paint mode
				if (edit_status&NOW_PAINTING) {
					//paint a block
					t = checkGridCollision(cursorx,cursory);
					if (t != 0xFF) {
						curblueprint->blocks[t].color = curcolor;
						updateGridArea();
						update_flags |= PAN_GRID;
					}
				} else if (edit_status&PICKING_COLOR) {
					//Pick a color, then switch to coloring mode
					t = checkGridCollision(cursorx,cursory);
					if (t != 0xFF) {
						curcolor = curblueprint->blocks[t].color;
						edit_status &= ~PICKING_COLOR;
						update_flags |= 0XFF;  //Update everything
					}
				} else {
					//select color and open grid cursor movement
					edit_status |= NOW_PAINTING;
					update_flags |= PAN_GRID;
					if ((cursorx|cursory)&0x80) {
						cursorx ^= 0xFF;
						if (cursorx>=limit) cursorx = 0;
						if (cursory&0x80) cursory = 0;
					}
				}
			} else {
				//Running edit mode
				if ((cursorx|cursory)>127) {
					//Bringing a piece out of inventory. If a piece is already
					//being held, no worries. it can be safely overwritten
					tt = t = 0;
					for (i=0;i<curblueprint->numblocks;i++) {
						if (curblueprint->blocks[i].block_id == curindex) t++;
						tt |= blockobject_list[curblueprint->blocks[i].block_id].type;
					}
					// Begin placement if enough items left AND
					// we're not placing more than one command module
					if ((t<inventory[curindex]) && !((tt&blockobject_list[curindex].type&COMMAND))) {
						selected_object.block_id = curindex;
						selected_object.x = selected_object.y = -1;
						selected_object.color = curcolor;
						selected_object.orientation = ROT_0;
						edit_status |= EDIT_SELECT;
						if (cursory>12) cursory=0;
						cursorx ^= 0xFF;
						if (cursorx>=limit) cursorx = 0;
						adjustCursorWithOrientation();
						update_flags |= (PAN_GRID|PAN_CURSEL|PAN_STATUS);
					}
				} else {
					//Picking up or plopping a piece out on the grid
						
						
						
					update_flags |= (PAN_GRID|PAN_CURSEL);
				}
			}
		}
		
		
		
		if (kc&kb_Mode) {
			if (edit_status&FILE_SELECT) {
				if ((edit_status&(OTHER_CONFIRM|QUIT_CONFIRM))) {
					//Go back from blocking
					
				} else {
					//Turn off FILE_SELECT mode
					edit_status &= ~FILE_SELECT;
				}
			} else {
				//Not doing a file operation. Check other states
				if (edit_status&EDIT_SELECT) {
					//If holding a block, put it away
					if (selected_object.block_id) selected_object.block_id = 0;
					edit_status &= ~EDIT_SELECT;
				} else if (edit_status&COLOR_SELECT) {
					if (edit_status&(NOW_PAINTING|PICKING_COLOR)) {
						cursorx ^= 0xFF;
						edit_status &= ~(NOW_PAINTING|PICKING_COLOR);
					}
				} else cursorx ^= 0xFF;
			}
			update_flags |= (PAN_GRID|PAN_STATUS);
		}
		
		/* Handle directional buttons */
		if (edit_status&COLOR_SELECT && !(edit_status&NOW_PAINTING)) {
			//Selecting color palette. Bitmask shenanigans follows.
			if (kd&kb_Down)  curcolor = (curcolor+4)&0x3F;
			if (kd&kb_Up)    curcolor = (curcolor-4)&0x3F;
			if (kd&kb_Right) curcolor = ((curcolor+1)&0x03)+(curcolor&0x3C);
			if (kd&kb_Left)  curcolor = ((curcolor-1)&0x03)+(curcolor&0x3C);
			update_flags |= (PAN_INV|PAN_RIGHT|PAN_STATUS);
		} else {
			//Moving around the cursor
			if ((cursorx|cursory)>127) {
				//If cursor is in the inventory box
				if (kd&kb_Down) curindex = getNextInvIndex(curindex);
				if (kd&kb_Up)   curindex = getPrevInvIndex(curindex);
				if (kd&(kb_Up|kb_Down)) update_flags |= (PAN_INV|PAN_CURSEL);
				if (kd&kb_Right) {
					cursorx ^= 0xFF;
					if (cursorx>=limit) cursorx = 0;
					if (cursory>127) cursory=0;
					update_flags |= (PAN_GRID|PAN_CURSEL);
				}
			} else {
				//Cursor on the grid
				if (kd) {
					if (edit_status&EDIT_SELECT) {
						tx = blockobject_list[selected_object.block_id].w-1;
						ty = blockobject_list[selected_object.block_id].h-1;
						//If C or CC once rotate, flip tx and ty
						if (selected_object.orientation&1) {
							t = tx;
							tx = ty;
							ty = t;
						}
					} else tx = ty = 0;
					//Use tx and ty to restrict large blocks to grid area
					modifyCursorPos(&cursorx,&cursory,kd,(6+gridlevel*2)-tx,(6+gridlevel*2)-ty);
					update_flags |= (PAN_GRID|PAN_CURSEL);
				}
			}
		}
		
		//maintenance
		if (update_flags&PAN_FULLSCREEN) {
			update_flags &= ~PAN_FULLSCREEN;  //Clear fullscreen bit
			update_flags |= ~PAN_FULLSCREEN;  //Set all others because dirty.
			gfx_FillScreen(COLOR_BLACK);
			/* TODO: No other flags available. Render ship name here */
		}
		//inventory bar
		if (update_flags&PAN_INV) {
			update_flags &= ~PAN_INV;
			drawInventory();
		}
		//ship grid build area
		if (update_flags&PAN_GRID) {
			update_flags &= ~PAN_GRID;
			drawGridArea();
		}
		//preview
		if (update_flags&PAN_PREVIEW) {
			update_flags &= ~PAN_PREVIEW;
			drawPreview();
		}
		//status bar
		if (update_flags&PAN_STATUS) {
			update_flags &= ~PAN_STATUS;
			drawStatusBar();
		}
		//cursel block stats
		if (update_flags&PAN_CURSEL) {
			update_flags &= ~PAN_CURSEL;
			drawCurSelBar();
		}
		//Limit field
		if (update_flags&PAN_LIM) {
			update_flags &= ~PAN_LIM;
			drawPowerLimit();
		}
		//color and stats field
		if (update_flags&PAN_RIGHT) {
			update_flags &= ~PAN_RIGHT;
			drawRightBar();
		}
		
		gfx_BlitBuffer();
		
		/* Debounce */
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
	int sx,tx,w,h;
	uint8_t offset,sy,ty,cols,gridy,gridx,limit,t;
	uint8_t xoff,yoff,i,orientation,spriteid;
	gfx_sprite_t *srcsprite;
	
	offset = 32-((gridlevel-1)<<4);
	sx = 64+6+offset;
	sy = 24+offset;
	limit = (gridlevel<<1)+6;  //8,10,12
//	if (edit_status&EDIT_SELECT) {
		gfx_SetColor(COLOR_BLACK);
		gfx_FillRectangle_NoClip(sx,sy,16*limit,16*limit);
//	}
	gfx_SetColor(COLOR_GRAY|COLOR_LIGHTER);
	gfx_Rectangle_NoClip(sx-1,sy-1,limit*16+2,limit*16+2);
	for (gridy=0;gridy<limit;gridy++) {
		for (gridx=0;gridx<limit;gridx++) {
			gfx_Rectangle_NoClip(sx+(gridx*16),sy+(gridy*16),16,16);
		}
	}
	
	gfx_ScaledTransparentSprite_NoClip(buildarea,sx,sy,2,2);
	if ((cursorx|cursory)<128) {
		if (edit_status&EDIT_SELECT) {
			//Prepare and draw sprite
			prepareGridItem(&selected_object);
			
			/* TODO: Change gfx_TransparentSprite to whatever will draw 2x scale stuff */
			gfx_ScaledTransparentSprite_NoClip(tempblock_grid,sx+(cursorx*16),sy+(cursory*16),2,2);
			//Set cursor width and height
			w = blockobject_list[selected_object.block_id].w*16;
			h = blockobject_list[selected_object.block_id].h*16;
			if (selected_object.orientation&1) {
				t = w;
				w = h;
				h = t;
			}
		} else {
			//Set cursor width and height
			w = 16; h=16;
		}
		gfx_SetColor(COLOR_MAGENTA|COLOR_LIGHTER);
		gfx_Rectangle_NoClip(sx+(cursorx*16),sy+(cursory*16),w,h);
		gfx_Rectangle_NoClip(sx+(cursorx*16)-1,sy+(cursory*16)-1,w+2,h+2);
	}
	
	
	
	
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


/*	No prototype. Bundle with drawInventory()
	pos=[0-3], invidx = index to inventory slot */
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
	char *s;
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
	fn_PaintSprite(tempblock_inv,curcolor);
	gfx_TransparentSprite_NoClip(tempblock_inv,2,(58+8+4));
	//Print other stats according to the diagram
	drawInvCount(16-6+2,58+8+6,curindex);
	gfx_SetTextFGColor(COLOR_BLACK);
	s = blockobject_list[curindex].name;
	gfx_SetTextXY(1,60);
	for (i=0;s[i]&&i<9;i++) gfx_PrintChar(s[i]);  //Instead of flipping clipping bits around
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
	uint8_t i,blocktype,y,w,cofg,cobg,t;
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
			if (0xFF == (t = checkGridCollision(cursorx,cursory))) {
				blocktype = 0;
			} else {
				blocktype = curblueprint->blocks[t].block_id;
			}
		}
	}
	//Move the following inside of if condition once render is verified correct
	gfx_SetColor(COLOR_DARKGRAY|COLOR_DARKER);
	if (!blocktype) {
		gfx_FillRectangle_NoClip(0,(164+64-12),320,12);
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
	if (energy_used>energy_total) {
		w=60;
		gfx_SetColor(COLOR_RED);
	} else {
		w = (60*energy_used)/energy_total;
		gfx_SetColor(COLOR_LIME);
	}
	gfx_FillRectangle_NoClip((320-62),14,w,8);
	gfx_SetTextFGColor(COLOR_BLACK);
	gfx_SetTextXY((320-60),14);
	gfx_PrintUInt(energy_used,3);
	gfx_PrintChar('/');
	gfx_PrintUInt(energy_total,3);
}

/* No prototype. Bundle with drawRightBar */
void rightBarStats(char *s,uint8_t *y, uint8_t fgcol, uint8_t bgcol, int value) {
	int x = (64+6+192+6-2);
	drawAndSetupTextBox(x,*y,54,22,fgcol,bgcol);
	gfx_PrintString(s);
	gfx_SetTextXY(x+14,12+*y);
	gfx_PrintUInt(value,3);
	*y = 22+*y;
}

void drawRightBar(void) {
	uint8_t i,gridx,gridy,y;
	int total_hp,total_atk,total_def,total_spd,total_agi;
	int x;
	gridblock_obj *tgbo;
	blockprop_obj *tbpo;
	
	gfx_SetColor(COLOR_GRAY|COLOR_DARKER);
	gfx_FillRectangle_NoClip((64+6+192+6),24,48,192);
	if ((edit_status&COLOR_SELECT)) {
		//Generate color palette
		x = (64+6+192+6+1);
		y = (24+1);
		for (i=gridy=0;gridy<16;++gridy) {
			for (gridx=0;gridx<4;++gridx,++i) {
				gfx_SetColor(i);
				gfx_FillRectangle_NoClip(x+(12*gridx),y+(12*gridy),10,10);
			}
		}
		x = ((curcolor&3)*12)+x-1;
		y = (((curcolor&0xFC)>>2)*12)+y-1;
		gfx_SetColor(COLOR_WHITE|COLOR_LIGHTER);
		gfx_Rectangle_NoClip(x,y,12,12);
		gfx_Rectangle_NoClip(x+1,y+1,10,10);
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
		y = 24+41;
		rightBarStats("HT  PTS",&y,COLOR_GREEN,COLOR_BLACK,total_hp);
		rightBarStats("ATTACK",&y,COLOR_WHITE,COLOR_BLACK,total_atk);
		rightBarStats("ARMOR",&y,COLOR_BLACK,COLOR_WHITE,total_def);
		rightBarStats("SPEED",&y,COLOR_RED,COLOR_WHITE,total_spd);
		rightBarStats("AGILTY",&y,COLOR_BLUE,COLOR_WHITE,total_agi);
	}
}

/*	No prototype. Bundle with drawStatusBar() 
	pos: [0-4]; status: 0=normal, 1=selected, 2=unselectable */
void drawOption(uint8_t pos,char *s,uint8_t status) {
	int x,w;
	uint8_t y;
	if (status&2) 	gfx_SetTextFGColor(COLOR_DARKGRAY|COLOR_LIGHTER);
	else			gfx_SetTextFGColor(COLOR_BLACK);
	x = 64*pos;
	y = (240-12);
	if (status&1) {
		gfx_SetColor(COLOR_MAROON|COLOR_LIGHTER);
		gfx_FillRectangle_NoClip(x+1,y+1,62,10);
	}
	gfx_PrintStringXY(s,x+2+(60-gfx_GetStringWidth(s))/2,y+2);
	
}

void drawStatusBar(void) {
	uint8_t i,y;
	int x;
	gfx_SetColor(COLOR_GRAY|COLOR_LIGHTER);
	gfx_FillRectangle_NoClip(x=0,y=(240-12),320,12);
	gfx_SetColor(COLOR_DARKGRAY);
	for (i=0;i<5;i++,x+=64) {
		gfx_Rectangle_NoClip(x,y,64,12);
	}
	drawOption(0,"FILE",!!(edit_status&FILE_SELECT));
	/* File select overrides all other flags */
	if (edit_status&FILE_SELECT) {
		drawOption(1,"SAVE",0);
		drawOption(2,"RELOAD",0);
		drawOption(4,"QUIT",!!(edit_status&QUIT_CONFIRM));
	} else {
		if (edit_status&COLOR_SELECT) {
			//Handle color edit display
			drawOption(4,"PAINTER",1);
			drawOption(3,"LOCK",!!(curcolor==gamedata.default_color));
			drawOption(2,"COLR ALL",0);
			drawOption(1,"GET COLR",!!(edit_status&PICKING_COLOR));
		} else {
			//Handle move mode. Note: EDIT_SELECT used only if holding a block
			drawOption(4,"EDITOR",1);
			if (edit_status&EDIT_SELECT) {
				drawOption(3,"ROT CW",0);
				drawOption(2,"FLIP",0);
				drawOption(1,"ROT CCW",0);
			}
		}
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

void updateGridArea(void) {
	uint8_t i,offset,orientation,blockid,w,h,gx,gy;
	blueprint_obj *blueprint;
	gfx_sprite_t *srcsprite;
	
	blueprint = curblueprint;
	fn_FillSprite(buildarea,TRANSPARENT_COLOR);
	if (!blueprint) return;
	offset = gridlevel-blueprint->gridlevel;
	for (i=0;i<blueprint->numblocks;i++) {
		prepareGridItem(&blueprint->blocks[i]);
		fn_DrawNestedSprite(tempblock_grid,buildarea,(offset+blueprint->blocks[i].x)<<3,(offset+blueprint->blocks[i].y)<<3);
	}
}

void modifyCursorPos(uint8_t *xpos, uint8_t *ypos, kb_key_t dir, uint8_t xlim, uint8_t ylim) {
	if (dir&kb_Down)  *ypos = (*ypos >= ylim-1)? 0 : 1+*ypos;
	if (dir&kb_Up)    *ypos = (--*ypos >= ylim)? ylim-1 : *ypos;
	if (dir&kb_Right) *xpos = (*xpos >= xlim-1)? 0 : 1+*xpos;
	if (dir&kb_Left)  *xpos = (--*xpos >= xlim)? xlim-1 : *xpos;
	
}

/* Returns block offset of first collision in curblueprint. 0xFF if item not found  */
uint8_t checkGridCollision(uint8_t xpos, uint8_t ypos) {
	uint8_t i,offset,limit,w,h,t,x,y,sh,sw;
	gridblock_obj *gridblock;
	blockprop_obj *blockprop;
	
	offset = gridlevel-curblueprint->gridlevel;
	xpos -= offset;
	ypos -= offset;
	limit = 6+(curblueprint->gridlevel*2);
	if (xpos>=limit || ypos>=limit) return 0xFF;  //outside range
	
	if (edit_status&EDIT_SELECT) {
		blockprop = &blockobject_list[selected_object.block_id];
		sw = blockprop->w;
		sh = blockprop->h;
		if (selected_object.orientation&1) {
			t = sw;
			sw = sh;
			sh = t;
		}
	} else {
		sw = sh = 1;
	}
	for (i=0;i<curblueprint->numblocks;i++) {
		gridblock = &curblueprint->blocks[i];
		blockprop = &blockobject_list[gridblock->block_id];
		x = gridblock->x;
		y = gridblock->y;
		w = blockprop->w;
		h = blockprop->h;
		if (gridblock->orientation&1) {
			t = w;
			w = h;
			h = t;
		}
		if (gfx_CheckRectangleHotspot(xpos,ypos,sw,sh,x,y,w,h)) {
			return i;
		}
	}
	return 0xFF;
}

/* Write properly rotated sprite in tempblock_grid  */
void prepareGridItem(gridblock_obj *gbo) {
	uint8_t i,w,h;
	gfx_sprite_t *srcsprite;
	
	srcsprite = blockobject_list[gbo->block_id].sprite;
	
	w = tempblock_scratch->width = srcsprite->width;
	h = tempblock_scratch->height = srcsprite->height;
	switch (gbo->orientation&3) {
		case ROT_0: memcpy(tempblock_scratch,srcsprite,w*h+2); break;
		case ROT_1: gfx_RotateSpriteC(srcsprite,tempblock_scratch); break;
		case ROT_2: gfx_RotateSpriteHalf(srcsprite,tempblock_scratch); break;
		case ROT_3: gfx_RotateSpriteCC(srcsprite,tempblock_scratch); break;
		default: break;
	}
	if (gbo->orientation&HFLIP) gfx_FlipSpriteY(tempblock_scratch,tempblock_grid);
	else                        memcpy(tempblock_grid,tempblock_scratch,h*w+2);
	fn_PaintSprite(tempblock_grid,gbo->color);
}

void adjustCursorWithOrientation(void) {
	uint8_t w,h,t,limit;
	
	limit = (gridlevel<<1)+6;  //8,10,12
	w = blockobject_list[selected_object.block_id].w;
	h = blockobject_list[selected_object.block_id].h;
	if (selected_object.orientation&1) {
		t = w;
		w = h;
		h = t;
	}
	if (cursorx+w > limit) cursorx = limit-w;
	if (cursory+h > limit) cursory = limit-h;
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
