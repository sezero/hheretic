//**************************************************************************
//**
//** S_SOUND.C:  MUSIC & SFX API
//**
//** $Revision$
//** $Date$
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "h2stdinc.h"
#include "doomdef.h"
#include "p_local.h"	/* P_AproxDistance() */
#include "sounds.h"
#include "i_sound.h"
#include "soundst.h"

// MACROS ------------------------------------------------------------------

#define DEFAULT_ARCHIVEPATH	"o:\\sound\\archive\\"
#define PRIORITY_MAX_ADJUST	10
#define DIST_ADJUST	(MAX_SND_DIST/PRIORITY_MAX_ADJUST)

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static boolean S_StopSoundID(int sound_id, int priority);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern int	snd_MaxVolume;
extern int	snd_MusicVolume;
extern int	snd_Channels;

#ifdef __WATCOMC__
extern int	snd_SfxDevice;
extern int	snd_MusicDevice;
extern int	snd_DesiredSfxDevice;
extern int	snd_DesiredMusicDevice;
extern int	tsm_ID;
#endif

extern void	**lumpcache;

extern int	startepisode;
extern int	startmap;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

/* Music info */
musicinfo_t S_music[] =
{
	{ "MUS_E1M1", 0 }, // 1-1
	{ "MUS_E1M2", 0 },
	{ "MUS_E1M3", 0 },
	{ "MUS_E1M4", 0 },
	{ "MUS_E1M5", 0 },
	{ "MUS_E1M6", 0 },
	{ "MUS_E1M7", 0 },
	{ "MUS_E1M8", 0 },
	{ "MUS_E1M9", 0 },

	{ "MUS_E2M1", 0 }, // 2-1
	{ "MUS_E2M2", 0 },
	{ "MUS_E2M3", 0 },
	{ "MUS_E2M4", 0 },
	{ "MUS_E1M4", 0 },
	{ "MUS_E2M6", 0 },
	{ "MUS_E2M7", 0 },
	{ "MUS_E2M8", 0 },
	{ "MUS_E2M9", 0 },

	{ "MUS_E1M1", 0 }, // 3-1
	{ "MUS_E3M2", 0 },
	{ "MUS_E3M3", 0 },
	{ "MUS_E1M6", 0 },
	{ "MUS_E1M3", 0 },
	{ "MUS_E1M2", 0 },
	{ "MUS_E1M5", 0 },
	{ "MUS_E1M9", 0 },
	{ "MUS_E2M6", 0 },

	{ "MUS_E1M6", 0 }, // 4-1
	{ "MUS_E1M2", 0 },
	{ "MUS_E1M3", 0 },
	{ "MUS_E1M4", 0 },
	{ "MUS_E1M5", 0 },
	{ "MUS_E1M1", 0 },
	{ "MUS_E1M7", 0 },
	{ "MUS_E1M8", 0 },
	{ "MUS_E1M9", 0 },

	{ "MUS_E2M1", 0 }, // 5-1
	{ "MUS_E2M2", 0 },
	{ "MUS_E2M3", 0 },
	{ "MUS_E2M4", 0 },
	{ "MUS_E1M4", 0 },
	{ "MUS_E2M6", 0 },
	{ "MUS_E2M7", 0 },
	{ "MUS_E2M8", 0 },
	{ "MUS_E2M9", 0 },

	{ "MUS_E3M2", 0 }, // 6-1
	{ "MUS_E3M3", 0 }, // 6-2
	{ "MUS_E1M6", 0 }, // 6-3

	{ "MUS_TITL", 0 },
	{ "MUS_INTR", 0 },
	{ "MUS_CPTD", 0 }
};

/* Sound info */
sfxinfo_t S_sfx[] =
{
	{ {0,0,0,0,0,0,0,0}, NULL, 0, -1, NULL, 0, 0 },
	{ "gldhit", NULL, 32, -1, NULL, 0, 2 },
	{ "gntful", NULL, 32, -1, NULL, 0, -1 },
	{ "gnthit", NULL, 32, -1, NULL, 0, -1 },
	{ "gntpow", NULL, 32, -1, NULL, 0, -1 },
	{ "gntact", NULL, 32, -1, NULL, 0, -1 },
	{ "gntuse", NULL, 32, -1, NULL, 0, -1 },
	{ "phosht", NULL, 32, -1, NULL, 0, 2 },
	{ "phohit", NULL, 32, -1, NULL, 0, -1 },
	{ "-phopow", &S_sfx[sfx_hedat1], 32, -1, NULL, 0, 1 },
	{ "lobsht", NULL, 20, -1, NULL, 0, 2 },
	{ "lobhit", NULL, 20, -1, NULL, 0, 2 },
	{ "lobpow", NULL, 20, -1, NULL, 0, 2 },
	{ "hrnsht", NULL, 32, -1, NULL, 0, 2 },
	{ "hrnhit", NULL, 32, -1, NULL, 0, 2 },
	{ "hrnpow", NULL, 32, -1, NULL, 0, 2 },
	{ "ramphit", NULL, 32, -1, NULL, 0, 2 },
	{ "ramrain", NULL, 10, -1, NULL, 0, 2 },
	{ "bowsht", NULL, 32, -1, NULL, 0, 2 },
	{ "stfhit", NULL, 32, -1, NULL, 0, 2 },
	{ "stfpow", NULL, 32, -1, NULL, 0, 2 },
	{ "stfcrk", NULL, 32, -1, NULL, 0, 2 },
	{ "impsit", NULL, 32, -1, NULL, 0, 2 },
	{ "impat1", NULL, 32, -1, NULL, 0, 2 },
	{ "impat2", NULL, 32, -1, NULL, 0, 2 },
	{ "impdth", NULL, 80, -1, NULL, 0, 2 },
	{ "-impact", &S_sfx[sfx_impsit], 20, -1, NULL, 0, 2 },
	{ "imppai", NULL, 32, -1, NULL, 0, 2 },
	{ "mumsit", NULL, 32, -1, NULL, 0, 2 },
	{ "mumat1", NULL, 32, -1, NULL, 0, 2 },
	{ "mumat2", NULL, 32, -1, NULL, 0, 2 },
	{ "mumdth", NULL, 80, -1, NULL, 0, 2 },
	{ "-mumact", &S_sfx[sfx_mumsit], 20, -1, NULL, 0, 2 },
	{ "mumpai", NULL, 32, -1, NULL, 0, 2 },
	{ "mumhed", NULL, 32, -1, NULL, 0, 2 },
	{ "bstsit", NULL, 32, -1, NULL, 0, 2 },
	{ "bstatk", NULL, 32, -1, NULL, 0, 2 },
	{ "bstdth", NULL, 80, -1, NULL, 0, 2 },
	{ "bstact", NULL, 20, -1, NULL, 0, 2 },
	{ "bstpai", NULL, 32, -1, NULL, 0, 2 },
	{ "clksit", NULL, 32, -1, NULL, 0, 2 },
	{ "clkatk", NULL, 32, -1, NULL, 0, 2 },
	{ "clkdth", NULL, 80, -1, NULL, 0, 2 },
	{ "clkact", NULL, 20, -1, NULL, 0, 2 },
	{ "clkpai", NULL, 32, -1, NULL, 0, 2 },
	{ "snksit", NULL, 32, -1, NULL, 0, 2 },
	{ "snkatk", NULL, 32, -1, NULL, 0, 2 },
	{ "snkdth", NULL, 80, -1, NULL, 0, 2 },
	{ "snkact", NULL, 20, -1, NULL, 0, 2 },
	{ "snkpai", NULL, 32, -1, NULL, 0, 2 },
	{ "kgtsit", NULL, 32, -1, NULL, 0, 2 },
	{ "kgtatk", NULL, 32, -1, NULL, 0, 2 },
	{ "kgtat2", NULL, 32, -1, NULL, 0, 2 },
	{ "kgtdth", NULL, 80, -1, NULL, 0, 2 },
	{ "-kgtact", &S_sfx[sfx_kgtsit], 20, -1, NULL, 0, 2 },
	{ "kgtpai", NULL, 32, -1, NULL, 0, 2 },
	{ "wizsit", NULL, 32, -1, NULL, 0, 2 },
	{ "wizatk", NULL, 32, -1, NULL, 0, 2 },
	{ "wizdth", NULL, 80, -1, NULL, 0, 2 },
	{ "wizact", NULL, 20, -1, NULL, 0, 2 },
	{ "wizpai", NULL, 32, -1, NULL, 0, 2 },
	{ "minsit", NULL, 32, -1, NULL, 0, 2 },
	{ "minat1", NULL, 32, -1, NULL, 0, 2 },
	{ "minat2", NULL, 32, -1, NULL, 0, 2 },
	{ "minat3", NULL, 32, -1, NULL, 0, 2 },
	{ "mindth", NULL, 80, -1, NULL, 0, 2 },
	{ "minact", NULL, 20, -1, NULL, 0, 2 },
	{ "minpai", NULL, 32, -1, NULL, 0, 2 },
	{ "hedsit", NULL, 32, -1, NULL, 0, 2 },
	{ "hedat1", NULL, 32, -1, NULL, 0, 2 },
	{ "hedat2", NULL, 32, -1, NULL, 0, 2 },
	{ "hedat3", NULL, 32, -1, NULL, 0, 2 },
	{ "heddth", NULL, 80, -1, NULL, 0, 2 },
	{ "hedact", NULL, 20, -1, NULL, 0, 2 },
	{ "hedpai", NULL, 32, -1, NULL, 0, 2 },
	{ "sorzap", NULL, 32, -1, NULL, 0, 2 },
	{ "sorrise", NULL, 32, -1, NULL, 0, 2 },
	{ "sorsit", NULL, 200, -1, NULL, 0, 2 },
	{ "soratk", NULL, 32, -1, NULL, 0, 2 },
	{ "soract", NULL, 200, -1, NULL, 0, 2 },
	{ "sorpai", NULL, 200, -1, NULL, 0, 2 },
	{ "sordsph", NULL, 200, -1, NULL, 0, 2 },
	{ "sordexp", NULL, 200, -1, NULL, 0, 2 },
	{ "sordbon", NULL, 200, -1, NULL, 0, 2 },
	{ "-sbtsit", &S_sfx[sfx_bstsit], 32, -1, NULL, 0, 2 },
	{ "-sbtatk", &S_sfx[sfx_bstatk], 32, -1, NULL, 0, 2 },
	{ "sbtdth", NULL, 80, -1, NULL, 0, 2 },
	{ "sbtact", NULL, 20, -1, NULL, 0, 2 },
	{ "sbtpai", NULL, 32, -1, NULL, 0, 2 },
	{ "plroof", NULL, 32, -1, NULL, 0, 2 },
	{ "plrpai", NULL, 32, -1, NULL, 0, 2 },
 	{ "plrdth", NULL, 80, -1, NULL, 0, 2 },
	{ "gibdth", NULL, 100, -1, NULL, 0, 2 },
	{ "plrwdth", NULL, 80, -1, NULL, 0, 2 },
	{ "plrcdth", NULL, 100, -1, NULL, 0, 2 },
	{ "itemup", NULL, 32, -1, NULL, 0, 2 },
	{ "wpnup", NULL, 32, -1, NULL, 0, 2 },
	{ "telept", NULL, 50, -1, NULL, 0, 2 },
	{ "doropn", NULL, 40, -1, NULL, 0, 2 },
	{ "dorcls", NULL, 40, -1, NULL, 0, 2 },
	{ "dormov", NULL, 40, -1, NULL, 0, 2 },
	{ "artiup", NULL, 32, -1, NULL, 0, 2 },
	{ "switch", NULL, 40, -1, NULL, 0, 2 },
	{ "pstart", NULL, 40, -1, NULL, 0, 2 },
	{ "pstop", NULL, 40, -1, NULL, 0, 2 },
	{ "stnmov", NULL, 40, -1, NULL, 0, 2 },
	{ "chicpai", NULL, 32, -1, NULL, 0, 2 },
	{ "chicatk", NULL, 32, -1, NULL, 0, 2 },
	{ "chicdth", NULL, 40, -1, NULL, 0, 2 },
	{ "chicact", NULL, 32, -1, NULL, 0, 2 },
	{ "chicpk1", NULL, 32, -1, NULL, 0, 2 },
	{ "chicpk2", NULL, 32, -1, NULL, 0, 2 },
	{ "chicpk3", NULL, 32, -1, NULL, 0, 2 },
	{ "keyup", NULL, 50, -1, NULL, 0, 2 },
	{ "ripslop", NULL, 16, -1, NULL, 0, 2 },
	{ "newpod", NULL, 16, -1, NULL, 0, -1 },
	{ "podexp", NULL, 40, -1, NULL, 0, -1 },
	{ "bounce", NULL, 16, -1, NULL, 0, 2 },
	{ "-volsht", &S_sfx[sfx_bstatk], 16, -1, NULL, 0, 2 },
	{ "-volhit", &S_sfx[sfx_lobhit], 16, -1, NULL, 0, 2 },
	{ "burn", NULL, 10, -1, NULL, 0, 2 },
	{ "splash", NULL, 10, -1, NULL, 0, 1 },
	{ "gloop", NULL, 10, -1, NULL, 0, 2 },
	{ "respawn", NULL, 10, -1, NULL, 0, 1 },
	{ "blssht", NULL, 32, -1, NULL, 0, 2 },
	{ "blshit", NULL, 32, -1, NULL, 0, 2 },
	{ "chat", NULL, 100, -1, NULL, 0, 1 },
	{ "artiuse", NULL, 32, -1, NULL, 0, 1 },
	{ "gfrag", NULL, 100, -1, NULL, 0, 1 },
	{ "waterfl", NULL, 16, -1, NULL, 0, 2 },

	// Monophonic sounds

	{ "wind", NULL, 16, -1, NULL, 0, 1 },
	{ "amb1", NULL, 1, -1, NULL, 0, 1 },
	{ "amb2", NULL, 1, -1, NULL, 0, 1 },
	{ "amb3", NULL, 1, -1, NULL, 0, 1 },
	{ "amb4", NULL, 1, -1, NULL, 0, 1 },
	{ "amb5", NULL, 1, -1, NULL, 0, 1 },
	{ "amb6", NULL, 1, -1, NULL, 0, 1 },
	{ "amb7", NULL, 1, -1, NULL, 0, 1 },
	{ "amb8", NULL, 1, -1, NULL, 0, 1 },
	{ "amb9", NULL, 1, -1, NULL, 0, 1 },
	{ "amb10", NULL, 1, -1, NULL, 0, 1 },
	{ "amb11", NULL, 1, -1, NULL, 0, 0 }
};

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static channel_t Channel[MAX_CHANNELS];
static int	AmbChan;
static int	RegisteredSong;	/* the current registered song. */
static int	isExternalSong;
static int	NextCleanup;
static boolean	MusicPaused;
static int	Mus_Song = -1;
static int	Mus_LumpNum;
static void	*Mus_SndPtr;
static byte	*SoundCurve;

// CODE --------------------------------------------------------------------

//==========================================================================
//
// S_Init
//
//==========================================================================

void S_Init(void)
{
	SoundCurve = (byte *) Z_Malloc(MAX_SND_DIST, PU_STATIC, NULL);
	I_StartupSound();
	if (snd_Channels > 8)
	{
		snd_Channels = 8;
	}
	I_SetChannels(snd_Channels);
	I_SetMusicVolume(snd_MusicVolume);
	S_SetMaxVolume(true);
}

//==========================================================================
//
// S_ShutDown
//
//==========================================================================

void S_ShutDown(void)
{
#ifdef __WATCOMC__
	if (tsm_ID == -1)
		return;
#endif
	if (RegisteredSong)
	{
		I_StopSong(RegisteredSong);
		I_UnRegisterSong(RegisteredSong);
	}
	I_ShutdownSound();
}

//==========================================================================
//
// S_Start
//
//==========================================================================

void S_Start(void)
{
	int i;

	S_StartSong((gameepisode-1)*9 + gamemap-1, true);

	// stop all sounds
	for (i = 0; i < snd_Channels; i++)
	{
		if (Channel[i].handle)
		{
			S_StopSound(Channel[i].mo);
		}
	}
	memset(Channel, 0, 8*sizeof(channel_t));
}

//==========================================================================
//
// S_StartSong
//
//==========================================================================

void S_StartSong(int song, boolean loop)
{
	if (song == Mus_Song)
	{ // don't replay an old song
		return;
	}
	if (RegisteredSong)
	{
		I_StopSong(RegisteredSong);
		I_UnRegisterSong(RegisteredSong);
		if (!isExternalSong)
		{
			Z_ChangeTag(lumpcache[Mus_LumpNum], PU_CACHE);
#ifdef __WATCOMC__
			_dpmi_unlockregion(Mus_SndPtr, lumpinfo[Mus_LumpNum].size);
#endif
		}
	}
	if (song < mus_e1m1 || song >= NUMMUSIC)
	{
		return;
	}
	isExternalSong = I_RegisterExternalSong(S_music[song].name);
	if (isExternalSong)
	{
		RegisteredSong = isExternalSong;
		I_PlaySong(RegisteredSong, loop);
		Mus_Song = song;
		return;
	}
	Mus_LumpNum = W_GetNumForName(S_music[song].name);
	Mus_SndPtr = W_CacheLumpNum(Mus_LumpNum, PU_MUSIC);
#ifdef __WATCOMC__
	_dpmi_lockregion(Mus_SndPtr, lumpinfo[Mus_LumpNum].size);
#endif
	RegisteredSong = I_RegisterSong(Mus_SndPtr);
	I_PlaySong(RegisteredSong, loop); // 'true' denotes endless looping.
	Mus_Song = song;
}

//==========================================================================
//
// S_StartSound
//
//==========================================================================

void S_StartSound(mobj_t *origin, int sound_id)
{
	static int sndcount = 0;

	int i;
	int dist, vol, chan;
	int priority;
	int angle, sep;
	int absx, absy;

	if (sound_id == 0 || snd_MaxVolume == 0)
		return;
#if 0
	if (origin == NULL)
	{
		origin = players[consoleplayer].mo;
	// this can be uninitialized when we are newly
	// started before the demos start playing !...
	}
#endif

	// calculate the distance before other stuff so that we can throw out
	// sounds that are beyond the hearing range.
	if (origin)
	{
		absx = abs(origin->x - players[consoleplayer].mo->x);
		absy = abs(origin->y - players[consoleplayer].mo->y);
	}
	else
	{
		absx = absy = 0;
	}
	dist = absx + absy - (absx > absy ? absy>>1 : absx>>1);
	dist >>= FRACBITS;
//	dist = P_AproxDistance(origin->x-viewx, origin->y-viewy)>>FRACBITS;

	if (dist >= MAX_SND_DIST)
	{
//		dist = MAX_SND_DIST - 1;
		return;	// sound is beyond the hearing range...
	}
	if (dist < 0)
	{
		dist = 0;
	}
	priority = S_sfx[sound_id].priority;
	priority *= (10 - (dist/160));
	if (!S_StopSoundID(sound_id, priority))
	{
		return;	// other sounds have greater priority
	}
	for (i = 0; i < snd_Channels; i++)
	{
		if (!origin || origin->player)
		{
			i = snd_Channels;
			break;	// let the player have more than one sound.
		}
		if (origin == Channel[i].mo)
		{ // only allow other mobjs one sound
			S_StopSound(Channel[i].mo);
			break;
		}
	}
	if (i >= snd_Channels)
	{
		if (sound_id >= sfx_wind)
		{
			if (AmbChan != -1
			     && S_sfx[sound_id].priority <=
				S_sfx[Channel[AmbChan].sound_id].priority)
			{
				return;	//ambient channel already in use
			}
			else
			{
				AmbChan = -1;
			}
		}
		for (i = 0; i < snd_Channels; i++)
		{
			if (Channel[i].mo == NULL)
			{
				break;
			}
		}
		if (i >= snd_Channels)
		{
			// look for a lower priority sound to replace.
			sndcount++;
			if (sndcount >= snd_Channels)
			{
				sndcount = 0;
			}
			for (chan = 0; chan < snd_Channels; chan++)
			{
				i = (sndcount + chan) % snd_Channels;
				if (priority >= Channel[i].priority)
				{
					chan = -1;	// denote that sound should be replaced.
					break;
				}
			}
			if (chan != -1)
			{
				return;	// no free channels.
			}
			else	// replace the lower priority sound.
			{
				if (Channel[i].handle)
				{
					if (I_SoundIsPlaying(Channel[i].handle))
					{
						I_StopSound(Channel[i].handle);
					}
					if (S_sfx[Channel[i].sound_id].usefulness > 0)
					{
						S_sfx[Channel[i].sound_id].usefulness--;
					}

					if (AmbChan == i)
					{
						AmbChan = -1;
					}
				}
			}
		}
	}
	if (S_sfx[sound_id].lumpnum == 0)
	{
		S_sfx[sound_id].lumpnum = I_GetSfxLumpNum(&S_sfx[sound_id]);
	}
	if (S_sfx[sound_id].snd_ptr == NULL)
	{
		if (W_LumpLength(S_sfx[sound_id].lumpnum) <= 8)
		{
			char name[9];
			strncpy(name, S_sfx[sound_id].name, 8);
			name[8] = '\0';
		//	I_Error("broken sound lump #%d (%s)\n",
			fprintf(stderr, "broken sound lump #%d (%s)\n",
					S_sfx[sound_id].lumpnum, name);
			return;
		}
		S_sfx[sound_id].snd_ptr =
			W_CacheLumpNum(S_sfx[sound_id].lumpnum, PU_SOUND);
#ifdef __WATCOMC__
		_dpmi_lockregion(S_sfx[sound_id].snd_ptr,
				 lumpinfo[S_sfx[sound_id].lumpnum].size);
#endif
	}

	// calculate the volume based upon the distance from the sound origin.
//	vol = (snd_MaxVolume*16 + dist*(-snd_MaxVolume*16)/MAX_SND_DIST)>>9;
	vol = SoundCurve[dist];

	if (!origin || origin == players[consoleplayer].mo)
	{
		sep = 128;
	}
	else
	{
		angle = R_PointToAngle2(players[consoleplayer].mo->x,
					players[consoleplayer].mo->y,
					origin->x, origin->y);
		angle = (angle - viewangle)>>24;
		sep = angle*2 - 128;
		if (sep < 64)
			sep = -sep;
		if (sep > 192)
			sep = 512-sep;
	}

	Channel[i].pitch = (byte)(127 + (M_Random() & 7) - (M_Random() & 7));
	Channel[i].handle = I_StartSound(sound_id, S_sfx[sound_id].snd_ptr, vol,
					 sep, Channel[i].pitch, 0);
	Channel[i].mo = origin;
	Channel[i].sound_id = sound_id;
	Channel[i].priority = priority;
	if (sound_id >= sfx_wind)
	{
		AmbChan = i;
	}
	if (S_sfx[sound_id].usefulness == -1)
	{
		S_sfx[sound_id].usefulness = 1;
	}
	else
	{
		S_sfx[sound_id].usefulness++;
	}
}

//==========================================================================
//
// S_StartSoundAtVolume
//
//==========================================================================
void S_StartSoundAtVolume(mobj_t *origin, int sound_id, int volume)
{
	int i;

	if (sound_id == 0 || snd_MaxVolume == 0)
		return;
	if (origin == NULL)
	{
		origin = players[displayplayer].mo;
	}

	if (volume == 0)
	{
		return;
	}
	volume = (volume * (snd_MaxVolume + 1) * 8)>>7;

// no priority checking, as ambient sounds would be the LOWEST.
	for (i = 0; i < snd_Channels; i++)
	{
		if (Channel[i].mo == NULL)
		{
			break;
		}
	}
	if (i >= snd_Channels)
	{
		return;
	}
	if (S_sfx[sound_id].lumpnum == 0)
	{
		S_sfx[sound_id].lumpnum = I_GetSfxLumpNum(&S_sfx[sound_id]);
	}
	if (S_sfx[sound_id].snd_ptr == NULL)
	{
		if (W_LumpLength(S_sfx[sound_id].lumpnum) <= 8)
		{
			char name[9];
			strncpy(name, S_sfx[sound_id].name, 8);
			name[8] = '\0';
		//	I_Error("broken sound lump #%d (%s)\n",
			fprintf(stderr, "broken sound lump #%d (%s)\n",
					S_sfx[sound_id].lumpnum, name);
			return;
		}
		S_sfx[sound_id].snd_ptr =
			W_CacheLumpNum(S_sfx[sound_id].lumpnum, PU_SOUND);
#ifdef __WATCOMC__
		_dpmi_lockregion(S_sfx[sound_id].snd_ptr,
				 lumpinfo[S_sfx[sound_id].lumpnum].size);
#endif
	}
	Channel[i].pitch = (byte)(127 - (M_Random() & 3) + (M_Random() & 3));
	Channel[i].handle = I_StartSound(sound_id, S_sfx[sound_id].snd_ptr,
					 volume, 128, Channel[i].pitch, 0);
	Channel[i].mo = origin;
	Channel[i].sound_id = sound_id;
	Channel[i].priority = 1; // super low priority.
	if (S_sfx[sound_id].usefulness == -1)
	{
		S_sfx[sound_id].usefulness = 1;
	}
	else
	{
		S_sfx[sound_id].usefulness++;
	}
}

//==========================================================================
//
// S_StopSoundID
//
//==========================================================================

boolean S_StopSoundID(int sound_id, int priority)
{
	int i;
	int lp; //least priority
	int found;

	if (S_sfx[sound_id].numchannels == -1)
	{
		return true;
	}
	lp = -1; //denote the argument sound_id
	found = 0;
	for (i = 0; i < snd_Channels; i++)
	{
		if (Channel[i].sound_id == sound_id && Channel[i].mo)
		{
			found++; //found one.  Now, should we replace it??
			if (priority >= Channel[i].priority)
			{ // if we're gonna kill one, then this'll be it
				lp = i;
				priority = Channel[i].priority;
			}
		}
	}
	if (found < S_sfx[sound_id].numchannels)
	{
		return true;
	}
	else if (lp == -1)
	{
		return false;	// don't replace any sounds
	}
	if (Channel[lp].handle)
	{
		if (I_SoundIsPlaying(Channel[lp].handle))
		{
			I_StopSound(Channel[lp].handle);
		}
		if (S_sfx[Channel[lp].sound_id].usefulness > 0)
		{
			S_sfx[Channel[lp].sound_id].usefulness--;
		}
		Channel[lp].mo = NULL;
	}
	return true;
}

//==========================================================================
//
// S_StopSound
//
//==========================================================================

void S_StopSound(mobj_t *origin)
{
	int i;

	for (i = 0; i < snd_Channels; i++)
	{
		if (Channel[i].mo == origin)
		{
			I_StopSound(Channel[i].handle);
			if (S_sfx[Channel[i].sound_id].usefulness > 0)
			{
				S_sfx[Channel[i].sound_id].usefulness--;
			}
			Channel[i].handle = 0;
			Channel[i].mo = NULL;
			if (AmbChan == i)
			{
				AmbChan = -1;
			}
		}
	}
}

//==========================================================================
//
// S_SoundLink
//
//==========================================================================

void S_SoundLink(mobj_t *oldactor, mobj_t *newactor)
{
	int i;

	for (i = 0; i < snd_Channels; i++)
	{
		if (Channel[i].mo == oldactor)
			Channel[i].mo = newactor;
	}
}

//==========================================================================
//
// S_PauseSound
//
//==========================================================================

void S_PauseSound(void)
{
	I_PauseSong(RegisteredSong);
}

//==========================================================================
//
// S_ResumeSound
//
//==========================================================================

void S_ResumeSound(void)
{
	I_ResumeSong(RegisteredSong);
}

//==========================================================================
//
// S_UpdateSounds
//
//==========================================================================

void S_UpdateSounds(mobj_t *listener)
{
	int i, dist, vol;
	int angle, sep;
	int priority;
	int absx, absy;

	listener = players[consoleplayer].mo;
	if (snd_MaxVolume == 0)
	{
		return;
	}

	if (NextCleanup < gametic)
	{
		for (i = 0; i < NUMSFX; i++)
		{
			if (S_sfx[i].usefulness == 0 && S_sfx[i].snd_ptr)
			{
				if (lumpcache[S_sfx[i].lumpnum])
				{
					if (((memblock_t *) ((byte*)(lumpcache[S_sfx[i].lumpnum]) -
								sizeof(memblock_t)))->id == ZONEID)
					{ // taken directly from the Z_ChangeTag macro
						Z_ChangeTag2(lumpcache[S_sfx[i].lumpnum], PU_CACHE);
#ifdef __WATCOMC__
						_dpmi_unlockregion(S_sfx[i].snd_ptr,
								   lumpinfo[S_sfx[i].lumpnum].size);
#endif
					}
				}
				S_sfx[i].usefulness = -1;
				S_sfx[i].snd_ptr = NULL;
			}
		}
		// Note, heretic does this every second (gametic+35)
		NextCleanup = gametic + 35*30;	// every 30 seconds
	}
	for (i = 0; i < snd_Channels; i++)
	{
		if (!Channel[i].handle || S_sfx[Channel[i].sound_id].usefulness == -1)
		{
			continue;
		}
		if (!I_SoundIsPlaying(Channel[i].handle))
		{
			if (S_sfx[Channel[i].sound_id].usefulness > 0)
			{
				S_sfx[Channel[i].sound_id].usefulness--;
			}
			Channel[i].handle = 0;
			Channel[i].mo = NULL;
			Channel[i].sound_id = 0;
			if (AmbChan == i)
			{
				AmbChan = -1;
			}
		}
		if (Channel[i].mo == NULL || Channel[i].sound_id == 0
			|| Channel[i].mo == listener)
		{
			continue;
		}
		else
		{
			absx = abs(Channel[i].mo->x - listener->x);
			absy = abs(Channel[i].mo->y - listener->y);
			dist = absx+absy-(absx > absy ? absy>>1 : absx>>1);
			dist >>= FRACBITS;

			if (dist >= MAX_SND_DIST)
			{
				S_StopSound(Channel[i].mo);
				continue;
			}
			if (dist < 0)
			{
				dist = 0;
			}
			vol = SoundCurve[dist];
			angle = R_PointToAngle2(listener->x, listener->y,
						Channel[i].mo->x, Channel[i].mo->y);
			angle = (angle - viewangle)>>24;
			sep = angle*2-128;
			if(sep < 64)
				sep = -sep;
			if(sep > 192)
				sep = 512-sep;
			I_UpdateSoundParams(Channel[i].handle, vol, sep, Channel[i].pitch);
			priority = S_sfx[Channel[i].sound_id].priority;
			priority *= PRIORITY_MAX_ADJUST - (dist / DIST_ADJUST);
			Channel[i].priority = priority;
		}
	}
}

//==========================================================================
//
// S_GetChannelInfo
//
//==========================================================================

void S_GetChannelInfo(SoundInfo_t *s)
{
	int i;
	ChanInfo_t *c;

	s->channelCount = snd_Channels;
	s->musicVolume = snd_MusicVolume;
	s->soundVolume = snd_MaxVolume;
	for (i = 0; i < snd_Channels; i++)
	{
		c = &s->chan[i];
		c->id = Channel[i].sound_id;
		c->priority = Channel[i].priority;
		c->name = S_sfx[c->id].name;
		c->mo = Channel[i].mo;
		c->distance = P_AproxDistance(c->mo->x-viewx, c->mo->y-viewy)>>FRACBITS;
	}
}

void S_SetMaxVolume(boolean fullprocess)
{
	int i;
	byte *SC = (byte *)W_CacheLumpName("SNDCURVE", PU_CACHE);
	if (!fullprocess)
	{
		SoundCurve[0] = ( *(SC) * (snd_MaxVolume * 8))>>7;
	}
	else
	{
		for (i = 0; i < MAX_SND_DIST; i++)
		{
			SoundCurve[i] = ( *(SC + i) * (snd_MaxVolume * 8))>>7;
		}
	}
}

//==========================================================================
//
// S_SetMusicVolume
//
//==========================================================================

void S_SetMusicVolume(void)
{
	I_SetMusicVolume(snd_MusicVolume);
	if (snd_MusicVolume == 0)
	{
		I_PauseSong(RegisteredSong);
		MusicPaused = true;
	}
	else if (MusicPaused)
	{
		MusicPaused = false;
		I_ResumeSong(RegisteredSong);
	}
}

