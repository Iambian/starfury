#ifndef __main_bdata__
#define __main_bdata__

#include <stdint.h>
#include "defs.h"

#define BLOCK_CIC1	1
#define BLOCK_CIC2	2
#define BLOCK_CIC3	3 
#define BLOCK_SQR1	4
#define BLOCK_TRI1	5
#define BLOCK_SEM1	6
#define BLOCK_SQR2	7
#define BLOCK_TRI2	8
#define BLOCK_SEM2	9

#define BLOCK_SQR3	10
#define BLOCK_TRI3	11
#define BLOCK_SEM3	12
#define BLOCK_WINS	13
#define BLOCK_WINM	14
#define BLOCK_WINL	15
#define BLOCK_ENGS	16
#define BLOCK_ENGL	17
#define BLOCK_TUR1	18
#define BLOCK_TUR2	19

#define BLOCK_TUR3	20
#define BLOCK_LAS1	21
#define BLOCK_LAS2	22
#define BLOCK_LAS3	23
#define BLOCK_MIS1	24
#define BLOCK_MIS2	25
#define BLOCK_MIS3	26

void load_blockdata(void);  /* Loads in block data to memory */
extern blockprop_obj blockobject_list[];

extern blueprint_obj basic_blueprint;
extern gridblock_obj basic_blueprint_grid[];
extern blueprint_obj *builtin_blueprints[];








#endif
