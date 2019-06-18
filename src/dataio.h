#ifndef __dataio_cfile__
#define __dataio_cfile__

#include <stdint.h>
#include <graphx.h>		//Provides typedefs for graphx objects
#include <fileioc.h>
#include "defs.h"


/* ------------------------- Structs and typedefs ------------------------- */
typedef struct gamedata_struct {
	uint8_t file_version;
	uint8_t gridlevel;
	uint8_t blueprints_owned;  //A bit field. Up to 8 default blueprints
	uint8_t default_color;
	int credits_owned;
	uint8_t custom_blueprints_owned; //Number. Up to 7 custom.
	uint8_t reserved[255];     //Allowed to overflow slightly
} gamedata_t;

typedef struct stats_sum_struct {
	int hp;
	int power;  //Add to only when command or energy modules found
	int weight; //Add to in every other instance
	int atk;
	int def;
	int spd;
	int agi;
	uint8_t wpn; //Weapon count
	uint8_t cmd;  //Command module count
} stats_sum;

/* ------------------------- Variables ------------------------- */
extern blueprint_obj empty_blueprint;  //Convenience object for sizeof
extern gridblock_obj empty_gridblock;  //Convenience object for sizeof
extern stats_sum bpstats;

extern uint8_t *inventory;
extern blueprint_obj *curblueprint;
extern gamedata_t gamedata;
extern blueprint_obj temp_blueprint;
extern gridblock_obj temp_bpgrid[];

/* ------------------------- Functions ------------------------- */
void initPlayerData(void);

ti_var_t openSaveReader(void);
void saveGameData(void);
void saveBlueprint(uint8_t bpslot);
void loadBlueprint(uint8_t bpslot);
void loadBuiltinBlueprint(uint8_t bpslot);
void resetBlueprint(void);
uint8_t createNewBlueprint(void);
void setMinimalInventory(void);
void normalizeBlueprint(void);
void addGridBlock(gridblock_obj *gbo);
void sumStats(void);
void drawShipPreview(gfx_sprite_t *sprite);

#endif