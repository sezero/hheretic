/*  Ripped && Adapted from the PrBoom project:
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301  USA.
 *
 * DESCRIPTION:
 *  System interface for sound, using SDL.
 *
 */

#include "h2stdinc.h"
#include <math.h>	/* pow() */
#include "SDL.h"
#include "doomdef.h"
#include "sounds.h"
#include "i_sound.h"


#define SAMPLE_FORMAT	AUDIO_S16SYS	/* requested, see set_sample_format() */
#define SAMPLE_RATE	11025	/* Hz */
#define SAMPLE_CHANNELS	2


/*
 *	SOUND HEADER & DATA
 */

int snd_Channels;
int snd_MaxVolume,		/* maximum volume for sound */
	snd_MusicVolume;	/* maximum volume for music */
boolean snd_MusicAvail,		/* whether music is available */
	snd_SfxAvail;		/* whether sfx are available */

/*
 *	SOUND FX API
 */

typedef struct
{
	unsigned char	*begin;		/* pointers into Sample.firstSample */
	unsigned char	*end;

	short		*lvol_table;	/* point into vol_lookup */
	short		*rvol_table;

	unsigned int	pitch_step;
	unsigned int	step_remainder;	/* 0.16 bit remainder of last step. */

	int		pri;
	unsigned int	time;
} Channel;

typedef struct
{
/* Sample data is a lump from a wad: byteswap the a, freq
 * and the length fields before using them		*/
	short		a;		/* always 3	*/
	short		freq;		/* always 11025	*/
	int32_t		length;		/* sample length */
	unsigned char	firstSample;
} Sample;
COMPILE_TIME_ASSERT(Sample, offsetof(Sample,firstSample) == 8);


#define CHAN_COUNT	8
static Channel	channel[CHAN_COUNT];

#define MAX_VOL		64	/* 64 keeps our table down to 16Kb */
static short	vol_lookup[MAX_VOL * 256];

static int	steptable[256];		/* Pitch to stepping lookup */

static boolean	snd_initialized;
static int	SAMPLECOUNT = 512;
int	snd_samplerate = SAMPLE_RATE;
static int	snd_outrate = SAMPLE_RATE;	/* rate the device actually runs at */

static Uint16	snd_format;		/* output format, see audio_loop() */

static boolean set_sample_format (Uint16 format)
{
	switch (format)
	{
	case AUDIO_U8:
	case AUDIO_S8:
	case AUDIO_U16SYS:
	case AUDIO_S16SYS:
		snd_format = format;
		return true;
	}
	return false;
}


// Adds the current sample of every active channel to *pdl / *pdr,
// advances the channels and clamps the sums to signed 16-bit.
static inline void mix_channels (int *pdl, int *pdr)
{
	Channel* chan;
	Channel* cend;
	unsigned int sample;
	register int dl = *pdl;
	register int dr = *pdr;

	cend = channel + CHAN_COUNT;

	chan = channel;
	for ( ; chan < cend; chan++)
	{
		// Check channel, if active.
		if (chan->begin)
		{
			// Get the sample from the channel.
			sample = *chan->begin;

			// Adjust volume accordingly.
			dl += chan->lvol_table[sample];
			dr += chan->rvol_table[sample];

			// Increment sample pointer with pitch adjustment.
			chan->step_remainder += chan->pitch_step;
			chan->begin += chan->step_remainder >> 16;
			chan->step_remainder &= 65535;

			// Check whether we are done.
			if (chan->begin >= chan->end)
			{
				chan->begin = NULL;
			//	printf ("  channel done %d\n", chan);
			}
		}
	}

	if (dl > 0x7fff)
		dl = 0x7fff;
	else if (dl < -0x8000)
		dl = -0x8000;
	if (dr > 0x7fff)
		dr = 0x7fff;
	else if (dr < -0x8000)
		dr = -0x8000;

	*pdl = dl;
	*pdr = dr;
}

// The channels are mixed as signed 16-bit and converted to the output
// format while writing the stream.  Unsigned formats differ from signed
// ones only in the inverted sign bit (signflip).
static inline void mix_stream (Uint8 *stream, int len,
				boolean bits8, int signflip)
{
	int dl, dr;

	if (bits8)
	{
		Uint8 *begin = stream;
		Uint8 *end = stream + len;

		while (begin < end)
		{
			dl = 0;
			dr = 0;
			mix_channels(&dl, &dr);

			begin[0] = (dl >> 8) ^ signflip;
			begin[1] = (dr >> 8) ^ signflip;
			begin += 2;
		}
	}
	else
	{
		Uint16 *begin = (Uint16 *) stream;
		Uint16 *end = (Uint16 *) (stream + len);

		while (begin < end)
		{
			dl = 0;
			dr = 0;
			mix_channels(&dl, &dr);

			begin[0] = dl ^ signflip;
			begin[1] = dr ^ signflip;
			begin += 2;
		}
	}
}

static void audio_loop (void *unused, Uint8 *stream, int len)
{
	// Constant arguments make the compiler build a separate loop
	// for each format, without per-sample format checks.
	switch (snd_format)
	{
	case AUDIO_U8:
		mix_stream(stream, len, true, 0x80);
		break;
	case AUDIO_S8:
		mix_stream(stream, len, true, 0);
		break;
	case AUDIO_U16SYS:
		mix_stream(stream, len, false, 0x8000);
		break;
	case AUDIO_S16SYS:
		mix_stream(stream, len, false, 0);
		break;
	}
}


void I_SetSfxVolume(int volume)
{
}

// Gets lump nums of the named sound.  Returns pointer which will be
// passed to I_StartSound() when you want to start an SFX.  Must be
// sure to pass this to UngetSoundEffect() so that they can be
// freed!

int I_GetSfxLumpNum(sfxinfo_t *sound)
{
	if (sound->name[0] == 0)
		return 0;
	if (sound->link)
		sound = sound->link;
	return W_GetNumForName(sound->name);
}


// Id is unused.
// Data is a pointer to a Sample structure.
// Volume ranges from 0 to 127.
// Separation (orientation/stereo) ranges from 0 to 255.  128 is balanced.
// Pitch ranges from 0 to 255.  Normal is 128.
// Priority looks to be unused (always 0).

int I_StartSound(int id, void *data, int vol, int sep, int pitch, int priority)
{
	// Relative time order to find oldest sound.
	static unsigned int soundTime = 0;
	int chanId;
	Sample *sample;
	Channel *chan;
	int oldest;
	int i;

	// Find an empty channel, the oldest playing channel, or default to 0.
	// Currently ignoring priority.

	chanId = 0;
	oldest = soundTime;
	for (i = 0; i < CHAN_COUNT; i++)
	{
		if (! channel[ i ].begin)
		{
			chanId = i;
			break;
		}
		if (channel[ i ].time < oldest)
		{
			chanId = i;
			oldest = channel[ i ].time;
		}
	}

	sample = (Sample *) data;
	chan = &channel[chanId];

	I_UpdateSoundParams(chanId + 1, vol, sep, pitch);

	// begin must be set last because the audio thread will access the channel
	// once it is non-zero.  Perhaps this should be protected by a mutex.
	chan->pri = priority;
	chan->time = soundTime;
	chan->end = &sample->firstSample + LONG(sample->length);
	chan->begin = &sample->firstSample;

	soundTime++;

#if 0
	printf ("I_StartSound %d: v:%d s:%d p:%d pri:%d | %d %d %d %d\n",
		id, vol, sep, pitch, priority,
		chanId, chan->pitch_step, SHORT(sample->a), SHORT(sample->freq));
#endif

	return chanId + 1;
}

void I_StopSound(int handle)
{
	handle--;
	handle &= 7;
	channel[handle].begin = NULL;
}

int I_SoundIsPlaying(int handle)
{
	handle--;
	handle &= 7;
	return (channel[ handle ].begin != NULL);
}

void I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
	int lvol, rvol;
	Channel *chan;

	if (!snd_initialized)
		return;
	SDL_LockAudio();
	// Set left/right channel volume based on seperation.
	sep += 1;	// range 1 - 256
	lvol = vol - ((vol * sep * sep) >> 16);	// (256*256);
	sep = sep - 257;
	rvol = vol - ((vol * sep * sep) >> 16);

	// Sanity check, clamp volume.
	if (rvol < 0)
	{
	//	printf ("rvol out of bounds %d, id %d\n", rvol, handle);
		rvol = 0;
	}
	else if (rvol > 127)
	{
	//	printf ("rvol out of bounds %d, id %d\n", rvol, handle);
		rvol = 127;
	}

	if (lvol < 0)
	{
	//	printf ("lvol out of bounds %d, id %d\n", lvol, handle);
		lvol = 0;
	}
	else if (lvol > 127)
	{
	//	printf ("lvol out of bounds %d, id %d\n", lvol, handle);
		lvol = 127;
	}

	// Limit to MAX_VOL (64)
	lvol >>= 1;
	rvol >>= 1;

	handle--;
	handle &= 7;
	chan = &channel[handle];
	chan->pitch_step = steptable[pitch];
	chan->step_remainder = 0;
	chan->lvol_table = &vol_lookup[lvol * 256];
	chan->rvol_table = &vol_lookup[rvol * 256];

	SDL_UnlockAudio();
}


/*
 *	SOUND STARTUP STUFF
 */

// inits all sound stuff
void I_StartupSound (void)
{
	SDL_AudioSpec desired, obtained;

	if (snd_initialized)
		return;

	if (M_CheckParm("--nosound") || M_CheckParm("-s") || M_CheckParm("-nosound"))
	{
		fprintf(stdout, "I_StartupSound: Sound Disabled.\n");
		return;
	}

	fprintf(stdout, "I_StartupSound (SDL):\n");
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "Couldn't init SDL audio: %s\n", SDL_GetError());
		return;
	}

	/* Initialize variables */
	snd_SfxAvail = snd_MusicAvail = false;
	desired.freq = snd_samplerate;
	desired.format = SAMPLE_FORMAT;
	desired.channels = SAMPLE_CHANNELS;
	SAMPLECOUNT = 512;
	desired.samples = SAMPLECOUNT*snd_samplerate/11025;
	desired.callback = audio_loop;

	if (SDL_OpenAudio(&desired, &obtained) == -1)
	{
		fprintf(stderr, "Couldn't open audio with desired format\n");
		return;
	}
	if (!set_sample_format(obtained.format))
	{
		fprintf(stderr, "Unsupported audio format 0x%04x\n", obtained.format);
		SDL_CloseAudio();
		return;
	}
	snd_initialized = true;
	snd_outrate = obtained.freq;
	SAMPLECOUNT = obtained.samples;
	fprintf(stdout, "Configured audio device with %d Hz, %d samples/slice\n",
		snd_outrate, SAMPLECOUNT);
	snd_SfxAvail = true;
	SDL_PauseAudio(0);
}

// shuts down all sound stuff
void I_ShutdownSound (void)
{
	if (snd_initialized)
	{
		snd_initialized = false;
		snd_SfxAvail = false;
		snd_MusicAvail = false;
		SDL_CloseAudio();
	}
}

void I_SetChannels(int channels)
{
	int v, j;
	int *steptablemid;

	// We always have CHAN_COUNT channels.

	for (j = 0; j < CHAN_COUNT; j++)
	{
		channel[j].begin = NULL;
		channel[j].end   = NULL;
		channel[j].time = 0;
	}

	// This table provides step widths for pitch parameters, scaled
	// from the sound effect rate to the output rate.
	steptablemid = steptable + 128;
	for (j = -128; j < 128; j++)
	{
		steptablemid[j] = (int) (pow(2.0, (j/64.0)) * 65536.0 *
					 SND_SAMPLERATE / snd_outrate);
	}

	// Generate the volume lookup tables.
	for (v = 0; v < MAX_VOL; v++)
	{
		// Turn the unsigned samples into signed samples.
		for (j = 0; j < 256; j++)
		{
			vol_lookup[v*256+j] = (v * (j-128) * 256) / (MAX_VOL-1);
			//printf ("vol_lookup[%d*256+%d] = %d\n", v, j, vol_lookup[v*256+j]);
		}
	}
}


/*
 *	SONG API
 */

int I_RegisterSong(void *data, int siz)
{
	return 0;
}

int I_RegisterExternalSong(const char *name)
{
	return 0;
}

void I_UnRegisterSong(int handle)
{
}

void I_PauseSong(int handle)
{
}

void I_ResumeSong(int handle)
{
}

void I_SetMusicVolume(int volume)
{
}

int I_QrySongPlaying(int handle)
{
	return 0;
}

// Stops a song.  MUST be called before I_UnregisterSong().
void I_StopSong(int handle)
{
}

void I_PlaySong(int handle, boolean looping)
{
}
