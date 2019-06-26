#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <graphx.h>

#include "defs.h"
#include "bdata.h"
#include "gfx/out/blox_gfx.h"


#define GST (gfx_sprite_t*)

blockprop_obj blockobject_list[] = {
/*   sptr                ,type   ,cost,w,h, hp,atk,def,agi,spd,name         */
	{GST NULL            ,FILLER , 255,1,1,255,255,255,255,255,"Empty block"},
	{GST lv1cic_data     ,COMMAND,  80,2,2, 20,  1,  0,  0,  0,"S Cockpit"},
	{GST lv2cic_data     ,COMMAND, 120,3,2, 40,  3,  5,  0,  0,"M Cockpit"},
	{GST lv3cic_data     ,COMMAND, 200,3,2, 60,  7, 10,  0,  0,"L Cockpit"},
	{GST lv1square_data  ,FILLER ,   1,1,1,  2,  0,  0,  0,  0,"S Armor"},
	{GST lv1triangle_data,FILLER ,   1,1,1,  2,  0,  0,  0,  0,"S Triangle"},
	{GST lv1semi_data    ,FILLER ,   1,1,1,  2,  0,  0,  0,  0,"S Semicircle"},
	{GST lv2square_data  ,FILLER ,   2,1,1,  5,  0,  1,  0,  0,"M Armor"},
	{GST lv2triangle_data,FILLER ,   2,1,1,  5,  0,  1,  0,  0,"M Triangle"},
	{GST lv2semi_data    ,FILLER ,   2,1,1,  5,  0,  1,  0,  0,"M Semicircle"},
/*   sptr                ,type   ,cost,w,h, hp,atk,def,agi,spd,name         */
	{GST lv3square_data  ,FILLER ,   3,1,1,  9,  0,  2,  0,  0,"L Armor"},
	{GST lv3triangle_data,FILLER ,   3,1,1,  9,  0,  2,  0,  0,"L Triangle"},
	{GST lv3semi_data    ,FILLER ,   3,1,1,  9,  0,  2,  0,  0,"L Semicircle"},
	{GST wingshort_data  ,FILLER ,   1,1,1,  1,  0,  0,  1,  0,"S Wing"},
	{GST wingmed_data    ,FILLER ,   2,1,2,  1,  0,  0,  2,  0,"M Wing"},
	{GST winglong_data   ,FILLER ,   3,1,3,  1,  0,  0,  3,  0,"L Wing"},
	{GST enginesmall_data,ENGINE ,  10,1,1,  5,  0,  0,  1,  5,"S Engine"},
	{GST enginelarge_data,ENGINE ,  30,2,2, 10,  0,  0,  3, 10,"M Engine"},
	{GST turret1_data    ,WEAPON ,  10,1,1,  0,  1,  0,  0,  0,"S Turret"},
	{GST turret2_data    ,WEAPON ,  15,1,1,  1,  2,  0,  0,  0,"M Turret"},
/*   sptr                ,type   ,cost,w,h, hp,atk,def,agi,spd,name         */
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
/*   sptr                ,type   ,cost,w,h, hp,atk,def,agi,spd,name         */



	
};

gridblock_obj basic_blueprint_grid[] = {
//	blockid,    xx,yy,color,            orientation
	{BLOCK_CIC1, 6, 3,COLOR_YELLOW     ,ROT_1      },
	{BLOCK_SQR1, 2, 2,COLOR_BLUE       ,ROT_0      },
	{BLOCK_SQR1, 3, 2,COLOR_CYAN       ,ROT_0      },
	{BLOCK_SQR1, 5, 3,COLOR_PURPLE     ,ROT_0      },
	{BLOCK_SQR1, 4, 3,COLOR_MAGENTA    ,ROT_0      },
	{BLOCK_SQR1, 3, 3,COLOR_WHITE      ,ROT_0      },
	{BLOCK_SQR1, 2, 3,COLOR_GRAY       ,ROT_0      },
	{BLOCK_SQR1, 5, 4,COLOR_DARKGRAY   ,ROT_0      },
	{BLOCK_SQR1, 4, 4,COLOR_NAVY       ,ROT_0      },
	{BLOCK_SQR1, 3, 4,COLOR_GREEN      ,ROT_0      },
	{BLOCK_SQR1, 2, 4,COLOR_GREEN      ,ROT_0      },
	{BLOCK_SQR1, 2, 5,COLOR_GREEN      ,ROT_0      },
	{BLOCK_SQR1, 3, 5,COLOR_GREEN      ,ROT_0      },
	{BLOCK_WINS, 4, 2,COLOR_GREEN      ,ROT_0      },
	{BLOCK_WINS, 4, 5,COLOR_GREEN      ,ROT_1      },
	{BLOCK_WINM, 2, 1,COLOR_GREEN      ,ROT_3|HFLIP},
	{BLOCK_WINM, 2, 6,COLOR_GREEN      ,ROT_1      },
	{BLOCK_ENGS, 1, 3,COLOR_LIME       ,ROT_0      },
	{BLOCK_ENGS, 1, 4,COLOR_LIME       ,ROT_0      },
	{BLOCK_TUR1, 5, 2,COLOR_MAROON     ,ROT_0      },
	{BLOCK_TUR1, 5, 5,COLOR_MAROON     ,ROT_2|HFLIP},
};

blueprint_obj basic_blueprint = {
	GRID_LEVEL_1,
	sizeof basic_blueprint_grid / sizeof basic_blueprint_grid[0],
	"Refurbished Ship",
	basic_blueprint_grid,
};



/*	Up to 8 built-in blueprints can be used. See enum BLPT for bitmasks and
	positions that it indicates for each. Where unused, use basic_blueprint */
blueprint_obj *builtin_blueprints[] = {
	&basic_blueprint,
	&basic_blueprint,
	&basic_blueprint,
	&basic_blueprint,
	&basic_blueprint,
	&basic_blueprint,
	&basic_blueprint,
	&basic_blueprint,
};




