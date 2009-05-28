
//**************************************************************************
//**
//** sv_save.c
//**
//** $Revision$
//** $Date$
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "h2stdinc.h"
#include "doomdef.h"
#include "p_local.h"

// MACROS ------------------------------------------------------------------

#define SVG_RAM			0
#define SVG_FILE		1
#define SAVEGAMESIZE		0x30000
#define SAVE_GAME_TERMINATOR	0x1d

// TYPES -------------------------------------------------------------------

typedef enum
{
	tc_end,
	tc_mobj
} thinkerclass_t;

enum
{
	tc_ceiling,
	tc_door,
	tc_floor,
	tc_plat,
	tc_flash,
	tc_strobe,
	tc_glow,
	tc_endspecials
} specials_e;

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void ArchivePlayers(void);
static void UnarchivePlayers(void);
static void ArchiveWorld(void);
static void UnarchiveWorld(void);
static void ArchiveThinkers(void);
static void UnarchiveThinkers(void);
static void ArchiveSpecials(void);
static void UnarchiveSpecials(void);
static void OpenStreamOut(const char *fileName);
static void CloseStreamOut(const char *fileName);
static void StreamOutBuffer(const void *buffer, int size);
static void StreamOutByte(byte val);
static void StreamOutWord(unsigned short val);
static void StreamOutLong(unsigned int val);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static FILE *SavingFP;
static int SaveGameType;
static void *SaveBuffer;
static byte *SavePtr;

// CODE --------------------------------------------------------------------

//==========================================================================
//
// SV_SaveGame
//
//==========================================================================

void SV_SaveGame(int slot, const char *description)
{
	int i;
	char fileName[MAX_OSPATH];
	char verString[SAVEVERSIONSIZE];

	snprintf(fileName, sizeof(fileName), "%s%s%d.hsg",
		 basePath, SAVEGAMENAME, slot);
	OpenStreamOut(fileName);
	StreamOutBuffer(description, SAVESTRINGSIZE);
	memset(verString, 0, sizeof(verString));
	sprintf(verString, "version %i", VERSION);
	StreamOutBuffer(verString, SAVEVERSIONSIZE);
	StreamOutByte(gameskill);
	StreamOutByte(gameepisode);
	StreamOutByte(gamemap);
	for (i = 0; i < MAXPLAYERS; i++)
	{
		StreamOutByte(playeringame[i]);
	}
	StreamOutByte(leveltime>>16);
	StreamOutByte(leveltime>>8);
	StreamOutByte(leveltime);
	ArchivePlayers();
	ArchiveWorld();
	ArchiveThinkers();
	ArchiveSpecials();
	CloseStreamOut(fileName);
}

//==========================================================================
//
// SV_LoadGame
//
//==========================================================================

void SV_LoadGame(int slot)
{
	int i;
	int a, b, c;
	char fileName[MAX_OSPATH];
	char vcheck[SAVEVERSIONSIZE];

	snprintf(fileName, sizeof(fileName), "%s%s%d.hsg",
			 basePath, SAVEGAMENAME, slot);
	M_ReadFile(fileName, &SaveBuffer);
	SavePtr = (byte *)SaveBuffer + SAVESTRINGSIZE;	// Skip the description field
	memset(vcheck, 0, sizeof(vcheck));
	sprintf(vcheck, "version %i", VERSION);
	if (strcmp((char *)SavePtr, vcheck) != 0)
	{ // Bad version
		return;
	}
	SavePtr += SAVEVERSIONSIZE;
	gameskill = *SavePtr++;
	gameepisode = *SavePtr++;
	gamemap = *SavePtr++;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		playeringame[i] = *SavePtr++;
	}
	// Load a base level
	G_InitNew(gameskill, gameepisode, gamemap);

	// Create leveltime
	a = *SavePtr++;
	b = *SavePtr++;
	c = *SavePtr++;
	leveltime = (a<<16) + (b<<8) + c;

	// De-archive all the modifications
	UnarchivePlayers();
	UnarchiveWorld();
	UnarchiveThinkers();
	UnarchiveSpecials();

	if (*SavePtr != SAVE_GAME_TERMINATOR)
	{ // Missing savegame termination marker
		I_Error("Bad savegame");
	}
	Z_Free(SaveBuffer);
}

//==========================================================================
//
// ArchivePlayers
//
//==========================================================================

static void ArchivePlayers(void)
{
	int i, j;
	player_t dest;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
		{
			continue;
		}
		memcpy(&dest, &players[i], sizeof(player_t));
		for (j = 0; j < NUMPSPRITES; j++)
		{
			if (dest.psprites[j].state)
			{
				dest.psprites[j].state =
					(state_t *)(dest.psprites[j].state-states);
			}
		}
		StreamOutBuffer(&dest, sizeof(player_t));
	}
}

//==========================================================================
//
// UnarchivePlayers
//
//==========================================================================

static void UnarchivePlayers(void)
{
	int i, j;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;
		memcpy(&players[i],SavePtr, sizeof(player_t));
		SavePtr += sizeof(player_t);
		players[i].mo = NULL;		// will be set when unarc thinker
		players[i].message = NULL;
		players[i].attacker = NULL;
		for (j = 0; j < NUMPSPRITES; j++)
		{
			if (players[i]. psprites[j].state)
				players[i]. psprites[j].state = &states[(int)players[i].psprites[j].state];
		}
	}
}

//==========================================================================
//
// ArchiveWorld
//
//==========================================================================

static void ArchiveWorld(void)
{
	int i, j;
	sector_t *sec;
	line_t *li;
	side_t *si;

	// Sectors
	for (i = 0, sec = sectors; i < numsectors; i++, sec++)
	{
		StreamOutWord(sec->floorheight>>FRACBITS);
		StreamOutWord(sec->ceilingheight>>FRACBITS);
		StreamOutWord(sec->floorpic);
		StreamOutWord(sec->ceilingpic);
		StreamOutWord(sec->lightlevel);
		StreamOutWord(sec->special);	// needed?
		StreamOutWord(sec->tag);		// needed?
	}

	// Lines
	for (i = 0, li = lines; i < numlines; i++, li++)
	{
		StreamOutWord(li->flags);
		StreamOutWord(li->special);
		StreamOutWord(li->tag);
		for (j = 0; j < 2; j++)
		{
			if (li->sidenum[j] == -1)
			{
				continue;
			}
			si = &sides[li->sidenum[j]];
			StreamOutWord(si->textureoffset>>FRACBITS);
			StreamOutWord(si->rowoffset>>FRACBITS);
			StreamOutWord(si->toptexture);
			StreamOutWord(si->bottomtexture);
			StreamOutWord(si->midtexture);
		}
	}
}

//==========================================================================
//
// UnarchiveWorld
//
//==========================================================================

static void UnarchiveWorld(void)
{
	int i, j;
	sector_t *sec;
	line_t *li;
	side_t *si;
	short *get;

	get = (short *)SavePtr;

//
// do sectors
//
	for (i = 0, sec = sectors; i < numsectors; i++, sec++)
	{
		sec->floorheight = *get++ << FRACBITS;
		sec->ceilingheight = *get++ << FRACBITS;
		sec->floorpic = *get++;
		sec->ceilingpic = *get++;
		sec->lightlevel = *get++;
		sec->special = *get++;	// needed?
		sec->tag = *get++;	// needed?
		sec->specialdata = 0;
		sec->soundtarget = 0;
	}

//
// do lines
//
	for (i = 0, li = lines; i < numlines; i++, li++)
	{
		li->flags = *get++;
		li->special = *get++;
		li->tag = *get++;
		for (j = 0; j < 2; j++)
		{
			if (li->sidenum[j] == -1)
				continue;
			si = &sides[li->sidenum[j]];
			si->textureoffset = *get++ << FRACBITS;
			si->rowoffset = *get++ << FRACBITS;
			si->toptexture = *get++;
			si->bottomtexture = *get++;
			si->midtexture = *get++;
		}
	}

	SavePtr = (byte *)get;
}

//==========================================================================
//
// ArchiveThinkers
//
//==========================================================================

static void ArchiveThinkers(void)
{
	thinker_t *th;
	mobj_t mobj;

	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function == P_MobjThinker)
		{
			StreamOutByte(tc_mobj);
			memcpy(&mobj, th, sizeof(mobj_t));
			mobj.state = (state_t *)(mobj.state-states);
			if (mobj.player)
			{
				mobj.player = (player_t *)((mobj.player-players)+1);
			}
			StreamOutBuffer(&mobj, sizeof(mobj_t));
			continue;
		}
		//I_Error("P_ArchiveThinkers: Unknown thinker function");
	}

	// Add a terminating marker
	StreamOutByte(tc_end);
}

//==========================================================================
//
// UnarchiveThinkers
//
//==========================================================================

static void UnarchiveThinkers(void)
{
	byte tclass;
	thinker_t *currentthinker, *next;
	mobj_t *mobj;

//
// remove all the current thinkers
//
	currentthinker = thinkercap.next;
	while (currentthinker != &thinkercap)
	{
		next = currentthinker->next;
		if (currentthinker->function == P_MobjThinker)
			P_RemoveMobj ((mobj_t *)currentthinker);
		else
			Z_Free (currentthinker);
		currentthinker = next;
	}
	P_InitThinkers ();

// read in saved thinkers
	while (1)
	{
		tclass = *SavePtr++;
		switch (tclass)
		{
		case tc_end:
			return;			// end of list

		case tc_mobj:
			mobj = (mobj_t *) Z_Malloc (sizeof(*mobj), PU_LEVEL, NULL);
			memcpy (mobj, SavePtr, sizeof(*mobj));
			SavePtr += sizeof(*mobj);
			mobj->state = &states[(int)mobj->state];
			mobj->target = NULL;
			if (mobj->player)
			{
				mobj->player = &players[(int)mobj->player-1];
				mobj->player->mo = mobj;
			}
			P_SetThingPosition (mobj);
			mobj->info = &mobjinfo[mobj->type];
			mobj->floorz = mobj->subsector->sector->floorheight;
			mobj->ceilingz = mobj->subsector->sector->ceilingheight;
			mobj->thinker.function = P_MobjThinker;
			P_AddThinker (&mobj->thinker);
			break;
			
		default:
			I_Error ("Unknown tclass %i in savegame",tclass);
		}
	}
}

//==========================================================================
//
// ArchiveSpecials
//
//==========================================================================

static void ArchiveSpecials(void)
{
/*
T_MoveCeiling, (ceiling_t: sector_t * swizzle), - active list
T_VerticalDoor, (vldoor_t: sector_t * swizzle),
T_MoveFloor, (floormove_t: sector_t * swizzle),
T_LightFlash, (lightflash_t: sector_t * swizzle),
T_StrobeFlash, (strobe_t: sector_t *),
T_Glow, (glow_t: sector_t *),
T_PlatRaise, (plat_t: sector_t *), - active list
*/
	thinker_t *th;
	ceiling_t ceiling;
	vldoor_t door;
	floormove_t floor;
	plat_t plat;
	lightflash_t flash;
	strobe_t strobe;
	glow_t glow;

	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function == T_MoveCeiling)
		{
			StreamOutByte(tc_ceiling);
			memcpy(&ceiling, th, sizeof(ceiling_t));
			ceiling.sector = (sector_t *)(ceiling.sector-sectors);
			StreamOutBuffer(&ceiling, sizeof(ceiling_t));
			continue;
		}
		if (th->function == T_VerticalDoor)
		{
			StreamOutByte(tc_door);
			memcpy(&door, th, sizeof(vldoor_t));
			door.sector = (sector_t *)(door.sector-sectors);
			StreamOutBuffer(&door, sizeof(vldoor_t));
			continue;
		}
		if (th->function == T_MoveFloor)
		{
			StreamOutByte(tc_floor);
			memcpy(&floor, th, sizeof(floormove_t));
			floor.sector = (sector_t *)(floor.sector-sectors);
			StreamOutBuffer(&floor, sizeof(floormove_t));
			continue;
		}
		if (th->function == T_PlatRaise)
		{
			StreamOutByte(tc_plat);
			memcpy(&plat, th, sizeof(plat_t));
			plat.sector = (sector_t *)(plat.sector-sectors);
			StreamOutBuffer(&plat, sizeof(plat_t));
			continue;
		}
		if (th->function == T_LightFlash)
		{
			StreamOutByte(tc_flash);
			memcpy(&flash, th, sizeof(lightflash_t));
			flash.sector = (sector_t *)(flash.sector-sectors);
			StreamOutBuffer(&flash, sizeof(lightflash_t));
			continue;
		}
		if (th->function == T_StrobeFlash)
		{
			StreamOutByte(tc_strobe);
			memcpy(&strobe, th, sizeof(strobe_t));
			strobe.sector = (sector_t *)(strobe.sector-sectors);
			StreamOutBuffer(&strobe, sizeof(strobe_t));
			continue;
		}
		if (th->function == T_Glow)
		{
			StreamOutByte(tc_glow);
			memcpy(&glow, th, sizeof(glow_t));
			glow.sector = (sector_t *)(glow.sector-sectors);
			StreamOutBuffer(&glow, sizeof(glow_t));
			continue;
		}
	}
	// Add a terminating marker
	StreamOutByte(tc_endspecials);
}

//==========================================================================
//
// UnarchiveSpecials
//
//==========================================================================

static void UnarchiveSpecials(void)
{
	byte tclass;
	ceiling_t *ceiling;
	vldoor_t *door;
	floormove_t *floor;
	plat_t *plat;
	lightflash_t *flash;
	strobe_t *strobe;
	glow_t *glow;

// read in saved thinkers
	while (1)
	{
		tclass = *SavePtr++;
		switch (tclass)
		{
		case tc_endspecials:
			return;		// end of list

		case tc_ceiling:
			ceiling = (ceiling_t *) Z_Malloc (sizeof(*ceiling), PU_LEVEL, NULL);
			memcpy (ceiling, SavePtr, sizeof(*ceiling));
			SavePtr += sizeof(*ceiling);
			ceiling->sector = &sectors[(int)ceiling->sector];
			ceiling->sector->specialdata = T_MoveCeiling;
			if (ceiling->thinker.function)
				ceiling->thinker.function = T_MoveCeiling;
			P_AddThinker (&ceiling->thinker);
			P_AddActiveCeiling(ceiling);
			break;

		case tc_door:
			door = (vldoor_t *) Z_Malloc (sizeof(*door), PU_LEVEL, NULL);
			memcpy (door, SavePtr, sizeof(*door));
			SavePtr += sizeof(*door);
			door->sector = &sectors[(int)door->sector];
			door->sector->specialdata = door;
			door->thinker.function = T_VerticalDoor;
			P_AddThinker (&door->thinker);
			break;

		case tc_floor:
			floor = (floormove_t *) Z_Malloc (sizeof(*floor), PU_LEVEL, NULL);
			memcpy (floor, SavePtr, sizeof(*floor));
			SavePtr += sizeof(*floor);
			floor->sector = &sectors[(int)floor->sector];
			floor->sector->specialdata = T_MoveFloor;
			floor->thinker.function = T_MoveFloor;
			P_AddThinker (&floor->thinker);
			break;

		case tc_plat:
			plat = (plat_t *) Z_Malloc (sizeof(*plat), PU_LEVEL, NULL);
			memcpy (plat, SavePtr, sizeof(*plat));
			SavePtr += sizeof(*plat);
			plat->sector = &sectors[(int)plat->sector];
			plat->sector->specialdata = T_PlatRaise;
			if (plat->thinker.function)
				plat->thinker.function = T_PlatRaise;
			P_AddThinker (&plat->thinker);
			P_AddActivePlat(plat);
			break;

		case tc_flash:
			flash = (lightflash_t *) Z_Malloc (sizeof(*flash), PU_LEVEL, NULL);
			memcpy (flash, SavePtr, sizeof(*flash));
			SavePtr += sizeof(*flash);
			flash->sector = &sectors[(int)flash->sector];
			flash->thinker.function = T_LightFlash;
			P_AddThinker (&flash->thinker);
			break;

		case tc_strobe:
			strobe = (strobe_t *) Z_Malloc (sizeof(*strobe), PU_LEVEL, NULL);
			memcpy (strobe, SavePtr, sizeof(*strobe));
			SavePtr += sizeof(*strobe);
			strobe->sector = &sectors[(int)strobe->sector];
			strobe->thinker.function = T_StrobeFlash;
			P_AddThinker (&strobe->thinker);
			break;

		case tc_glow:
			glow = (glow_t *) Z_Malloc (sizeof(*glow), PU_LEVEL, NULL);
			memcpy (glow, SavePtr, sizeof(*glow));
			SavePtr += sizeof(*glow);
			glow->sector = &sectors[(int)glow->sector];
			glow->thinker.function = T_Glow;
			P_AddThinker (&glow->thinker);
			break;

		default:
			I_Error ("P_UnarchiveSpecials:Unknown tclass %i "
						"in savegame",tclass);
		}
	}
}

//==========================================================================
//
// OpenStreamOut
//
//==========================================================================

static void OpenStreamOut(const char *fileName)
{
	MallocFailureOk = true;
	SaveBuffer = Z_Malloc(SAVEGAMESIZE, PU_STATIC, NULL);
	SavePtr = (byte *)SaveBuffer;
	MallocFailureOk = false;
	if (SaveBuffer == NULL)
	{ // Not enough memory - use file save method
		SaveGameType = SVG_FILE;
		SavingFP = fopen(fileName, "wb");
	}
	else
	{
		SaveGameType = SVG_RAM;
	}
}

//==========================================================================
//
// CloseStreamOut
//
//==========================================================================

static void CloseStreamOut(const char *fileName)
{
	int length;

	StreamOutByte(SAVE_GAME_TERMINATOR);
	if (SaveGameType == SVG_RAM)
	{
		length = SavePtr - (byte *)SaveBuffer;
		if (length > SAVEGAMESIZE)
		{
			I_Error("Savegame buffer overrun");
		}
		M_WriteFile(fileName, SaveBuffer, length);
		Z_Free(SaveBuffer);
	}
	else
	{ // SVG_FILE
		fclose(SavingFP);
	}
}

//==========================================================================
//
// SV_Write
//
//==========================================================================

static void StreamOutBuffer(const void *buffer, int size)
{
	if (SaveGameType == SVG_RAM)
	{
		memcpy(SavePtr, buffer, size);
		SavePtr += size;
	}
	else
	{ // SVG_FILE
		fwrite(buffer, size, 1, SavingFP);
	}
}

static void StreamOutByte(byte val)
{
	StreamOutBuffer(&val, sizeof(byte));
}

static void StreamOutWord(unsigned short val)
{
	StreamOutBuffer(&val, sizeof(unsigned short));
}

static void StreamOutLong(unsigned int val)
{
	StreamOutBuffer(&val, sizeof(int));
}

