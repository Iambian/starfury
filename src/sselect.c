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
#include "sselect.h"
#include "menu.h"
#include "edit.h"

#define STRI_TOPY (20+64+4+((64-32)/2))
#define STRI_BTMY (20+64+4+((64-32)/2)+32)
#define STRI_MIDY (20+64+4+((64-32)/2)+32-16)
#define STRI_LX (64-32)
#define STRI_RX (64+128+64+(64-32))


/* Function prototypes */
uint8_t prevShipIndex(uint8_t sidx);
uint8_t nextShipIndex(uint8_t sidx);
void renderShipFile(uint8_t pos,uint8_t index);  //pos: 0,1,2. index is shipselidx
void copyShip(uint8_t ssidx);

/* Globals and static variables */
uint8_t shipselidx;
const char *bpcopywhere = "Copy blueprint where?";
const char *bpdefault[] = {"Options","Fly this ship","Copy to custom blueprint"};
const char *bpcustom[] = {"Options","Fly this ship","Copy to another blueprint","Edit this blueprint","Rename this blueprint","Clear this blueprint"};
const char *bpnothingthere[]= {"You fool!","You can't fly a ship","without having built","anything yet!"};
char **bpcopy;
const char *bpoverwritten[] = {"Notice","File has been overwritten!"};
char *bperaseconfirm[] = {"Really erase this file?","Do not","Erase this"};
char nameinput[17];




void shipSelect(void) {
	kb_key_t kc,kd;
	uint8_t i,update_flags,tx,ty,t,limit;
	uint8_t tt;

	//Initialize selector render
	shipselidx = 0;  //You will always have a starting blueprint
	bpcopy = malloc(sizeof(bpcopywhere)*8); //Need 8 char pointer slots
	bpcopy[0] = bpcopywhere;

	while (1) {
		kb_Scan();
		kd = kb_Data[7];
		kc = kb_Data[1];
		if (kc&kb_Mode) { keywait(); break; }
		
		if (kc&kb_2nd) {
			if (shipselidx<8) {
				t = staticMenu(bpdefault,3);
				if (1==t) {
					//Fly the ship
				} else 	if (2==t) {
					//Copy ship to another file
					copyShip(shipselidx);
				}
			} else {
				t = staticMenu(bpcustom,6);
				getShipData(shipselidx);
				if (1==t) {
					if (!temp_blueprint.numblocks) {
						alert(bpnothingthere,4);
					} else if (0) {
						//Fill out other conditions in case you try to fly a ship
						//that won't actually fly.
					}
					
				} else if (2==t) {
					//Copy ship to another file
					copyShip(shipselidx);
				} else if (3==t) {
					//Edit operation
					//DEBUG: COMMENT OUT TO "OPTIMIZE" AWAY EDITOR COMPILE FOR FASTER TESTING
					openEditor(shipselidx-8);
				} else if (4==t) {
					//Rename operation
					if (nameInput(&temp_blueprint.name)) saveBlueprint(shipselidx-8);
				} else if (5==t) {
					//Clear operation
					if (2==staticMenu(bperaseconfirm,3)) {
						memcpy(&temp_blueprint,&empty_blueprint,sizeof empty_blueprint);
						saveBlueprint(shipselidx-8);
					}
				}
			}
		}

		gfx_FillScreen(COLOR_BLACK);
		
		if (kd == kb_Up) {
			if (shipselidx) shipselidx = prevShipIndex(shipselidx);
			dbg_sprintf(dbgout,"idx: %i\n",shipselidx);
		}
		if (kd == kb_Down) {
			if (0xFF != (t=nextShipIndex(shipselidx)))
				shipselidx = t;
			dbg_sprintf(dbgout,"idx: %i\n",shipselidx);
		}
		
		tt = 0x15|COLOR_LIGHTER; //Steel gray
		for (i=0;i<4;i++,tt-=COLOR_DARK) {
			gfx_SetColor(tt);
			gfx_FillRectangle_NoClip(0,i*4,320,4);
		}
		gfx_SetTextFGColor(0x3C|COLOR_LIGHT);    //Picked using color contrast tool
		gfx_PrintStringXY("Ship Selection",8,4);
		
		gfx_SetColor(0x56); //A faded blue
		gfx_FillTriangle(STRI_LX,STRI_TOPY,STRI_LX,STRI_BTMY,STRI_LX+24,STRI_MIDY); //L
		gfx_FillTriangle(STRI_RX,STRI_TOPY,STRI_RX,STRI_BTMY,STRI_RX-24,STRI_MIDY); //R
		gfx_SetColor(0xD6); //A faded blue
		gfx_FillTriangle(STRI_LX+2,STRI_TOPY+4,STRI_LX+2,STRI_BTMY-4,STRI_LX+24-4,STRI_MIDY);
		gfx_FillTriangle(STRI_RX-2,STRI_TOPY+4,STRI_RX-2,STRI_BTMY-4,STRI_RX-24+4,STRI_MIDY);
		
		renderShipFile(0,prevShipIndex(shipselidx));
		renderShipFile(1,shipselidx);
		renderShipFile(2,nextShipIndex(shipselidx));
		
		gfx_BlitBuffer();
		if (kd|kc) keywait();
	}

}

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
	gfx_SetTextXY(x,*y);
	gfx_PrintString(s);
	gfx_PrintChar(' ');
	gfx_PrintUInt(stat,3);
	*y += 10;
}
//pos: 0,1,2. 1 is the middle option
void renderShipFile(uint8_t pos,uint8_t index) {
	uint8_t i,ybase,colorbase,ytemp;
	int xbase;
	
	if (getShipData(index)) return;
	//dbg_sprintf(dbgout,"Rendering... \n");
	curblueprint = &temp_blueprint;
	curblueprint->blocks = temp_bpgrid;
	
	//new dims: 30 px top and bottom, 12px inbetween
	xbase = 64;
	ybase = 30+pos*(52+12);
	
	if (index<8) colorbase = 0x25; //A faded red, base
	else         colorbase = 0x16; //A faded blue, lighter
	
	//Draw outline
	menuRectangle(xbase,ybase,192,52,colorbase);
	//Draw blueprint name
	gfx_SetTextFGColor(COLOR_WHITE);
	gfx_PrintStringXY(curblueprint->name,xbase+(50+8),ybase+4);
	//Draw divider line between blueprint name and stats
	gfx_SetColor(colorbase|COLOR_LIGHTER);
	gfx_HorizLine(xbase+(48+4),ybase+15,(144-8));
	//Draw preview
	drawShipPreview(mainsprite);
	gfx_TransparentSprite_NoClip(mainsprite,xbase+2,ybase+2);
	
	if (curblueprint->numblocks) {
		sumStats();
		ytemp = ybase + (2+10+8);
		rsf_textline(xbase+(48+14+8),&ytemp,"HP",bpstats.hp);
		rsf_textline(xbase+(48+14),&ytemp,"ATK",bpstats.atk);
		rsf_textline(xbase+(48+14),&ytemp,"DEF",bpstats.def);
		ytemp = ybase + (2+10+8);
		rsf_textline(xbase+(48+14+51+14+8),&ytemp,"PW",bpstats.power);
		rsf_textline(xbase+(48+14+51+14),&ytemp,"SPD",bpstats.spd);
		rsf_textline(xbase+(48+14+51+14),&ytemp,"AGI",bpstats.agi);
	}
}

void copyShip(uint8_t ssidx) {
	uint8_t tt;
	loadAllShipNames(bpcopy);
	if (tt=staticMenu(bpcopy,1+gamedata.custom_blueprints_owned)) {
		getShipData(ssidx);
		saveBlueprint(tt-1);
		alert(bpoverwritten,2);
	}
}






