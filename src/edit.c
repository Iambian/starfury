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
#include "main.h"
#include "bdata.h"
#include "util.h"
#include "dataio.h"
#include "edit.h"
#include "menu.h"
#include "gfx/out/gui_gfx.h"

/* Function prototypes goes here */
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



/* Variables that are malloc'd to */
gfx_sprite_t *buildarea;		//Up to 96x96 upscaled by 2 during render.
gfx_sprite_t *tempblock_grid; 	//Up to 32x32 (4x4 block)
gfx_sprite_t *tempblock_inv;    //Up to 32x32 (4x4 block)
// note: tempblock_scratch moved to main.c for more permanent role

/* Globals can go here */
const char *edit_quitconfirm[] = {"Really quit?","No","Yes"};
const char *edit_reloadconfirm[] = {"Discard all changes?","No","Yes"};
const char *edit_savenotice[] = {"Notice","Your blueprint","has been saved!"};

//Note: curblueprint defined in dataio.c. It's also set up there on file load
uint8_t gridlevel;      //Used internally
uint8_t curcolor;       //Currently selected color (keep intensity 0)
uint8_t curindex;       //Index of currently selected object
uint8_t cursorx;		//- If these two are greater than 11, assume that	
uint8_t cursory;		//- the cursor is focused on the inventory bar
uint8_t edit_status;    //Bitfield. See enum ESTAT
gridblock_obj selected_object;  //X,Y at -1,-1 if obj is in inventory



void openEditor(uint8_t cbpidx) {
	kb_key_t kc,kd;
	uint8_t i,update_flags,tx,ty,t,limit;
	uint8_t tt;
	
	//Init
	limit = (gamedata.gridlevel<<1)+6;
	cursorx = cursory = -1;
	curcolor = gamedata.default_color;
	curblueprint = &temp_blueprint;
	update_flags = 0xFF;
	gridlevel = gamedata.gridlevel;
	curindex = getNextInvIndex(0);

	//Malloc for all temporary variables
	tempblock_grid = gfx_MallocSprite(TEMPBLOCK_MAX_W,TEMPBLOCK_MAX_H);
	tempblock_inv  = gfx_MallocSprite(TEMPBLOCK_MAX_W,TEMPBLOCK_MAX_H);
	t = limit<<3;
	if (!(buildarea = gfx_MallocSprite(t,t))) error("Buildarea malloc fail");
	fn_FillSprite(buildarea,TRANSPARENT_COLOR);
	updateGridArea();
	
	//Debugging
	for (i=2;i<25;i++) {
		if (!inventory[i]) inventory[i] = 5;
	}

	while (1) {
		kb_Scan(); kd = kb_Data[7]; kc = kb_Data[1];
		
		/* ==================== HANDLE TOP ROW BUTTONS ==================== */
		if (kc == kb_Graph) {
			if (edit_status&FILE_SELECT) {
				//You pushed QUIT. What do?
				if (2==staticMenu(edit_quitconfirm,3)) break;
				update_flags = 0xFF;
			} else {
				if (!(edit_status&EDIT_SELECT)) {
					//Do not allow switching to color if holding a block
					edit_status ^= COLOR_SELECT;
					update_flags |= (PAN_RIGHT|PAN_STATUS);
				}
			}
		}
		if (kc == kb_Trace) {
			if (edit_status&FILE_SELECT) {
				//Nothing in this file menu slot. Yet?
			} else {
				if (edit_status&EDIT_SELECT) {
					//Assumes move mode. Rotate CW
					t = selected_object.orientation;
					selected_object.orientation = ((t+1)&3)|(t&0xFC);
					adjustCursorWithOrientation();
				} else if (edit_status&COLOR_SELECT) {
					//Set color lock
					gamedata.default_color = curcolor;
					update_flags |= PAN_STATUS;
				}
			}
		}
		if (kc == kb_Zoom) {
			if (edit_status&FILE_SELECT) {
				//File: RELOAD. Copy from file to buffer.
				if (2==staticMenu(edit_reloadconfirm,3)) getShipData(cbpidx+8);
				updateGridArea();
				update_flags = 0xFF;
			} else {
				if (edit_status&EDIT_SELECT) {
					//Assumes move mode. Flip
					selected_object.orientation ^= HFLIP;
				} else if (edit_status&COLOR_SELECT) {
					//Color all with selected color
					for (i=0;i<curblueprint->numblocks;i++) {
						curblueprint->blocks[i].color = curcolor;
					}
					updateGridArea();
					update_flags |= PAN_PREVIEW;
				}
			}
		}
		if (kc == kb_Window) {
			if (edit_status&FILE_SELECT) {
				//File: SAVE. Write current buffer out to file.
				saveBlueprint(cbpidx);
				alert(edit_savenotice,3);
				update_flags = 0xFF;
			} else {
				if (edit_status&EDIT_SELECT) {
					//Assumes move mode. Rotate CCW
					t = selected_object.orientation;
					selected_object.orientation = ((t-1)&3)|(t&0xFC);
					adjustCursorWithOrientation();
				} else if (edit_status&COLOR_SELECT) {
					//Change to grid with color select mode. edit_status: PICKING_COLOR
					if ((cursorx|cursory)&0x80) {
						cursorx ^= 0xFF;
						if (cursorx>=limit) cursorx = 0;
						if (cursory&0x80) cursory = 0;
					}					
					edit_status ^= PICKING_COLOR;
					update_flags |= PAN_STATUS;
				}
			}
		}
		if (kc&kb_Yequ) {
			edit_status ^= FILE_SELECT;
			update_flags |= PAN_STATUS;
		}
		if (kc&(kb_Graph|kb_Trace|kb_Zoom|kb_Window|kb_Yequ)) update_flags |= PAN_GRID;

		/* ==================== HANDLE COMMAND BUTTONS ==================== */
		if (kb_Data[6]&kb_Clear) {  /* DEBUG: INSTANT QUIT */
			keywait();
			break;
		}
		
		if (kc == kb_Del) {
			if ((cursorx|cursory<128) && ((t=checkGridCollision(cursorx,cursory))!=0xFF)) {
				if (edit_status&NOW_PAINTING) {
					curblueprint->blocks[t].color = gamedata.default_color;
					updateGridArea();
					update_flags |= (PAN_GRID|PAN_INV);
				} else {
					//Remove block under the cursor
					curblueprint->blocks[t].block_id = 0;
					update_flags |= (PAN_GRID|PAN_LIM|PAN_INV|PAN_RIGHT);
					updateGridArea();
				}
			}
		}
		if (kc == kb_2nd) {
			update_flags |= PAN_RIGHT;
			if (edit_status&FILE_SELECT) {
				//Check if there's any blocking menus. Otherwise do nothing.
				
			} else if (edit_status&COLOR_SELECT) {
				//Running paint mode
				if ((edit_status&NOW_PAINTING)&&!(edit_status&PICKING_COLOR)) {
					//paint a block
					t = checkGridCollision(cursorx,cursory);
					if (t != 0xFF) {
						curblueprint->blocks[t].color = curcolor;
						updateGridArea();
						update_flags |= (PAN_PREVIEW|PAN_GRID);
						updateGridArea();
					}
				} else if (edit_status&PICKING_COLOR) {
					//Pick a color, then switch to coloring mode
					t = checkGridCollision(cursorx,cursory);
					if (t != 0xFF) {
						curcolor = curblueprint->blocks[t].color;
						//edit_status &= ~PICKING_COLOR;
						update_flags |= 0xFF;  //Update everything
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
						update_flags |= 0xFF;
					}
				} else {
					//Picking up or plopping a piece out on the grid
					if (edit_status&EDIT_SELECT) {
						//Put down an object
						if ((t = checkGridCollision(cursorx,cursory)) == 0xFF) {
							selected_object.x = cursorx;
							selected_object.y = cursory;
							addGridBlock(&selected_object);
							updateGridArea();
							edit_status &= ~EDIT_SELECT;
							update_flags |= (PAN_LIM|PAN_INV);
						}
					} else {
						//Pick up an object
						if ((t = checkGridCollision(cursorx,cursory)) != 0xFF) {
							memcpy(&selected_object,&curblueprint->blocks[t],sizeof empty_gridblock);
							curblueprint->blocks[t].block_id = 0; //Remove from grid
							updateGridArea();
							edit_status |= EDIT_SELECT;
							update_flags |= (PAN_LIM|PAN_INV);
						}
					}
					update_flags |= (PAN_GRID|PAN_CURSEL);
				}
			}
		}
		if (kc == kb_Mode) {
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
		/* =================== HANDLE DIRECTIONAL BUTTONS ================= */
		if (edit_status&COLOR_SELECT && !(edit_status&(NOW_PAINTING|PICKING_COLOR))) {
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
		/* ====================== BEGIN FIELD RENDERING =================== */
		//maintenance
		if (update_flags&PAN_FULLSCREEN) {
			update_flags &= ~PAN_FULLSCREEN;  //Clear fullscreen bit
			update_flags |= ~PAN_FULLSCREEN;  //Set all others because dirty.
			gfx_FillScreen(COLOR_BLACK);
			gfx_SetTextFGColor(0x3C|COLOR_LIGHT);    //Picked using color contrast tool
			gfx_PrintStringXY("Ship name",64+5,2);
			gfx_SetTextFGColor(COLOR_WHITE|COLOR_LIGHTER);
			gfx_PrintStringXY(curblueprint->name,(64+10),12);
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
		//if (update_flags&PAN_PREVIEW) {
		//	update_flags &= ~PAN_PREVIEW;
			drawPreview();
		//}
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
	
	//Free all temporary variables
	free(tempblock_grid);
	free(tempblock_inv);
	free(buildarea);
	return;
}

/* ########################################################################## */
/* ########################################################################## */
/* ########################################################################## */

void drawGridArea(void) {
	int sx,tx,w,h;
	uint8_t offset,sy,ty,cols,gridy,gridx,limit,t;
	uint8_t xoff,yoff,i,orientation,spriteid;
	gfx_sprite_t *srcsprite;
	
	offset = 32-((gridlevel-1)<<4);
	sx = 64+6+offset;
	sy = 24+offset;
	limit = (gridlevel<<1)+6;  //8,10,12
	//Draw grid outline
	gfx_SetColor(COLOR_BLACK);
	gfx_FillRectangle_NoClip(sx,sy,16*limit,16*limit);
	gfx_SetColor(COLOR_GRAY|COLOR_LIGHTER);
	gfx_Rectangle_NoClip(sx-1,sy-1,limit*16+2,limit*16+2);
	//Draw actual grid
	for (gridy=0;gridy<limit;gridy++) for (gridx=0;gridx<limit;gridx++)
		gfx_Rectangle_NoClip(sx+(gridx*16),sy+(gridy*16),16,16);
	//Draw big sprite over the grid
	gfx_ScaledTransparentSprite_NoClip(buildarea,sx,sy,2,2);
	//Draw cursor and held block if applicable
	if ((cursorx|cursory)<128) {
		if (edit_status&EDIT_SELECT && selected_object.block_id) {
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

/* No prototype. Bundle with drawSmallInvBox() and drawInventory() */
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
	
	static uint8_t i;
	
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
	gfx_RotatedScaledTransparentSprite_NoClip(buildarea,nx,ny,i++,32);
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
	
	sumStats();  //In dataio.c. Using bpstats.power and bpstats.weight
	
	energy_used = bpstats.weight;
	energy_total = bpstats.power;
	
	if (energy_used>=energy_total) {
		w=60;
		if (energy_used>energy_total) gfx_SetColor(COLOR_RED);
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
		sumStats();  //In dataio.c. Using bpstats.{hp,atk,def,agi,spd}
		
		y = 24+41;
		rightBarStats("HT  PTS",&y,COLOR_GREEN,COLOR_BLACK,bpstats.hp);
		rightBarStats("ATTACK",&y,COLOR_WHITE,COLOR_BLACK,bpstats.atk);
		rightBarStats("ARMOR",&y,COLOR_BLACK,COLOR_WHITE,bpstats.def);
		rightBarStats("SPEED",&y,COLOR_RED,COLOR_WHITE,bpstats.spd);
		rightBarStats("AGILTY",&y,COLOR_BLUE,COLOR_WHITE,bpstats.agi);
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
		gfx_SetColor(0xD6); //This should be a lightish faded red
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

/* ************************************************************************** */
/* ************************************************************************** */


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
		if (!blueprint->blocks[i].block_id) continue; //Do not draw empty blocks
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
		if (!gridblock->block_id) continue;  //Do not catch empty blocks
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
	
	if (!gbo->block_id) return;  //Do not display empty objects
	
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














