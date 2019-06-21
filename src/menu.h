/* Declaration(s) for the menu system */

#ifndef __menu_cutil__
#define __menu_cutil__

#include <stdint.h>


uint8_t staticMenu(char **sarr,uint8_t numstrings);
void alert(char **sarr,uint8_t numstrings);
void menuRectangle(int x,uint8_t y,int w, uint8_t h, uint8_t basecolor);

#endif