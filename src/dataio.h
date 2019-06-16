#ifndef __dataio_cfile__
#define __dataio_cfile__

#include <stdint.h>
#include <graphx.h>		//Provides typedefs for graphx objects
#include <fileioc.h>
#include "defs.h"


typedef struct gamedata_struct {
	uint8_t file_version;
	uint8_t gridlevel;
	uint8_t blueprints_owned;  //A bit field. Up to 8 default blueprints
	uint8_t default_color;
	int credits_owned;
	uint8_t custom_blueprints_owned; //Number. Up to 7 custom.
	uint8_t reserved[255];     //Allowed to overflow slightly
} gamedata_t;

/* Variables */

extern uint8_t *inventory;
extern blueprint_obj *curblueprint;
extern gamedata_t gamedata;
extern blueprint_obj temp_blueprint;
extern gridblock_obj temp_bpgrid[];

extern blueprint_obj empty_blueprint;

/* Functions */

void initPlayerData(void);


ti_var_t openSaveReader(void);
void saveGameData(void);
void saveBlueprint(uint8_t bpslot);
void loadBlueprint(uint8_t bpslot);
void loadBuiltinBlueprint(uint8_t bpslot);
void setMinimalInventory(void);
void normalizeBlueprint(void);


#endif