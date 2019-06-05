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
#include "gfx/out/blox_gfx.h"

#define GST (gfx_sprite_t*)

blockprop_obj blockobject_list[] = {
/*   sptr                ,type   ,cost,w,h, hp,atk,def,agi,spd,name         */
	{GST NULL            ,FILLER , 255,1,1,255,255,255,255,255,"Empty block"},
	{GST lv1cic_data     ,COMMAND,  40,2,2, 20,  1,  0,  0,  0,"S Cockpit"},
	{GST lv2cic_data     ,COMMAND,  60,2,2, 40,  3,  5,  0,  0,"M Cockpit"},
	{GST lv3cic_data     ,COMMAND,  80,2,2, 60,  7, 10,  0,  0,"L Cockpit"},
	{GST lv1square_data  ,FILLER ,   1,1,1,  2,  0,  0,  0,  0,"S Armor"},
	{GST lv1triangle_data,FILLER ,   1,1,1,  2,  0,  0,  0,  0,"S Triangle"},
	{GST lv1semi_data    ,FILLER ,   1,1,1,  2,  0,  0,  0,  0,"S Semicircle"},
	{GST lv2square_data  ,FILLER ,   2,1,1,  5,  0,  1,  0,  0,"M Armor"},
	{GST lv2triangle_data,FILLER ,   2,1,1,  5,  0,  1,  0,  0,"M Triangle"},
	{GST lv2semi_data    ,FILLER ,   2,1,1,  5,  0,  1,  0,  0,"M Semicircle"},
	{GST lv3square_data  ,FILLER ,   3,1,1,  9,  0,  2,  0,  0,"L Armor"},
	{GST lv3triangle_data,FILLER ,   3,1,1,  9,  0,  2,  0,  0,"L Triangle"},
	{GST lv3semi_data    ,FILLER ,   3,1,1,  9,  0,  2,  0,  0,"L Semicircle"},
/*   sptr                ,type   ,cost,w,h, hp,atk,def,agi,spd,name         */
	{GST wingshort_data  ,FILLER ,   1,1,1,  1,  0,  0,  1,  0,"S Wing"},
	{GST wingmed_data    ,FILLER ,   2,1,2,  1,  0,  0,  2,  0,"M Wing"},
	{GST winglong_data   ,FILLER ,   3,1,3,  1,  0,  0,  3,  0,"L Wing"},
	{GST enginesmall_data,ENGINE ,  10,1,1,  5,  0,  0,  1,  5,"S Engine"},
	{GST enginelarge_data,ENGINE ,  30,1,1, 10,  0,  0,  3, 10,"M Engine"},
	{GST turret1_data    ,WEAPON ,  10,1,1,  0,  1,  0,  0,  0,"S Turret"},
	{GST turret2_data    ,WEAPON ,  15,1,1,  1,  2,  0,  0,  0,"M Turret"},
	{GST turret3_data    ,WEAPON ,  20,1,1,  2,  4,  0,  0,  0,"L Turret"},
	{GST laser1_data     ,WEAPON ,  20,1,1,  0,  1,  0,  0,  0,"S Laser"},
	{GST laser2_data     ,WEAPON ,  25,1,1,  1,  2,  0,  0,  0,"M Laser"},
	{GST laser3_data     ,WEAPON ,  30,1,1,  2,  3,  0,  0,  0,"L Laser"},
	{GST missile1_data   ,WEAPON ,  40,2,1,  0,  2,  0,  0,  0,"S Missile"},
	{GST missile2_data   ,WEAPON ,  50,2,1,  1,  4,  0,  0,  0,"M Missile"},
	{GST missile3_data   ,WEAPON ,  60,2,1,  2,  8,  0,  0,  0,"L Missile"},
	
	
	{GST NULL,FILLER ,   1,1,1,  2,  0,  0,  0,  0,""},
	{GST NULL,FILLER ,   1,1,1,  2,  0,  0,  0,  0,""},
	{GST NULL,FILLER ,   1,1,1,  2,  0,  0,  0,  0,""},
	
};













