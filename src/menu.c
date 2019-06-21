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
void drawMenuStrings(char **sarr, uint8_t numstrings,int xbase, uint8_t ybase, int width, uint8_t index, uint8_t cbase);




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
		
		if (kc&kb_Mode) { index = 0; break;}
		if (kc&kb_2nd) break;
		if ((kd&kb_Up) && (!--index)) index = numstrings-1;
		if ((kd&kb_Down) && (++index == numstrings)) index = 1;
		
		drawMenuStrings(sarr,numstrings,xbase,ybase,width,index,cbase);
		
		if (kd|kc) keywait();
	}
	return index;
}

//We don't have newlines so we've got to do it via array of strings.
//sarr is structured exactly like menus, except there are no decisions
//and any key pressed will close the notice
void alert(char **sarr,uint8_t numstrings) {
	kb_key_t kc,kd;
	int width,xbase,tempx,strwidth;
	uint8_t i,height,ybase,cbase,index,tempy;
	
	width = getLongestLength(sarr,numstrings)+8;
	height = (4+(numstrings-1)*10+16); //Border 4px, header 16px, others 10px
	xbase = (LCD_WIDTH-width)/2;
	ybase = (LCD_HEIGHT-height)/2;
	cbase = 0x25; //A faded red, set to darkest.
	
	keywait();
	do {
		kb_Scan();
		kd = kb_Data[7];
		kc = kb_Data[1];
		drawMenuStrings(sarr,numstrings,xbase,ybase,width,0,cbase);
	} while (!(kd|kc));
	keywait();
	
}

void drawMenuStrings(char **sarr, uint8_t numstrings,int xbase, uint8_t ybase, int width, uint8_t index, uint8_t cbase) {
	uint8_t i,height,ytemp;
	int xtemp;
	
	height = (4+(numstrings-1)*10+16); //Border 4px, header 16px, others 10px
	
	//Draw the menubox
	menuRectangle(xbase,ybase,width,height,cbase);
	//Draw header area
	gfx_SetColor(cbase|COLOR_LIGHTER);
	gfx_HorizLine(xbase+6,ybase+13,width-12);
	gfx_SetTextFGColor(0x3C|COLOR_LIGHT);    //Picked using color contrast tool
	gfx_PrintStringXY(sarr[0],xbase+2,ybase+3);  //Header
	//Draw menu options
	xtemp = xbase+2;
	ytemp = ybase+16;
	gfx_SetTextFGColor(COLOR_WHITE|COLOR_LIGHTER);
	gfx_SetColor(cbase|COLOR_DARK);
	for (i=1;i<numstrings;i++) {
		if (i==index) gfx_FillRectangle_NoClip(xtemp,ytemp,width-4,10);
		gfx_PrintStringXY(sarr[i],xtemp+(width-gfx_GetStringWidth(sarr[i]))/2-2,ytemp+1);
		ytemp += 10;
	}
	//Copy results to screen
	gfx_BlitRectangle(gfx_buffer,xbase,ybase,width,height);
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






