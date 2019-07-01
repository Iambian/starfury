/* Cross file declaration(s) for the ship editor */

#ifndef __enemy_cutil__
#define __enemy_cutil__

#include <stdint.h>
#include <graphx.h>		//Provides typedefs for graphx objects
#include "defs.h"






void parseEnemy(enemy_obj *eobj,uint8_t **scriptPtr);




#endif