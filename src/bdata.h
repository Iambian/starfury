#ifndef __main_bdata__
#define __main_bdata__

#include <stdint.h>
#include "defs.h"


void load_blockdata(void);  /* Loads in block data to memory */
extern blockprop_obj blockobject_list[];

extern blueprint_obj basic_blueprint;
extern gridblock_obj basic_blueprint_grid[];
extern blueprint_obj *builtin_blueprints[];








#endif
