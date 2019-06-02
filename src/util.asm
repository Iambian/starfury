XDEF _fn_DrawNestedSprite
XDEF _fn_FillSprite


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
