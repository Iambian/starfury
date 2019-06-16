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


/* -FILE STRUCTURE NOTES-
	1. Header of variable (gamedata_t). Always assumed to be 255 bytes large.
	   Always copy and write this amount.
	2. Player inventory. Is always 255 bytes large. Always writes to position
	   1 (not 0) of the array.
	3. Pairs of blueprint_obj and gridblock_obj[144]
	   as indicated by gamedata.custom_blueprints_owned
	
*/

/* Function prototypes */
void initPlayerData(void);
ti_var_t openSaveReader(void);
void saveGameData(void);

void setMinimalInventory(void);



/* Static stuff */

char *savefile = "SFuryPDt";
blueprint_obj empty_blueprint = {0,0,"Empty file      ",NULL};

/* Globals */
uint8_t *inventory;              //256 entry wide, indexed by position.
blueprint_obj *curblueprint;     //Almost always points to temp_blueprint
gamedata_t gamedata;             //
blueprint_obj temp_blueprint;    //For reading and writing
gridblock_obj temp_bpgrid[144];  //For reading and writing



/* Functions */

void initPlayerData(void) {
	//Setup game data
	gamedata.file_version = FILE_VERSION;
	gamedata.gridlevel = 1;
	gamedata.blueprints_owned = BP_BASIC;
	gamedata.default_color = COLOR_GREEN;
	gamedata.credits_owned = 100;
	gamedata.custom_blueprints_owned = 1;
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
}

void loadBlueprint(uint8_t bpslot) {
	ti_var_t f;
	int offset;
	
	f = openSaveWriter();
	offset = bpslot * ((sizeof temp_blueprint)*(sizeof temp_bpgrid));
	ti_Seek((2*255)+offset,SEEK_SET,f);
	ti_Read(&temp_blueprint,sizeof temp_blueprint,1,f);
	ti_Read(&temp_bpgrid,sizeof temp_bpgrid,1,f);
	temp_blueprint.blocks = &temp_bpgrid;  //Reassert that pointer
	normalizeBlueprint();
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








