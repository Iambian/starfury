/* Cross file declaration(s) for the ship editor */

#ifndef __enemy_cutil__
#define __enemy_cutil__

#include <stdint.h>
#include <graphx.h>		//Provides typedefs for graphx objects
#include "defs.h"





//Definitely export these
void parseEnemy(enemy_obj *eobj,uint8_t **scriptPtr);
enemy_obj *findEmptyEnemy(void);
enemy_obj *generateEnemy(enemy_data *edat);

//Maybe export these?
void enShot1(enemy_obj *eobj,uint8_t angle);


//Probably export these
extern uint8_t testEnemyScript[];
extern enemy_data testEnemyData;








#endif