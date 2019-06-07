/* Declarations for the assembly routines found in util.asm */

#ifndef __defs_cutil__
#define __defs_cutil__

#include <stdint.h>
#include <graphx.h>		//Provides typedefs for graphx objects

void fn_DrawNestedSprite(gfx_sprite_t *smSpr, gfx_sprite_t *lgSpr, uint8_t x, uint8_t y);
void fn_FillSprite(gfx_sprite_t *sprite, uint8_t color);
void fn_Setup_Palette(void);
void fn_PaintSprite(gfx_sprite_t *sprite, uint8_t color); //Turns all white-ish to color





#endif