//**************************************************************************
//**
//** $Id$
//**
//**************************************************************************

#include "h2stdinc.h"
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>
#include "doomdef.h"
#include "r_local.h"
#include "p_local.h"	/* for P_AproxDistance */
#include "sounds.h"
#include "i_sound.h"
#include "soundst.h"

// Macros

#define DEFAULT_ARCHIVEPATH	"o:\\sound\\archive\\"
#define PRIORITY_MAX_ADJUST	10
#define DIST_ADJUST	(MAX_SND_DIST/PRIORITY_MAX_ADJUST)

extern void **lumpcache;

extern void I_StartupMouse(void);
extern void I_ShutdownGraphics(void);

/*
===============================================================================

		MUSIC & SFX API

===============================================================================
*/

//static channel_t channel[MAX_CHANNELS];

//static int rs; //the current registered song.
//int mus_song = -1;
//int mus_lumpnum;
//void *mus_sndptr;
//byte *soundCurve;

extern sfxinfo_t S_sfx[];
extern musicinfo_t S_music[];

static channel_t Channel[MAX_CHANNELS];
static int RegisteredSong; //the current registered song.
static int isExternalSong;
static int NextCleanup;
static boolean MusicPaused;
static int Mus_Song = -1;
static int Mus_LumpNum;
static void *Mus_SndPtr;
static byte *SoundCurve;

extern int snd_MusicDevice;
extern int snd_SfxDevice;
extern int snd_MaxVolume;
extern int snd_MusicVolume;
extern int snd_Channels;

extern int startepisode;
extern int startmap;

int AmbChan;

boolean S_StopSoundID(int sound_id, int priority);

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
	int dist, vol;
	int i;
	int priority;
	int sep;
	int angle;
	int absx;
	int absy;

	static int sndcount = 0;
	int chan;

	if (sound_id == 0 || snd_MaxVolume == 0)
		return;
	if(origin == NULL)
	{
	//	origin = players[consoleplayer].mo;
	}

// calculate the distance before other stuff so that we can throw out
// sounds that are beyond the hearing range.
	if (origin)
	{
		absx = abs(origin->x-players[consoleplayer].mo->x);
		absy = abs(origin->y-players[consoleplayer].mo->y);
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
		S_sfx[sound_id].snd_ptr = W_CacheLumpNum(S_sfx[sound_id].lumpnum, PU_SOUND);
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
			//	players[consoleplayer].mo->y, Channel[i].mo->x, Channel[i].mo->y);
				players[displayplayer].mo->y, origin->x, origin->y);

		angle = (angle-viewangle)>>24;
		sep = angle*2 - 128;
		if (sep < 64)
			sep = -sep;
		if (sep > 192)
			sep = 512-sep;
	}

	Channel[i].pitch = (byte)(127 + (M_Random() & 7) - (M_Random() & 7));
	Channel[i].handle = I_StartSound(sound_id, S_sfx[sound_id].snd_ptr, vol, sep, Channel[i].pitch, 0);
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
	volume = (volume*(snd_MaxVolume+1)*8)>>7;

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
		S_sfx[sound_id].snd_ptr = W_CacheLumpNum(S_sfx[sound_id].lumpnum, PU_SOUND);
	}
	Channel[i].pitch = (byte)(127 - (M_Random() & 3) + (M_Random() & 3));
	Channel[i].handle = I_StartSound(sound_id, S_sfx[sound_id].snd_ptr, volume, 128, Channel[i].pitch, 0);
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
	int angle;
	int sep;
	int priority;
	int absx;
	int absy;

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
					if (((memblock_t *)((byte*) (lumpcache[S_sfx[i].lumpnum])-
							sizeof(memblock_t)))->id == ZONEID)
					{ // taken directly from the Z_ChangeTag macro
						Z_ChangeTag2(lumpcache[S_sfx[i].lumpnum], PU_CACHE);
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
			absx = abs(Channel[i].mo->x-listener->x);
			absy = abs(Channel[i].mo->y-listener->y);
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
			angle = (angle-viewangle)>>24;
			sep = angle*2-128;
			if(sep < 64)
				sep = -sep;
			if(sep > 192)
				sep = 512-sep;
			I_UpdateSoundParams(Channel[i].handle, vol, sep, Channel[i].pitch);
			priority = S_sfx[Channel[i].sound_id].priority;
			priority *= PRIORITY_MAX_ADJUST- (dist / DIST_ADJUST);
			Channel[i].priority = priority;
		}
	}
}

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

	if (!fullprocess)
	{
		SoundCurve[0] = (*((byte *)W_CacheLumpName("SNDCURVE", PU_CACHE))*(snd_MaxVolume*8))>>7;
	}
	else
	{
		for (i = 0; i < MAX_SND_DIST; i++)
		{
			SoundCurve[i] = (*((byte *)W_CacheLumpName("SNDCURVE", PU_CACHE)+i)*(snd_MaxVolume*8))>>7;
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

//==========================================================================
//
// S_ShutDown
//
//==========================================================================

void S_ShutDown(void)
{
	if (RegisteredSong)
	{
		I_StopSong(RegisteredSong);
		I_UnRegisterSong(RegisteredSong);
	}
	I_ShutdownSound();
}


/*
============================================================================

					TIMER INTERRUPT

============================================================================
*/


int		ticcount;
static long	_startSec;

static void I_StartupTimer(void)
{
	struct timeval tv;
	gettimeofday (&tv, NULL);
	_startSec = tv.tv_sec;
}

//--------------------------------------------------------------------------
//
// FUNC I_GetTime
//
// Returns time in 1/35th second tics.
//
//--------------------------------------------------------------------------

int I_GetTime (void)
{
	struct timeval tv;
	gettimeofday (&tv, NULL);
//	printf( "GT: %lx %lx\n", tv.tv_sec, tv.tv_usec );
//	ticcount = ((tv.tv_sec * 1000000) + tv.tv_usec) / 28571;
	ticcount = ((tv.tv_sec - _startSec) * 35) + (tv.tv_usec / 28571);
	return( ticcount );
}


/*
============================================================================

					JOYSTICK

============================================================================
*/

extern int usejoystick;
boolean joystickpresent;

/*
===============
=
= I_StartupJoystick
=
===============
*/

void I_StartupJoystick (void)
{
// NOTHING HERE YET: TOBE IMPLEMENTED IN i_sdl.c
	joystickpresent = false;
}

/*
===============
=
= I_JoystickEvents
=
===============
*/

void I_JoystickEvents (void)
{
}

/*
===============
=
= I_StartFrame
=
===============
*/

void I_StartFrame (void)
{
	I_JoystickEvents();
}


//==========================================================================


/*
===============
=
= I_Init
=
= hook interrupts and set graphics mode
=
===============
*/

void I_Init (void)
{
	I_StartupMouse();
	I_StartupJoystick();
	printf("S_Init... ");
	S_Init();
	S_Start();

	I_StartupTimer();
}


/*
===============
=
= I_Shutdown
=
= return to default system state
=
===============
*/

void I_Shutdown (void)
{
	S_ShutDown ();
	I_ShutdownGraphics ();
}


/*
================
=
= I_Error
=
================
*/

void I_Error (const char *error, ...)
{
	va_list argptr;

	D_QuitNetGame ();
	I_Shutdown ();
	va_start (argptr, error);
	vfprintf (stderr, error, argptr);
	va_end (argptr);
	fprintf (stderr, "\n");
	exit (1);
}

//--------------------------------------------------------------------------
//
// I_Quit
//
// Shuts down net game, saves defaults, prints the exit text message,
// goes to text mode, and exits.
//
//--------------------------------------------------------------------------

static void put_dos2ansi (byte attrib)
{
	byte	fore, back, blink = 0, intens = 0;
	int	table[] = { 30, 34, 32, 36, 31, 35, 33, 37 };

	fore  = attrib & 15;	// bits 0-3
	back  = attrib & 112;	// bits 4-6
	blink = attrib & 128;	// bit 7

	// Fix background, blink is either on or off.
	back = back >> 4;

	// Fix foreground
	if (fore > 7)
	{
		intens = 1;
		fore -= 8;
	}

	// Convert fore/back
	fore = table[fore];
	back = table[back] + 10;

	// 'Render'
	if (blink)
		printf ("\033[%d;5;%dm\033[%dm", intens, fore, back);
	else
		printf ("\033[%d;25;%dm\033[%dm", intens, fore, back);
}

static void I_ENDTEXT (void)
{
	int i, c, nl;
	byte *scr = (byte *)W_CacheLumpName("ENDTEXT", PU_CACHE);
	char *col = getenv("COLUMNS");

	if (col == NULL)
	{
		nl = 1;
		c = 80;
	}
	else
	{
		c = atoi(col);
		if (c > 80)
			nl = 1;
		else	nl = 0;
	}
	for (i = 1; i <= 80*25; scr += 2, i++)
	{
		put_dos2ansi (scr[1]);
		putchar (scr[0]);
		if (nl && !(i % 80))
			puts("\033[0m");
	}
	printf ("\033[m");	/* Cleanup */
}

void I_Quit (void)
{
	D_QuitNetGame();
	M_SaveDefaults();
	I_Shutdown();
	I_ENDTEXT ();

	exit(0);
}

/*
===============
=
= I_ZoneBase
=
===============
*/

byte *I_ZoneBase (int *size)
{
	byte *ptr;
	int heap = 0x800000;

	ptr = (byte *) malloc (heap);
	if (ptr == NULL)
		I_Error ("I_ZoneBase: Insufficient memory!");

	*size = heap;
	return ptr;
}

/*
=============
=
= I_AllocLow
=
=============
*/

byte *I_AllocLow (int length)
{
	byte *ptr;

	ptr = (byte *) malloc (length);
	if (ptr == NULL)
		I_Error ("I_AllocLow: Insufficient memory!");

	return ptr;
}

/*
============================================================================

						NETWORKING

============================================================================
*/

extern	doomcom_t	*doomcom;

/*
====================
=
= I_InitNetwork
=
====================
*/

void I_InitNetwork (void)
{
	int		i;

	i = M_CheckParm ("-net");
	if (!i)
	{
	//
	// single player game
	//
		doomcom = (doomcom_t *) malloc (sizeof(*doomcom));
		memset (doomcom, 0, sizeof(*doomcom));
		netgame = false;
		doomcom->id = DOOMCOM_ID;
		doomcom->numplayers = doomcom->numnodes = 1;
		doomcom->deathmatch = false;
		doomcom->consoleplayer = 0;
		doomcom->ticdup = 1;
		doomcom->extratics = 0;
		return;
	}

#if 0
	// THIS IS DOS-ISH AND BROKEN ON UNIX!!!
	doomcom = (doomcom_t *)atoi(myargv[i+1]);
	netgame = true;
	//DEBUG
	doomcom->skill = startskill;
	doomcom->episode = startepisode;
	doomcom->map = startmap;
	doomcom->deathmatch = deathmatch;
#endif
	I_Error ("NET GAME NOT IMPLEMENTED !!!");
}

void I_NetCmd (void)
{
	if (!netgame)
		I_Error ("I_NetCmd when not in netgame");
}


//=========================================================================
//
// I_CheckExternDriver
//
//		Checks to see if a vector, and an address for an external driver
//			have been passed.
//=========================================================================

void I_CheckExternDriver (void)
{
// THIS IS FOR DOS, ONLY. NOTHING ON UNIX.
	useexterndriver = false;
}


//=========================================================================
//
//		MAIN
//
//=========================================================================

static char base[MAX_OSPATH];
static const char datadir[] = SHARED_DATAPATH;	/* set by 'configure' */

static void CreateBasePath (void)
{
	int rc;
	char *homedir = getenv("HOME");
	if (homedir == NULL)
		I_Error ("Unable to determine user home directory");
	/* make sure that basePath has a trailing slash */
	snprintf(base, sizeof(base), "%s/%s/", homedir, H_USERDIR);
	basePath = base;
	rc = mkdir(base, S_IRWXU|S_IRWXG|S_IRWXO);
	if (rc != 0 && errno != EEXIST)
		I_Error ("Unable to create hheretic user directory");
}

static void InitializeWaddir (void)
{
	int i;
	const char *_waddir;

	_waddir = NULL;
	i = M_CheckParm("-waddir");
	if (i && i < myargc - 1)
		_waddir = myargv[i + 1];
	if (_waddir == NULL)
		_waddir = getenv(DATA_ENVVAR);
	if (_waddir == NULL)
	{
		if (datadir[0])
			_waddir = datadir;
	}
	waddir = _waddir;
	if (waddir && *waddir)
		printf ("Shared data path: %s\n", waddir);
}

static void PrintVersion (void)
{
	printf ("HHeretic v%d.%d.%d\n", VERSION_MAJ, VERSION_MIN, VERSION_PATCH);
}

static void PrintHelp (const char *name)
{
	printf ("http://sourceforge.net/projects/hhexen\n");
	printf ("http://hhexen.sf.net , http://icculus.org/hast\n");
	printf ("\n");
	printf ("Usage: %s [options]\n", name);
	printf ("     [ -h | --help]           Display this help message\n");
	printf ("     [ -v | --version]        Display the game version\n");
	printf ("     [ -f | --fullscreen]     Run the game fullscreen\n");
	printf ("     [ -w | --windowed]       Run the game windowed\n");
	printf ("     [ -s | --nosound]        Run the game without sound\n");
	printf ("     [ -g | --nograb]         Disable mouse grabbing\n");
#ifdef RENDER3D
	printf ("     [ -width  <width> ]      Set screen width \n");
	printf ("     [ -height <height> ]     Set screen height\n");
#endif
	printf ("     [ -file <wadfile> ]      Load extra wad files\n");
	printf ("     [ -waddir <path>  ]      Specify shared data path\n");
	printf ("  You can use the %s environment variable or the\n",
		DATA_ENVVAR);
	printf (" -waddir option to force the game's shared data directory.\n");
}

#define H2_LITTLE_ENDIAN	0	/* 1234 */
#define H2_BIG_ENDIAN		1	/* 4321 */
#define H2_PDP_ENDIAN		2	/* 3412 */

#if defined(WORDS_BIGENDIAN)
#define COMPILED_BYTEORDER	H2_BIG_ENDIAN
#else
#define COMPILED_BYTEORDER	H2_LITTLE_ENDIAN
#endif

static int DetectByteorder (void)
{
	int	i = 0x12345678;
		/*    U N I X */

	/*
	BE_ORDER:  12 34 56 78
		   U  N  I  X

	LE_ORDER:  78 56 34 12
		   X  I  N  U

	PDP_ORDER: 34 12 78 56
		   N  U  X  I
	*/

	if ( *(char *)&i == 0x12 )
		return H2_BIG_ENDIAN;
	else if ( *(char *)&i == 0x78 )
		return H2_LITTLE_ENDIAN;
	else if ( *(char *)&i == 0x34 )
		return H2_PDP_ENDIAN;

	return -1;
}

static void ValidateByteorder (void)
{
	const char	*endianism[] = { "LE", "BE", "PDP" };
	int		host_byteorder;

	host_byteorder = DetectByteorder ();
	if (host_byteorder < 0)
	{
		fprintf (stderr, "Unsupported byte order.\n");
		exit (1);
	}
	if (host_byteorder != COMPILED_BYTEORDER)
	{
		fprintf (stderr, "Detected byte order %s doesn't match compiled %s order!\n",
				 endianism[host_byteorder], endianism[COMPILED_BYTEORDER]);
		exit (1);
	}
	printf ("Detected byte order: %s\n", endianism[host_byteorder]);
}

int main (int argc, char **argv)
{
	PrintVersion ();

	myargc = argc;
	myargv = (const char **) argv;

	if (M_CheckParm("--version") || M_CheckParm("-v"))
		return 0;
	if (M_CheckParm("--help") || M_CheckParm("-h") || M_CheckParm("-?"))
	{
		PrintHelp (argv[0]);
		return 0;
	}

	ValidateByteorder();
	CreateBasePath();
	InitializeWaddir();

	D_DoomMain();

	return 0;
}

