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
#include "enemy.h"

//Field objects can include player bullets, enemy bullets, and powerups




/* Put your function prototypes here */
void setShotStats(field_obj *fobj, weapon_obj *wobj, int velocity);

void shotTur1(struct weapon_obj_struct *wobj);
void shotTur2(struct weapon_obj_struct *wobj);
void shotTur3(struct weapon_obj_struct *wobj);
void shotLas1(struct weapon_obj_struct *wobj);
void shotLas2(struct weapon_obj_struct *wobj);
void shotLas3(struct weapon_obj_struct *wobj);
void shotMis1(struct weapon_obj_struct *wobj);
void shotMis2(struct weapon_obj_struct *wobj);
void shotMis3(struct weapon_obj_struct *wobj);




const uint8_t transparent_color = TRANSPARENT_COLOR;
void keywait(void);
void waitanykey(void);
void error(char *msg);

/* Put all your globals here */
gfx_sprite_t *mainsprite;
gfx_sprite_t *altsprite;
gfx_sprite_t *tempblock_scratch;
gfx_sprite_t *tempblock_smallscratch;



/* Globals and defines that will be moved out to a new file once done testing */
uint8_t maintimer;
field_obj *fobjs;
enemy_obj *eobjs;
weapon_obj *wobjs;
field_obj  empty_fobj;
enemy_obj  empty_eobj;
weapon_obj empty_wobj;
enemy_data empty_edat;

fp168 playerx,playery;
int player_curhp;  //Maximums and other stats located in bpstats struct




void main(void) {
	kb_key_t kc,kd;
	void (*fShot)(struct weapon_obj_struct *wobj);
	fp168 tempfp;
	int tempint;
	gridblock_obj *gbo;
	blockprop_obj *bpo;
	field_obj *fobjs_cur;
	field_obj *fobjs_end;
	uint8_t i,k,update_flags,tx,ty,t,limit;
	uint8_t tt;
	enemy_obj *eobj;

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
	
	//Initialize gameplay test environment
	loadBuiltinBlueprint(0);      //Load first builtin blueprint
	drawShipPreview(mainsprite);  //Prerender the ship
	curblueprint = &temp_blueprint;  //idk why this isn't done already but meh
	sumStats();  //bpstats.[hp,power,weight,atk,def,spd,agi,wpn,cmd]
	
	
	//Allocate and initialize object memory
	fobjs = malloc(MAX_FIELD_OBJECTS * (sizeof empty_fobj));
	memset(fobjs,0,MAX_FIELD_OBJECTS * (sizeof empty_fobj));
	eobjs = malloc(MAX_ENEMY_OBJECTS * (sizeof empty_eobj));
	memset(eobjs,0,MAX_ENEMY_OBJECTS * (sizeof empty_eobj));
	if (bpstats.wpn) {
		wobjs = malloc(bpstats.wpn * (sizeof empty_wobj));
		memset(wobjs,0,bpstats.wpn * (sizeof empty_wobj));
	} else 	wobjs = NULL;
	//Initialize weapon memory
	for (k=i=0;i<curblueprint->numblocks;i++) {
		bpo = &blockobject_list[(gbo = &curblueprint->blocks[i])->block_id];
		if (bpo->type & WEAPON) {
			t = 3-curblueprint->gridlevel; //offset
			wobjs[k].xoffset = (gbo->x+t)<<2;
			wobjs[k].yoffset = (gbo->y+t)<<2;
			t = gbo->orientation;
			//collapse hflip to 180 deg orientation for direction and change to DIR enum
			wobjs[k].fire_direction = ((t&4)>>1)^(t&3);
			//wobjs[k].cooldown = 0; //Not necessary -- already zeroed w/ memset
			//dbg_sprintf(dbgout,"Weapon obj %i found\n",gbo->block_id);
			switch (gbo->block_id) {
				case BLOCK_TUR1: t =  5; tt =  1; fShot=shotTur1; break;
				case BLOCK_TUR2: t =  5; tt =  2; fShot=shotTur2; break;
				case BLOCK_TUR3: t =  5; tt =  3; fShot=shotTur3; break;
				case BLOCK_LAS1: t = 20; tt =  1; fShot=shotLas1; break;
				case BLOCK_LAS2: t = 20; tt =  2; fShot=shotLas2; break;
				case BLOCK_LAS3: t = 15; tt =  3; fShot=shotLas3; break;
				case BLOCK_MIS1: t = 40; tt = 20; fShot=shotMis1; break;
				case BLOCK_MIS2: t = 40; tt = 30; fShot=shotMis2; break;
				case BLOCK_MIS3: t = 40; tt = 40; fShot=shotMis3; break;
				default:         t = 10; tt =  0; fShot=NULL    ; break;
			}
			wobjs[k].cooldown_on_firing = t;
			wobjs[k].power = bpstats.atk + tt;
			wobjs[k].fShot = fShot;
			++k;
		}
	}
	
	//Insert debug generation
	
	eobj = generateEnemy(&testEnemyData);
	eobj->x.p.ipart = 420; eobj->y.p.ipart = 30;
	eobj = generateEnemy(&testEnemyData);
	eobj->x.p.ipart = 400; eobj->y.p.ipart = 50;
	eobj = generateEnemy(&testEnemyData);
	eobj->x.p.ipart = 380; eobj->y.p.ipart = 70;
	eobj = generateEnemy(&testEnemyData);
	eobj->x.p.ipart = 360; eobj->y.p.ipart = 90;
	eobj = generateEnemy(&testEnemyData);
	eobj->x.p.ipart = 340; eobj->y.p.ipart = 110;
	eobj = generateEnemy(&testEnemyData);
	eobj->x.p.ipart = 360; eobj->y.p.ipart = 130;
	eobj = generateEnemy(&testEnemyData);
	eobj->x.p.ipart = 380; eobj->y.p.ipart = 150;
	eobj = generateEnemy(&testEnemyData);
	eobj->x.p.ipart = 400; eobj->y.p.ipart = 170;
	eobj = generateEnemy(&testEnemyData);
	eobj->x.p.ipart = 420; eobj->y.p.ipart = 190;
	
	
	keywait();
	// I think we have enough object memory allocated and initialized?
	while (1) {
		kb_Scan();
		kd = kb_Data[7];
		kc = kb_Data[1];
		if (kc&kb_Mode) { keywait(); break; }
		if (kc&kb_2nd) {
			for (i=0;i<bpstats.wpn;i++) {
				if (!wobjs[i].cooldown && wobjs[i].fShot) {
					wobjs[i].cooldown = wobjs[i].cooldown_on_firing;
					wobjs[i].fShot(&wobjs[i]);
				}
			}
		}
		
		
		
		if (kd&kb_Left) {
			if ((tempint = playerx.fp-(128*bpstats.spd))> 0) {
				playerx.fp = tempint;
			} else playerx.fp = 0;
		}
		if (kd&kb_Right) {
			if ((tempint = playerx.fp+(128*bpstats.spd))< ((320-48)*256)) {
				playerx.fp = tempint;
			} else playerx.fp = ((320-48)*256);
		}
		if (kd&kb_Up) {
			if ((tempint = playery.fp-(128*bpstats.spd))> 0) {
				playery.fp = tempint;
			} else playery.fp = 0;
		}
		if (kd&kb_Down) {
			if ((tempint = playery.fp+(128*bpstats.spd))< ((240-48)*256)) {
				playery.fp = tempint;
			} else playery.fp = ((240-48)*256);
		}
		
		
		gfx_FillScreen(COLOR_BLACK);
		//Render and move all enemy sprites
		for (i=0;i<MAX_ENEMY_OBJECTS;i++) {
			if ((eobj = &eobjs[i])->id) {
				if (eobj->hitcounter && !(eobj->hitcounter&1)) fn_InvertSprite(eobj->sprite);
				gfx_TransparentSprite(eobj->sprite,eobj->x.p.ipart,eobj->y.p.ipart);
				if (eobj->hitcounter) {
					if (!(eobj->hitcounter&1)) fn_InvertSprite(eobj->sprite);
					--(eobj->hitcounter);
				}
				parseEnemy(eobj,&(eobj->moveScript));
			}
		}
		
		
		//Render player ship
		gfx_TransparentSprite_NoClip(mainsprite,playerx.p.ipart,playery.p.ipart);
		//Render all field objects
		for (fobjs_end=(fobjs_cur=fobjs)+MAX_FIELD_OBJECTS;fobjs_cur<fobjs_end;fobjs_cur++) {
			if (fobjs_cur->flag && fobjs_cur->fMov) fobjs_cur->fMov(fobjs_cur);
		}
		
		
		
		// At end of cycle, reduce cooldown timers on all weapons
		for (i=0;i<bpstats.wpn;i++) if (wobjs[i].cooldown) --(wobjs[i].cooldown);
		
		gfx_BlitBuffer();
		//gfx_SwapDraw();
		
	}
	
	//Battle system cleanup
	free(fobjs);
	free(eobjs);
	free(wobjs);
	
	
	
	
	/* Preserve save file and exit */
//	int_Reset();
	gfx_End();
	saveGameData();
	ti_CloseAll();
	exit(0);
}

/* Put other functions here */






field_obj *findEmptyFieldObject(void) {
	static int i=0; //Persist so i never has to travel too far to find next empty
	int prev;
	
	prev = i;
	do {
		if (i==MAX_FIELD_OBJECTS) i=0;
		if (!fobjs[i].flag) return &fobjs[i];
	} while (++i != prev);
	return NULL;
}

void setShotStats(field_obj *fobj, weapon_obj *wobj, int velocity) {
	fobj->power = wobj->power;
	fobj->x.fp = playerx.fp + (256*wobj->xoffset);
	fobj->y.fp = playery.fp + (256*wobj->yoffset);
	switch (wobj->fire_direction) {
		case 0:  fobj->dx.fp = velocity ; fobj->dy.fp = 0        ; break;
		case 1:  fobj->dx.fp = 0        ; fobj->dy.fp = velocity ; break;
		case 2:  fobj->dx.fp = -velocity; fobj->dy.fp = 0        ; break;
		case 3:  fobj->dx.fp = 0        ; fobj->dy.fp = -velocity; break;
		default: fobj->dx.fp = -9999    ; fobj->dy.fp = 0        ; break;
	}
}

//Create an actual sprite graphic later
uint8_t smallshotdat[] = {3,3, 0,255,0, 255,255,255, 0,255,0};
void smallShotMove(struct field_obj_struct *fobj) {
	int tempfxp;
	uint8_t bx,by,i;
	if ((unsigned int)(tempfxp = fobj->x.fp + fobj->dx.fp) >= ((320-3)*256)) {
		fobj->flag = 0;  //destroy object
		return;
	} else fobj->x.fp = tempfxp;
	if ((unsigned int)(tempfxp = fobj->y.fp + fobj->dy.fp) >= ((240-3)*256)) {
		fobj->flag = 0;  //destroy object
		return;
	} else fobj->y.fp = tempfxp;
	//Run bullet collision. Bullet hitbox is single point dead center
	if (fobj->flag & FOB_PBUL) {
		bx = ((fobj->x.p.ipart)>>1)+1;
		by = (fobj->y.p.ipart)+1;
		for (i=0;i<MAX_ENEMY_OBJECTS;i++) {
			if (eobjs[i].id && (bx>=eobjs[i].hbx && bx<(eobjs[i].hbx+eobjs[i].hbw)) && (by>=eobjs[i].hby && by<(eobjs[i].hby+eobjs[i].hbh))) {
				eobjs[i].hitcounter += 2;
				if ((eobjs[i].hp -= fobj->power)<0) destroyEnemy(&eobjs[i]);
				fobj->flag = 0; //destroy bullet
			}
		}
	} else if (fobj->flag & FOB_EBUL) {
		
		
		
	}
	//Draw. This won't be reached if the bullet collided with something.
	gfx_TransparentSprite_NoClip((gfx_sprite_t*)smallshotdat,fobj->x.p.ipart,fobj->y.p.ipart);
}


void shotTur1(struct weapon_obj_struct *wobj) {
	field_obj *fobj;
	if (!(fobj=findEmptyFieldObject())) return;
	dbg_sprintf(dbgout,"Empty field object ptr %i\n",fobj);
	fobj->flag = FOB_PBUL;
	setShotStats(fobj,wobj,1024);
	fobj->fMov = smallShotMove;
}
void shotTur2(struct weapon_obj_struct *wobj) {
	
}
void shotTur3(struct weapon_obj_struct *wobj) {
	
}
void shotLas1(struct weapon_obj_struct *wobj) {
	
}
void shotLas2(struct weapon_obj_struct *wobj) {
	
}
void shotLas3(struct weapon_obj_struct *wobj) {
	
}
void shotMis1(struct weapon_obj_struct *wobj) {
	
}
void shotMis2(struct weapon_obj_struct *wobj) {
	
}
void shotMis3(struct weapon_obj_struct *wobj) {
	
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
