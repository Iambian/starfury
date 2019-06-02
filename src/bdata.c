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

#include "defs.h"


blockprop_obj blockobject_list[5] = {
/*   sptr,type   ,cost,w,h, hp,atk,def,agi,spd,name         */
	{NULL,FILLER , 255,1,1,255,255,255,255,255,"Empty block"},
	{NULL,COMMAND,  40,2,2, 20,  1,  0,  0,  0,"S Cockpit"},
	{NULL,COMMAND,  60,2,2, 40,  3,  5,  0,  0,"M Cockpit"},
	{NULL,COMMAND,  80,2,2, 60,  7, 10,  0,  0,"L Cockpit"},
	{NULL,FILLER ,  01,1,1,  5,  0,  0,  0,  0,"S Armor"},
	
};













