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

#define S_MOVX(dx)      0,x,
#define S_MOVY(dy)      1,y,
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

uint8_t ens_instr_len[] = {
	//00,01,02,03,04,05,06,07,08,09,10,11,12,13,14,15,16,17,18,19
	   2, 2, 3, 2, 5, 2, 2, 1, 1, 1, 1,
	//20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39
};
//uint8_t *s_loop1,*s_loop2,s_loop1ct,s_loop2ct,s_acc,s_rad,s_ang;



//Pointer to script pointer to allow the parser to update to next position
void parseEnemy(enemy_obj *eobj,uint8_t **scriptPtr) {
	uint8_t *sptr,t,i;
	sptr = *scriptPtr;
	
	while (1) {
		switch (*sptr) {
			case 0: //movx
				eobj->x.p.ipart += sptr[1];
				break;
			case 1: //movy
				eobj->y.p.ipart += sptr[1];
				break;
			case 2: //movxy
				eobj->x.p.ipart += sptr[1];
				eobj->y.p.ipart += sptr[2];
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
				//probably should set up a firing routine? probably on init.
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
				break;
			default:
				return;  //Stop script if run into undefined command
		}
		scriptPtr[0] += ens_instr_len[sptr[0]];
	}
}












