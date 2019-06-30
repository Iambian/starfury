/* Cross file declaration(s) for the ship editor */

#ifndef __enemy_cutil__
#define __enemy_cutil__

#include <stdint.h>
#include <graphx.h>		//Provides typedefs for graphx objects
#include "defs.h"

#define S_MOVX(dx)      0,x
#define S_MOVY(dy)      1,y
#define S_MOVXY(dx,dy)  2,dx,dy
#define S_LOOP(ct)      3,ct
//trace along a circle starting at ang moving at dang along radius rad for ct cycles (using a loop slot)
#define S_TRACE(ang,dang,rad,ct) 4,ang,dang,rad,ct
#define S_SETACC(con) 5,con
#define S_ADDACC(con) 6,con
//Shoot at angle defined in acc
#define S_SHOOT 7



parseEnemy(enemy_obj *eobj,uint8_t **scriptPtr);




#endif