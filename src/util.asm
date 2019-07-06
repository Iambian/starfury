XDEF _fn_DrawNestedSprite
XDEF _fn_FillSprite
XDEF _fn_Setup_Palette
XDEF _fn_PaintSprite
XDEF _fn_InvertSprite
XDEF _fn_SetupTimer
XDEF _fn_SetNewTimerValue
XDEF _fn_ReadTimer
XDEF _fn_CheckTimer
XDEF _fn_StopTimer


XREF _transparent_color

.ASSUME ADL=1

;in:	arg0=smallSpritePtr, arg1=bigSpritePtr, arg2=Xpos, arg3=Ypos
;       +3                   +6                 +9         +12        +15
_fn_DrawNestedSprite:
	LD	IY,0
	ADD	IY,SP
	LD	HL,(IY+6)  ;bigspr
	LD	A,(HL)
	LD	E,A        ;get bigsprite width
	LD	D,(IY+12)  ;ypos
	MLT	DE         ;get y offset into bigsprite. Clears UDE
	INC	HL
	INC	HL         ;advance to start of bigsprite
	ADD	HL,DE      ;move to correct row of bigsprite
	LD	D,0
	LD	E,(IY+9)   ;xpos
	PUSH DE
	POP BC         ;pre-clear upper bytes of BC
	LD (IY+15),DE  ;pre-clear upper bytes in SKIPCOUNT (+15)
	ADD	HL,DE      ;move to correct column of bigsprite
	EX	DE,HL     ;bigsprite to DE, HL now clear
	LD	HL,(IY+3) ;load smallsprite
	SUB	A,(HL)    ;get skip distance to next row by bigsprw-smalsprw
	LD	(IY+15),A ;set skip value
	LD	C,(HL)    ;BC = width of smallsprite
	INC	HL
	LD	A,(HL)    ;A = height of smallsprite
	INC	HL
	;HL=smolsprt (src), DE=bigsprt (dest), BC=bytesInRow, A=height
drawNestedSpriteLoop:
	PUSH BC
		LDIR
		EX	DE,HL
		LD	BC,(IY+15)
		ADD	HL,BC ;skip to next row of bigsprite
		EX	DE,HL
	POP	BC
	DEC	A
	JR	NZ,drawNestedSpriteLoop
	RET
		
;in:	arg0=spriteAdr arg1=color
;       +3             +6
_fn_FillSprite:
	POP	BC  ;RETURN ADDRESS
	POP	HL  ;SPRITE ADDRESS
	POP DE  ;SET TO THIS COLOR
	PUSH DE
	PUSH HL
	PUSH BC ;REWIND STACK
	LD	C,(HL)
	INC	HL
	LD	B,(HL)
	INC	HL
	MLT	BC  ;W*H = SIZE TO WRITE OUT
	DEC	BC  ;LESS ONE FOR INITIAL WRITE
	LD	(HL),E  ;SET INITIAL COLOR
	PUSH HL
	POP DE
	INC DE
	LDIR	;COPY COLOR TO WHOLE SPRITE
	RET

;in: No inputs
_fn_Setup_Palette:
	LD HL,0E30019h
	RES 0,(HL)       ;Reset BGR bit to make our mapping correct
	LD	BC,0
	LD	IY,0E30200h  ;Address of palette
;palette index format: IIRRGGBB palette entry: IBBBBBGG GGGRRRRR
setupPaletteLoop:
	LD	HL,0
	;PROCESS BLUE. TARGET 0bbii0--
	LD	A,B
	RRCA             ;BIIRRGGB
	LD E,A           ;Keep for red processing
	RRCA             ;BBIIRRGG
	LD	C,A          ;Keep for green processing
	RRCA             ;GBBIIRRG
	AND	A,01111000b  ;0BBII000
	LD	H,A          ;Blue set.
	;PROCESS GREEN. TARGET ii0000gg, MASK LOW NIBBLE INTO HIGH BYTE
	LD A,C           ;BBIIRRGG
	XOR	H            ;xxxxxxyy
	AND	A,00000011b  ;keep low bits to mask back to original
	XOR	H            ;0BBII0GG
	LD	H,A          ;Green high set (------GG)
	LD	L,B          ;Green low set  (II------)
	;PROCESS RED. TARGET 000rrii0
	LD	A,B          ;IIRRGGBB
	RLCA             ;IRRGGBBI
	RLCA             ;RRGGBBII
	RLCA             ;RGGBBIIR
	XOR	E            ;-----xx-
	AND	A,00000110b
	XOR	E            ;biiRRIIb
	XOR L            ;---xxxx-
	AND A,00011110b
	XOR	L            ;IIxRRIIx
	AND	A,11011110b  ;II0RRII0
	LD	L,A
	LD	(IY+0),HL
	LEA IY,IY+2
	INC B
	JR NZ,setupPaletteLoop
	RET

;in:	arg0=spriteAdr, arg1=color
;       +3              +6        
_fn_PaintSprite:
	POP	BC  ;RETURN ADDRESS
	POP	HL  ;SPRITE ADDRESS
	POP DE  ;PAINT WITH THIS COLOR (E)
	SET	6,E
	SET	7,E ;SET INTENSITY BITS AS WE ARE AND'ING TO THE WHITE COLOR TO UNSET.
	PUSH DE
	PUSH HL
	PUSH BC ;REWIND STACK
	LD	C,(HL)
	INC	HL
	LD	B,(HL)
	INC	HL
	MLT	BC  ;W*H = SIZE TO COPY WITH
paintSpriteLoop:
	LD	A,(HL)
	OR	A,11000000b
	INC	A   ;Looking for code 00111111b. w/ prev op, will set Z if this is so
	JR	NZ,paintSpriteSkip
	LD	A,(HL)
	AND	E
	LD	(HL),A
paintSpriteSkip:
	INC	HL
	DEC	BC
	LD	A,C
	OR	A,B
	JR	NZ,paintSpriteLoop
	RET
	
TimerValue:
DB 0,0,1
;30fps = 32768/30 ~~ 1092.2
NEW_TIMER_VALUES: 
DB 00h,00h,00h,00h
DB 0FFh,0FFh,0FFh,0FFh
NEW_TIMER_VALUES_END:

;in:	arg0=spriteAdr
;       +3            
_fn_InvertSprite:
	POP	BC  ;RETURN ADDRESS
	POP	HL  ;SPRITE ADDRESS
	PUSH HL
	PUSH BC ;REWIND STACK
	LD	C,(HL)
	INC	HL
	LD	B,(HL)
	INC	HL
	LD	A,(_transparent_color)
	LD	E,A
fn_inverspriteloop:
	LD	A,(HL)
	CP	E
	JR	Z,fn_invertsprite_skip
	CPL
	LD	(HL),A
fn_invertsprite_skip:
	INC	HL
	DEC BC
	LD	A,C
	OR	A,B
	JR NZ,fn_inverspriteloop
	RET


_fn_StopTimer:
	LD	IY,0F20000h
	XOR	A,A
	SBC HL,HL
	LD	(IY+30h),HL   ;TCTRLR - RESET ALL BITS, TURNING OFF TIMERS
	RET

;in:	arg0=timerResetValue
;       +3  
_fn_SetupTimer:
	CALL _fn_StopTimer
	LEA	DE,IY+0
	LD	HL,NEW_TIMER_VALUES
	LD	BC,NEW_TIMER_VALUES_END-NEW_TIMER_VALUES
	LDIR
	LD	HL,1092
	LD	(TimerValue),HL
	LD	(IY+30h),00000011b  ;timer1 on, timer1 src xtal
	RET
	
_fn_SetNewTimerValue:
	POP	BC
	POP	HL
	PUSH	HL
	PUSH	BC
	LD	(TimerValue),HL
	RET

;returns timer as int	
_fn_ReadTimer:
	LD HL,(0F20000h)
	RET
	
;return value: 0=fine, nonzero=underflowed and reset
_fn_CheckTimer:
	LD	IY,0F20000h
	XOR	A,A
	SBC	HL,HL
	BIT	7,(IY+3)    ;check bit 31 of timer 1 value
	RET	Z           ;no underflow. return now.
	LD 	(IY+30h),A  ;turn off timers before beginning to modify them
	LD	DE,(TimerValue)
	LD	HL,(IY+0)
checkTimerLoop:
	ADD	HL,DE     ;account for lost time by refilling underflow by time interval
	JR	NC,checkTimerLoop   ;but keep looping if we're that far in the hole
	LD	(IY+0),HL   ;then set the (for sure) positive value
	LD	(IY+3),A    
	INC	A
	SBC	HL,HL       ;results in 0xFFFFFF.
	LD	(IY+30h),00000011b  ;timer1 on, timer1 src xtal
	RET

	

	
	
	




