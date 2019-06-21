#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <debug.h>
#include <intce.h>
#include <keypadc.h>
#include <graphx.h>
#include <decompress.h>
#include <fileioc.h>

#include "main.h"
#include "defs.h"
#include "dataio.h"
#include "bdata.h"
#include "util.h"


/* -FILE STRUCTURE NOTES-
	1. Header of variable (gamedata_t). Always assumed to be 255 bytes large.
	   Always copy and write this amount.
	2. Player inventory. Is always 255 bytes large. Always writes to position
	   1 (not 0) of the array.
	3. Pairs of blueprint_obj and gridblock_obj[144]
	   as indicated by gamedata.custom_blueprints_owned
	
*/

/* Function prototypes (defined in dataio.h) */


/* Static stuff */
char emptyblueprint[] = "Empty blueprint ";
char *emptystring = "";

char *savefile = "SFuryPDt";
blueprint_obj empty_blueprint = {0,0,"Empty blueprint ",NULL};
gridblock_obj empty_gridblock = {0,0,0,0,ROT_1};

/* Globals */
uint8_t *inventory;              //256 entry wide, indexed by position.
blueprint_obj *curblueprint;     //Almost always points to temp_blueprint
gamedata_t gamedata;             //
blueprint_obj temp_blueprint;    //For reading and writing
gridblock_obj temp_bpgrid[144];  //For reading and writing
stats_sum bpstats;               //For storing the sum of all the blueprint's stats


/* Functions */

void initPlayerData(void) {
	//Setup game data
	gamedata.file_version = FILE_VERSION;
	gamedata.gridlevel = 1;
	gamedata.blueprints_owned = BP_BASIC;
	gamedata.default_color = DEFAULT_COLOR;
	gamedata.credits_owned = 100;
	gamedata.custom_blueprints_owned = 0;
	createNewBlueprint();  //Initialize first user blueprint
	//Setup inventory
	inventory = malloc(255);
	setMinimalInventory();
	//Setup blueprint and grid area
	curblueprint = &temp_blueprint;
	memcpy(curblueprint,&empty_blueprint,sizeof empty_blueprint);
	curblueprint->blocks = &temp_bpgrid;
}

//This MUST be among the first things done. It also MUST return a nonzero
//value, otherwise we can't (reliably) use the save file
ti_var_t openSaveReader(void) {
	uint8_t r,i;
	ti_var_t f;
	
	ti_CloseAll();
	f = ti_Open(savefile,"r");  //Open for reading
	if (!f || (ti_GetC(f)!=FILE_VERSION)) {
		//Read failure. Open it for writing
		f = ti_Open(savefile,"w");
		if (!f) return 0;  //Open/create failure. File is unopenable.
		/* Thought: If ti_Write ever returns 0, r should clear at some point */
		r = 1;
		r &= ti_Write(&gamedata,255,1,f);
		r &= ti_Write(inventory+1,255,1,f);
		for (i=0;i<7;i++) {
			r &= ti_Write(&temp_blueprint,sizeof temp_blueprint,1,f);
			r &= ti_Write(&temp_bpgrid,sizeof temp_bpgrid,1,f);
		}
		if (!r) return 0;
		ti_CloseAll();
		f = ti_Open(savefile,"r");
		if (!f) return 0; //After all that and it still won't open? bleh.
	}
	return f;
	
}

/* Not prototyped. Bundle with saveGameData and saveBlueprint */
ti_var_t openSaveWriter(void) {
	ti_var_t f;
	
	openSaveReader();
	ti_CloseAll();
	f = ti_Open(savefile,"a+");
	ti_Rewind(f);
	return f;
}


void saveGameData(void) {
	ti_var_t f;
	
	f = openSaveWriter();
	ti_Write(&gamedata,255,1,f);
	ti_Write(inventory+1,255,1,f);
	ti_CloseAll();
}

//bpslot 0-6 (maps to 1-7. There is no eigth slot. There's also no write guard */
void saveBlueprint(uint8_t bpslot) {
	ti_var_t f;
	int offset;
	
	f = openSaveWriter();
	offset = bpslot * ((sizeof temp_blueprint)*(sizeof temp_bpgrid));
	ti_Seek((2*255)+offset,SEEK_SET,f);
	ti_Write(&temp_blueprint,sizeof temp_blueprint,1,f);
	ti_Write(&temp_bpgrid,sizeof temp_bpgrid,1,f);
	ti_CloseAll();

}

void loadBlueprint(uint8_t bpslot) {
	ti_var_t f;
	int offset;
	
	//dbg_sprintf(dbgout,"loadBluerpint(%i)\n",bpslot);

	if (bpslot >= gamedata.custom_blueprints_owned) {
		temp_blueprint.gridlevel = 0; //Load failed. Invalidate blueprint
		return;
	}
	//dbg_sprintf(dbgout,"lbp loading...\n");
	f = openSaveWriter();
	offset = bpslot * ((sizeof temp_blueprint)*(sizeof temp_bpgrid));
	ti_Seek((2*255)+offset,SEEK_SET,f);
	ti_Read(&temp_blueprint,sizeof temp_blueprint,1,f);
	ti_Read(&temp_bpgrid,sizeof temp_bpgrid,1,f);
	temp_blueprint.blocks = &temp_bpgrid;  //Reassert that pointer
	ti_CloseAll();
	//dbg_sprintf(dbgout,"lbp loaded with gridlevel %i\n",temp_blueprint.gridlevel);
	normalizeBlueprint();
	//dbg_sprintf(dbgout,"lbp normalized with gridlevel %i\n",temp_blueprint.gridlevel);
}

void loadBuiltinBlueprint(uint8_t bpslot) {
	blueprint_obj *tbpo;
	
	bpslot &= 7;
	tbpo = builtin_blueprints[bpslot];
	memcpy(&temp_blueprint,tbpo,sizeof empty_blueprint);
	memcpy(&temp_bpgrid,tbpo->blocks,sizeof temp_bpgrid); //A little overcopying is fine
	temp_blueprint.blocks = &temp_bpgrid;  //Reassert that pointer
	
	normalizeBlueprint();
}

void resetBlueprint(void) {
	memcpy(&temp_blueprint,&empty_blueprint,sizeof empty_blueprint);
	memset(&temp_bpgrid,0,sizeof temp_bpgrid);
	temp_blueprint.gridlevel = gamedata.gridlevel;
	temp_blueprint.blocks = &temp_bpgrid;
}

//Adds an empty (valid) blueprint and advances character data by one
//Clobbers all temps.
uint8_t createNewBlueprint(void) {
	if (gamedata.custom_blueprints_owned>6) return 1; //No. Do not add any more.
	resetBlueprint();
	saveBlueprint(gamedata.custom_blueprints_owned++);
	saveGameData();  //save the incremented ownership back to file
	return 0;
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

void normalizeBlueprint(void) {
	uint8_t i;
	int8_t offset;
	
	if (temp_blueprint.gridlevel > gamedata.gridlevel) {
		//If you can't use this blueprint, invalidate it
		temp_blueprint.numblocks = 0;
		temp_blueprint.gridlevel = gamedata.gridlevel;
	} else {
		//Start adjusting positions of all blocks on the grid to match
		offset = (gamedata.gridlevel - temp_blueprint.gridlevel)*2;
		temp_blueprint.gridlevel = gamedata.gridlevel;
		for (i=0;i<temp_blueprint.numblocks;i++) {
			temp_bpgrid[i].x += offset;
			temp_bpgrid[i].y += offset;
		}
	}
}

void addGridBlock(gridblock_obj *gbo) {
	uint8_t i;
	
	//Search for first free spot
	for (i=0;i<temp_blueprint.numblocks;i++) {
		if (!temp_blueprint.blocks[i].block_id) {
			memcpy(&temp_blueprint.blocks[i],gbo,sizeof empty_gridblock);
			return;  //Block added. Finish.
		}
	}
	//If not found, append and expand
	memcpy(&temp_blueprint.blocks[temp_blueprint.numblocks],gbo,sizeof empty_gridblock);
	temp_blueprint.numblocks++;
}

void sumStats(void) {
	uint8_t i,blockid;
	blockprop_obj *bpo;
	
	memset(&bpstats,0,sizeof bpstats);
	for (i=0;i<curblueprint->numblocks;i++) {
		if (!(blockid = curblueprint->blocks[i].block_id)) continue; //Don't do empty blocks
		bpo = &blockobject_list[blockid];
		bpstats.hp  += bpo->hp;
		bpstats.atk += bpo->atk;
		bpstats.def += bpo->def;
		bpstats.agi += bpo->agi;
		bpstats.spd += bpo->spd;
		if (bpo->type & (COMMAND|ENERGYSOURCE)) bpstats.power  += bpo->cost;
		else                                    bpstats.weight += bpo->cost;
		if (bpo->type & COMMAND) ++bpstats.cmd;
		if (bpo->type & WEAPON) ++bpstats.wpn;
	}
}

//Use sprite dimensions PREVIEWBLOCK_MAX_W, PREVIEWBLOCK_MAX_H for input sprite
void drawShipPreview(gfx_sprite_t *insprite) {
	uint8_t i,w,h,offset,x,y,blockid;
	gfx_sprite_t *srcsprite;
	blockprop_obj *bpo;
	gridblock_obj *gbo;
	
	if (!curblueprint->gridlevel) return;
	//On 48x48 plane, L1 to L3 is diff of 8x8 to 12x12 or 32x32 to 48x48. Mult by 4.
	//But base diff is up to 2. That's +2 to center the offset [(12-8)/2] so...
	//just multiply by 2? Neh. The multiply has to be the final step.
	offset = 3-curblueprint->gridlevel;
	fn_FillSprite(insprite,TRANSPARENT_COLOR);
	for (i=0;i<curblueprint->numblocks;i++) {
		blockid = curblueprint->blocks[i].block_id;
		if (!blockid) continue;
		gbo = &curblueprint->blocks[i];
		bpo = &blockobject_list[blockid];
		srcsprite = bpo->sprite;
		//if (!((gbo=&curblueprint->blocks[i])->block_id)) continue; //No empties
		//bpo = &blockobject_list[gbo->block_id];
		//if (!(srcsprite = bpo->sprite)) continue;  //No out of range (NULL) sprites
		w = tempblock_smallscratch->width = srcsprite->width >> 1;   //quick and dirty
		h = tempblock_smallscratch->height = srcsprite->height >> 1; //div by 2 for each
		//dbg_sprintf(dbgout,"Item %i, (w,h)=(%i,%i)\n",i,w,h);
		//Shrink down and move to tempsprite
		gfx_ScaleSprite(srcsprite,tempblock_smallscratch);
		switch (gbo->orientation&3) {
			//roatate, flip, or copy from tempsprite and move to tempblock_scratch
			case ROT_0: memcpy(tempblock_scratch,tempblock_smallscratch,w*h+2); break;
			case ROT_1: gfx_RotateSpriteC(tempblock_smallscratch,tempblock_scratch); break;
			case ROT_2: gfx_RotateSpriteHalf(tempblock_smallscratch,tempblock_scratch); break;
			case ROT_3: gfx_RotateSpriteCC(tempblock_smallscratch,tempblock_scratch); break;
			default: break;
		}
		if (gbo->orientation&HFLIP)
			gfx_FlipSpriteY(tempblock_scratch,tempblock_smallscratch);
		else
			memcpy(tempblock_smallscratch,tempblock_scratch,h*w+2);
		fn_PaintSprite(tempblock_smallscratch,gbo->color);
		fn_DrawNestedSprite(tempblock_smallscratch,insprite,(gbo->x+offset)<<2,(gbo->y+offset)<<2);
	}
}


//Uses the fact that the TI-OS keeps all files in the same memory 
void loadAllShipNames(char **namearray) {
	uint8_t i;
	int skiplen;
	ti_var_t f;
	blueprint_obj *bpo;
	
	if (!(f = openSaveReader())) for (i=1;i<8;i++) namearray[i]=emptystring;
	ti_Seek((2*255),SEEK_SET,f);
	for (i=1;i<8;i++) {
		bpo = ti_GetDataPtr(f);
		namearray[i] = &(bpo->name);
		skiplen = bpo->numblocks * (sizeof empty_gridblock);
		ti_Seek((sizeof empty_blueprint) + skiplen,SEEK_CUR,f);
	}
}
