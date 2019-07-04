#ifndef __main_cfile__
#define __main_cfile__

#include <stdint.h>
#include <graphx.h>
#include "defs.h"


//Move these out to file after done testing

field_obj *findEmptyFieldObject(void);
void smallShotMove(field_obj *fobj);

extern field_obj *fobjs;
extern enemy_obj *eobjs;
extern weapon_obj *wobjs;
extern field_obj  empty_fobj;
extern enemy_obj  empty_eobj;
extern weapon_obj empty_wobj;
extern enemy_data empty_edat;




//Keep these in here
extern const uint8_t transparent_color;

extern gfx_sprite_t *mainsprite;
extern gfx_sprite_t *altsprite;
extern gfx_sprite_t *tempblock_scratch;
extern gfx_sprite_t *tempblock_smallscratch;

void waitanykey(void);
void keywait(void);
void error(char *msg);


#endif
