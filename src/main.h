#ifndef __main_cfile__
#define __main_cfile__

#include <stdint.h>

extern gfx_sprite_t *tempblock_scratch;
extern gfx_sprite_t *tempblock_smallscratch;

void waitanykey(void);
void keywait(void);
void error(char *msg);


#endif
