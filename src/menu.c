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

#include "gfx/out/gui_gfx.h"
#include "main.h"
#include "defs.h"
#include "util.h"
#include "menu.h"

int getLongestLength(char **sarr, uint8_t numstrings);
void drawMenuStrings(char **sarr, uint8_t numstrings,int xbase, uint8_t ybase, int width, uint8_t height, uint8_t index, uint8_t cbase);



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
		
		drawMenuStrings(sarr,numstrings,xbase,ybase,width,height,index,cbase);
		//Copy results to screen
		gfx_BlitRectangle(gfx_buffer,xbase,ybase,width,height);
		
		if (kd|kc) keywait();
	}
	return index;
}

//sarr is two pointers, one is to an input buffer. The entire dialog is assumed
//to be 140 characters wide to provide buffering. Assumes the buffer is 17 char
//wide to accomodate the zero-terminator at the end
//                 0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF
static char carr_ninp[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789./";
char *ninp_sarr[2] = {"Input new name",NULL};

uint8_t nameInput(char *s) {
	kb_key_t kc,kd;
	uint8_t i,charcursor,gridcursor,ybase,cbase,height,ytemp;
	int width,xbase,xtemp;
	char *inputbuf;
	
	inputbuf = ninp_sarr[1] = s;
	inputbuf[16] = 0;  //making sure the string is, in fact, zero-terminated
	//And then move the char cursor up to the end of any preexisting name
	for (charcursor=0;charcursor<16;charcursor++) if (0==inputbuf[charcursor]) break;
	if (16==charcursor) charcursor=15;
	
	width = (16*8+8); //4px margin, 16px per selectable. coincides 8px 16chr
	height = (16*8+8+10+16); //4px margin, 16px select, 16px head, 10px "opt"
	xbase = (LCD_WIDTH-width)/2;
	ybase = (LCD_HEIGHT-height)/2;
	cbase = 0x16; //A faded blue, set to darkest.
	gridcursor = 0;
	
	keywait();
	
	while (1) {
		kb_Scan();
		kd = kb_Data[7];
		kc = kb_Data[1];
		
		if (kc&kb_Mode) return 0;
		if (kc&kb_2nd) {
			if ((15==charcursor)||(63==gridcursor)) { inputbuf[charcursor+1] = 0; return 1; }
			inputbuf[charcursor] = carr_ninp[gridcursor];
			++charcursor;
		}
		if (kc&kb_Del && charcursor) {
			--charcursor;
			inputbuf[charcursor] = 0;
		}
		//Bitmask shenanigans
		if (kd&kb_Left)  gridcursor = ((gridcursor-1)&7)|(gridcursor&0x38);
		if (kd&kb_Right) gridcursor = ((gridcursor+1)&7)|(gridcursor&0x38);
		if (kd&kb_Up)    gridcursor = (gridcursor-8)&0x3F;
		if (kd&kb_Down)  gridcursor = (gridcursor+8)&0x3F;
		
		drawMenuStrings(ninp_sarr,2,xbase,ybase,width,height,0,cbase);
		
		gfx_SetTextFGColor(COLOR_WHITE);
		gfx_SetColor(cbase|COLOR_DARK);
		for (i=0;i<64;i++) {
			xtemp = xbase+(4+((i&7)<<4));
			ytemp = ybase+(10+16+4)+(((i&~7)&0xFF)<<1);
			
			if (i==gridcursor) gfx_FillRectangle_NoClip(xtemp,ytemp,16,16);
			if (63==i) {
				gfx_TransparentSprite_NoClip((gfx_sprite_t*)ninp_end_data,xtemp+3,ytemp+5);
			} else {
				gfx_SetTextXY(xtemp+4,ytemp+4);
				gfx_PrintChar(carr_ninp[i]);
			}
		}
		
		
		//Copy results to screen
		gfx_BlitRectangle(gfx_buffer,xbase,ybase,width,height);
		if (kd|kc) keywait();
	}
	
	
	
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
		drawMenuStrings(sarr,numstrings,xbase,ybase,width,height,0,cbase);
		//Copy results to screen
		gfx_BlitRectangle(gfx_buffer,xbase,ybase,width,height);

	} while (!(kd|kc));
	keywait();
	
}

void drawMenuStrings(char **sarr, uint8_t numstrings,int xbase, uint8_t ybase, int width, uint8_t height, uint8_t index, uint8_t cbase) {
	uint8_t i,ytemp;
	int xtemp;
	
	//Draw the menubox
	menuRectangle(xbase,ybase,width,height,cbase);
	//Draw header area
	gfx_SetColor(cbase|COLOR_LIGHTER);
	gfx_HorizLine(xbase+6,ybase+13,width-12);
	gfx_SetTextFGColor(0x3C|COLOR_LIGHT);    //Picked using color contrast tool
	gfx_PrintStringXY(sarr[0],xbase+4,ybase+3);  //Header
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






