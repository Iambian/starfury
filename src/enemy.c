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
#include "util.h"
#include "gfx/out/enemy_gfx.h"

#define S_MOVX(dx)      0,dx,
#define S_MOVY(dy)      1,dy,
#define S_MOVXY(dx,dy)  2,dx,dy,
#define S_LOOP(ct)      3,ct,
//trace along a circle starting at ang moving at dang along radius rad for ct cycles (using a loop slot)
#define S_SETUPTRACE(ang,dang,rad,ct) 4,ang,dang,rad,ct,
#define S_SETACC(con) 5,con,
#define S_ADDACC(con) 6,con,
//Shoot at angle defined in acc
#define S_SHOOT 7,
#define S_LOOPEND 8,
#define S_SUSPEND 9,
//Actually do the trace. This cmd implies a SUSPEND action
#define S_TRACE 10,
//Shoot at angle acc with rnd being 0-255. 0=never shoot, 255=always shoot.
#define S_SHOOTRAND(rnd) 11,rnd,
#define S_SELFDESTRUCT 12,

/* Function prototypes */
void enShot1(enemy_obj *eobj,uint8_t angle);
void setCollisionField(enemy_obj *eobj);


/* Globals and consts */
enemy_data testEnemyData;
uint8_t testEnemyScript[];


int8_t costab[] = {127,126,126,126,126,126,125,125,124,123,123,122,121,120,119,118,117,116,114,113,112,110,108,107,105,103,102,100,98,96,94,91,89,87,85,82,80,78,75,73,70,67,65,62,59,57,54,51,48,45,42,39,36,33,30,27,24,21,18,15,12,9,6,3,0,-3,-6,-9,-12,-15,-18,-21,-24,-27,-30,-33,-36,-39,-42,-45,-48,-51,-54,-57,-59,-62,-65,-67,-70,-73,-75,-78,-80,-82,-85,-87,-89,-91,-94,-96,-98,-100,-102,-103,-105,-107,-108,-110,-112,-113,-114,-116,-117,-118,-119,-120,-121,-122,-123,-123,-124,-125,-125,-126,-126,-126,-126,-126,-127,-126,-126,-126,-126,-126,-125,-125,-124,-123,-123,-122,-121,-120,-119,-118,-117,-116,-114,-113,-112,-110,-108,-107,-105,-103,-102,-100,-98,-96,-94,-91,-89,-87,-85,-82,-80,-78,-75,-73,-70,-67,-65,-62,-59,-57,-54,-51,-48,-45,-42,-39,-36,-33,-30,-27,-24,-21,-18,-15,-12,-9,-6,-3,0,3,6,9,12,15,18,21,24,27,30,33,36,39,42,45,48,51,54,57,59,62,65,67,70,73,75,78,80,82,85,87,89,91,94,96,98,100,102,103,105,107,108,110,112,113,114,116,117,118,119,120,121,122,123,123,124,125,125,126,126,126,126,126};
uint8_t ens_instr_len[] = {
	//00,01,02,03,04,05,06,07,08,09,10,11,12,13,14,15,16,17,18,19
	   2, 2, 3, 2, 5, 2, 2, 1, 1, 1, 1, 2, 1
	//20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39
};



//Pointer to script pointer to allow the parser to update to next position
void parseEnemy(enemy_obj *eobj,uint8_t **scriptPtr) {
	uint8_t *sptr,t,i;
	
	while (1) {
		sptr = *scriptPtr;
		//dbg_sprintf(dbgout,"enobj: %i, sptr: %i, sptrval: %i\n",eobj,sptr,*sptr);
		switch (*sptr) {
			case 0: //movx
				eobj->x.p.ipart += (int8_t)(sptr[1]);
				setCollisionField(eobj);
				break;
			case 1: //movy
				eobj->y.p.ipart += (int8_t)(sptr[1]);
				setCollisionField(eobj);
				break;
			case 2: //movxy
				eobj->x.p.ipart += (int8_t)(sptr[1]);
				eobj->y.p.ipart += (int8_t)(sptr[2]);
				setCollisionField(eobj);
				break;
			case 3: //loop
				t = eobj->loopdepth;
				eobj->s_loop[t].cnt = sptr[1]; //loop length
				eobj->s_loop[t].ptr = sptr+2;  //cmd after loop
				++(eobj->loopdepth);
				break;
			case 4: //setup trace (circle)
				/* TODO: Figure out if we need to remember where the circle's
						 center point is while tracing its edges. Object's x,y
						 is assumed to be at the outer edge upon start*/
				t = eobj->loopdepth;
				eobj->s_ang  = sptr[1]; //angle
				eobj->s_dang = sptr[2]; //delta angle
				eobj->s_rad  = sptr[3]; //radius
				eobj->s_loop[t].cnt = sptr[4]; //loop length
				eobj->s_loop[t].ptr = sptr+5;  //cmd after loop
				++(eobj->loopdepth);
				break;
			case 5: //setacc
				eobj->s_acc = sptr[1];
				break;
			case 6: //addacc
				eobj->s_acc += sptr[1];
				break;
			case 7: //shoot
				eobj->fShoot(eobj,eobj->s_acc);
				break;
			case 8: //loopend
				t = eobj->loopdepth-1;
				if (--eobj->s_loop[t].cnt) {
					scriptPtr[0] = eobj->s_loop[t].ptr;
					continue;
				}
				//If it makes it out to here, the loop has ended
				eobj->loopdepth = t;
				break;
			case 9: //suspend
				scriptPtr[0]++;  //Advance script pointer by one
				return;          //then stop executing for this cycle
			case 10: //trace
				//Probably should set up some fancy 8-bit trig stuff.
				setCollisionField(eobj);
				break;
			case 11: //shootrand
				if (t = randInt(0,255)) { //Single equal is correct. storing to t.
					if (t >= sptr[1]) {
						eobj->fShoot(eobj,eobj->s_acc);
					}
				}
				//shoot a bullet using enemy's saved bullet routine but only
				//if it passes the random thing
				break;
			case 12: //selfdestruct
				eobj->id = 0;
				//maybe insert a field object depicting an explosion?
				return;  //This should not continue
			default:
				return;  //Stop script if run into undefined command
		}
		//dbg_sprintf(dbgout,"Looping around\n");
		scriptPtr[0] += ens_instr_len[sptr[0]];
	}
}

enemy_obj *findEmptyEnemy(void) {
	uint8_t i;
	
	for (i=0;i<MAX_ENEMY_OBJECTS;i++) {
		if (!(eobjs[i].id))
			return &eobjs[i];
	}
	return NULL;
}

enemy_obj *generateEnemy(enemy_data *edat) {
	enemy_obj *eobj;
	
	if (!(eobj = findEmptyEnemy())) return NULL;
	memset(eobj,0,sizeof empty_eobj);
	memcpy(eobj,edat,sizeof empty_edat);
	if (!eobj->sprite) eobj->sprite = (gfx_sprite_t*)enemy1_data;
	if (!eobj->moveScript) eobj->moveScript = testEnemyScript;
	if (!eobj->fShoot) eobj->fShoot = enShot1;
	
	eobj->hbw = eobj->sprite->width;
	eobj->hbh = eobj->sprite->height;
	
	return eobj;
}

void destroyEnemy(enemy_obj *eobj) {
	eobj->id = 0;
	//Later on, do things that make explosions and do item drops
}





void enShot1(enemy_obj *eobj,uint8_t angle) {
	
	field_obj *fobj;
	if (!(fobj = findEmptyFieldObject())) return;  //No free objs to create
	fobj->flag = FOB_EBUL;
	fobj->power = eobj->atk;
	
	fobj->x.fp = eobj->x.fp + eobj->sprite->width*128;  // div 2, times 256
	fobj->y.fp = eobj->y.fp + eobj->sprite->height*128; // div 2, times 256
	fobj->dx.fp = (int)(costab[angle])*4;
	fobj->dy.fp = (int)(costab[(uint8_t)(angle+64)])*4;
	fobj->fMov = smallShotMove;
	
}

void setCollisionField(enemy_obj *eobj) {
	unsigned int t;
	t = (eobj->x.p.ipart);
	if (t>320) t=0;
	else t >>= 1;
	eobj->hbx = (uint8_t) t;
	t = (eobj->y.p.ipart);
	if (t>240) t=0;
	eobj->hby = (uint8_t) t;
	//hitbox height and width is set during enemy initialization
}



enemy_data testEnemyData = {1,100,1,1,NULL,NULL,NULL};
uint8_t testEnemyScript[] = {
	S_LOOP(50)
		S_MOVX(-3)
		S_SUSPEND
	S_LOOPEND
	S_SETACC(128)
	S_LOOP(20)
		S_LOOP(15)
			S_MOVX(1)
			S_SUSPEND
		S_LOOPEND
		S_LOOP(15)
			S_MOVY(1)
			S_SUSPEND
		S_LOOPEND
		S_LOOP(15)
			S_MOVX(-1)
			S_SUSPEND
		S_LOOPEND
		S_LOOP(15)
			S_MOVY(-1)
			S_SUSPEND
		S_LOOPEND
		S_SHOOT
	S_LOOPEND
	S_SELFDESTRUCT
};










