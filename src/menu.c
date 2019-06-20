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
#include <fileioc.h>

#include "main.h"
#include "defs.h"
#include "util.h"
#include "menu.h"

int getLongestLength(char **sarr, uint8_t numstrings);





//Header start #DFEA09 Header end: #E0E020 (0x3C+COLOR_LIGHTER) [yellowish]
//Note: Can't do sizeof(sarr)/sizeof(sarr[0]) because that information isn't
//passed along to the function from outside this scope. It is simply seen as
//a pointer to a bunch of other pointers, not an array of pointers. Sadness.

//Blocking routine, draws centered menu. Returns 1-numstrings or 0 if cancel.
//sarr has header string followed by strings showing options
//numstrings is total length of sarr, including header string.
uint8_t staticMenu(char **sarr,uint8_t numstrings) {
	kb_key_t kc,kd;
	int width,xbase,tempx,strwidth;
	uint8_t i,height,ybase,cbase,index,tempy;
	
	width = getLongestLength(sarr,numstrings)+8;
	height = (4+(numstrings-1)*10+16); //Border 4px, header 16px, others 10px
	xbase = (LCD_WIDTH-width)/2;
	ybase = (LCD_HEIGHT-height)/2;
	cbase = 0x19; //A faded green, set to darkest.
	index = 1;
	
	keywait();
	
	while (1) {
		kb_Scan();
		kd = kb_Data[7];
		kc = kb_Data[1];
		
		menuRectangle(xbase,ybase,width,height,cbase);
		gfx_SetColor(cbase|COLOR_LIGHTER);
		//THICC horizontal divider line between header and menu options
		for (i=0;i<3;i++) gfx_HorizLine(xbase+6,ybase+12+i,width-12);
		gfx_SetTextFGColor(0x3C|COLOR_LIGHT);    //Picked using color contrast tool
		gfx_PrintStringXY(sarr[0],xbase+2,ybase+3);  //Header
		
		if (kc&kb_Mode) { index = 0; break;}
		if (kc&kb_2nd) break;
		if ((kd&kb_Up) && (!--index)) index = numstrings-1;
		if ((kd&kb_Down) && (++index == numstrings)) index = 1;
		
		tempx = xbase+2;
		tempy = ybase+16;
		gfx_SetTextFGColor(COLOR_WHITE|COLOR_LIGHTER);
		gfx_SetColor(cbase|COLOR_DARK);
		for (i=1;i<numstrings;i++) {
			if (i==index) gfx_FillRectangle_NoClip(tempx,tempy,width-4,10);
			gfx_PrintStringXY(sarr[i],tempx+(width-gfx_GetStringWidth(sarr[i]))/2,tempy+1);
			tempy += 10;
		}
		
		gfx_BlitRectangle(gfx_buffer,xbase,ybase,width,height);
		
		if (kd|kc) keywait();
	}
	return index;
}
void alert(char *s) {
	
}



int getLongestLength(char **sarr, uint8_t numstrings) {
	int largest_width,current_width;
	largest_width = 0;
	do {
		--numstrings;
		if ((current_width = gfx_GetStringWidth(sarr[numstrings])) > largest_width)
			largest_width = current_width;
	} while (numstrings);
	return largest_width;
}

void menuRectangle(int x,uint8_t y,int w, uint8_t h, uint8_t basecolor) {
	gfx_SetColor(basecolor|COLOR_LIGHTER);
	gfx_Rectangle_NoClip(x,y,w,h);
	gfx_SetColor(basecolor|COLOR_DARK);
	gfx_Rectangle_NoClip(++x,++y,w-=2,h-=2);
	gfx_SetColor(((basecolor>>1)&0x15)|COLOR_DARK); //darkshift, lighter
	gfx_FillRectangle(++x,++y,w-=2,h-=2);
}






