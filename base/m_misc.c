
// M_misc.c
// $Revision$
// $Date$

#include "h2stdinc.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include "doomdef.h"
#include "p_local.h"
#include "soundst.h"

#ifdef RENDER3D
#include "ogl_def.h"
#endif

int myargc;
char **myargv;

//---------------------------------------------------------------------------
//
// FUNC M_ValidEpisodeMap
//
//---------------------------------------------------------------------------

boolean M_ValidEpisodeMap(int episode, int map)
{
	if(episode < 1 || map < 1 || map > 9)
	{
		return false;
	}
	if(shareware)
	{ // Shareware version checks
		if(episode != 1)
		{
			return false;
		}
	}
	else if(ExtendedWAD)
	{ // Extended version checks
		if(episode == 6)
		{
			if(map > 3)
			{
				return false;
			}
		}
		else if(episode > 5)
		{
			return false;
		}
	}
	else
	{ // Registered version checks
		if(episode == 4)
		{
			if(map != 1)
			{
				return false;
			}
		}
		else if(episode > 3)
		{
			return false;
		}
	}
	return true;
}

/*
=================
=
= M_CheckParm
=
= Checks for the given parameter in the program's command line arguments
=
= Returns the argument number (1 to argc-1) or 0 if not present
=
=================
*/

int M_CheckParm (char *check)
{
	int     i;

	for (i = 1;i<myargc;i++)
	{
		if ( !strcasecmp(check, myargv[i]) )
			return i;
	}

	return 0;
}



/*
===============
=
= M_Random
=
= Returns a 0-255 number
=
===============
*/

unsigned char rndtable[256] = {
	0,   8, 109, 220, 222, 241, 149, 107,  75, 248, 254, 140,  16,  66,
	74,  21, 211,  47,  80, 242, 154,  27, 205, 128, 161,  89,  77,  36,
	95, 110,  85,  48, 212, 140, 211, 249,  22,  79, 200,  50,  28, 188,
	52, 140, 202, 120,  68, 145,  62,  70, 184, 190,  91, 197, 152, 224,
	149, 104,  25, 178, 252, 182, 202, 182, 141, 197,   4,  81, 181, 242,
	145,  42,  39, 227, 156, 198, 225, 193, 219,  93, 122, 175, 249,   0,
	175, 143,  70, 239,  46, 246, 163,  53, 163, 109, 168, 135,   2, 235,
	25,  92,  20, 145, 138,  77,  69, 166,  78, 176, 173, 212, 166, 113,
	94, 161,  41,  50, 239,  49, 111, 164,  70,  60,   2,  37, 171,  75,
	136, 156,  11,  56,  42, 146, 138, 229,  73, 146,  77,  61,  98, 196,
	135, 106,  63, 197, 195,  86,  96, 203, 113, 101, 170, 247, 181, 113,
	80, 250, 108,   7, 255, 237, 129, 226,  79, 107, 112, 166, 103, 241,
	24, 223, 239, 120, 198,  58,  60,  82, 128,   3, 184,  66, 143, 224,
	145, 224,  81, 206, 163,  45,  63,  90, 168, 114,  59,  33, 159,  95,
	28, 139, 123,  98, 125, 196,  15,  70, 194, 253,  54,  14, 109, 226,
	71,  17, 161,  93, 186,  87, 244, 138,  20,  52, 123, 251,  26,  36,
	17,  46,  52, 231, 232,  76,  31, 221,  84,  37, 216, 165, 212, 106,
	197, 242,  98,  43,  39, 175, 254, 145, 190,  84, 118, 222, 187, 136,
	120, 163, 236, 249
};
int rndindex = 0;
int prndindex = 0;

int P_Random (void)
{
	prndindex = (prndindex+1)&0xff;
	return rndtable[prndindex];
}

int M_Random (void)
{
	rndindex = (rndindex+1)&0xff;
	return rndtable[rndindex];
}

void M_ClearRandom (void)
{
	rndindex = prndindex = 0;
}


void M_ClearBox (fixed_t *box)
{
	box[BOXTOP] = box[BOXRIGHT] = H2MININT;
	box[BOXBOTTOM] = box[BOXLEFT] = H2MAXINT;
}

void M_AddToBox (fixed_t *box, fixed_t x, fixed_t y)
{
	if (x<box[BOXLEFT])
		box[BOXLEFT] = x;
	else if (x>box[BOXRIGHT])
		box[BOXRIGHT] = x;
	if (y<box[BOXBOTTOM])
		box[BOXBOTTOM] = y;
	else if (y>box[BOXTOP])
		box[BOXTOP] = y;
}



/*
==================
=
= M_WriteFile
=
==================
*/

boolean M_WriteFile (char const *name, const void *source, int length)
{
	int handle, count;

	handle = open (name, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
	if (handle == -1)
		return false;
	count = write (handle, source, length);
	close (handle);

	if (count < length)
		return false;

	return true;
}


/*
==================
=
= M_ReadFile
=
==================
*/

int M_ReadFile (char const *name, void **buffer)
{
	int handle, count, length;
	struct stat fileinfo;
	void        *buf;

	handle = open (name, O_RDONLY | O_BINARY, 0666);
	if (handle == -1)
		I_Error ("Couldn't read file %s", name);
	if (fstat (handle,&fileinfo) == -1)
		I_Error ("Couldn't read file %s", name);
	length = fileinfo.st_size;
	buf = Z_Malloc (length, PU_STATIC, NULL);
	count = read (handle, buf, length);
	close (handle);

	if (count < length)
		I_Error ("Couldn't read file %s", name);

	*buffer = buf;
	return length;
}

//---------------------------------------------------------------------------
//
// PROC M_FindResponseFile
//
//---------------------------------------------------------------------------

#define MAXARGVS 100

void M_FindResponseFile(void)
{
	int i;

	for(i = 1; i < myargc; i++)
	{
		if(myargv[i][0] == '@')
		{
			FILE *handle;
			int size;
			int k;
			int index;
			int indexinfile;
			char *infile;
			char *file;
			char *moreargs[20];
			char *firstargv;

			// READ THE RESPONSE FILE INTO MEMORY
			handle = fopen(&myargv[i][1], "rb");
			if(!handle)
			{
				printf("\nNo such response file!");
				exit(1);
			}
			printf("Found response file %s!\n",&myargv[i][1]);
			fseek (handle,0,SEEK_END);
			size = ftell(handle);
			fseek (handle,0,SEEK_SET);
			file = malloc (size);
			fread (file,size,1,handle);
			fclose (handle);

			// KEEP ALL CMDLINE ARGS FOLLOWING @RESPONSEFILE ARG
			for (index = 0,k = i+1; k < myargc; k++)
				moreargs[index++] = myargv[k];
			
			firstargv = myargv[0];
			myargv = malloc(sizeof(char *)*MAXARGVS);
			memset(myargv,0,sizeof(char *)*MAXARGVS);
			myargv[0] = firstargv;
			
			infile = file;
			indexinfile = k = 0;
			indexinfile++;  // SKIP PAST ARGV[0] (KEEP IT)
			do
			{
				myargv[indexinfile++] = infile+k;
				while(k < size &&  

					((*(infile+k)>= ' '+1) && (*(infile+k)<='z')))
					k++;
				*(infile+k) = 0;
				while(k < size &&
					((*(infile+k)<= ' ') || (*(infile+k)>'z')))
					k++;
			} while(k < size);
			
			for (k = 0;k < index;k++)
				myargv[indexinfile++] = moreargs[k];
			myargc = indexinfile;
			// DISPLAY ARGS
			if(M_CheckParm("-debug"))
			{
				printf("%d command-line args:\n", myargc);
				for(k = 1; k < myargc; k++)
				{
					printf("%s\n", myargv[k]);
				}
			}
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
// PROC M_ForceUppercase
//
// Change string to uppercase.
//
//---------------------------------------------------------------------------

void M_ForceUppercase(char *text)
{
	char c;

	while((c = *text) != 0)
	{
		if(c >= 'a' && c <= 'z')
		{
			*text++ = c-('a'-'A');
		}
		else
		{
			text++;
		}
	}
}

/*
==============================================================================

							DEFAULTS

==============================================================================
*/

int     usemouse;
int     usejoystick;

extern int mouselook;
extern int alwaysrun;

extern  int     key_right, key_left, key_up, key_down;
extern  int     key_strafeleft, key_straferight;
extern  int     key_fire, key_use, key_strafe, key_speed;
extern	int		key_flyup, key_flydown, key_flycenter;
extern	int		key_lookup, key_lookdown, key_lookcenter;
extern	int		key_invleft, key_invright, key_useartifact;

extern  int         mousebfire;
extern  int         mousebstrafe;
extern  int         mousebforward;

extern  int         joybfire;
extern  int         joybstrafe;
extern  int         joybuse;
extern  int         joybspeed;

extern  int         messageson;

extern  int     viewwidth, viewheight;

int mouseSensitivity;

extern  int screenblocks;

#ifndef __NeXT__
extern int snd_Channels;
extern int snd_DesiredMusicDevice, snd_DesiredSfxDevice;
extern int snd_MusicDevice, // current music card # (index to dmxCodes)
	snd_SfxDevice; // current sfx card # (index to dmxCodes)

extern int     snd_SBport, snd_SBirq, snd_SBdma;       // sound blaster variables
extern int     snd_Mport;                              // midi variables
#endif

default_t defaults[] =
{
/* change of order here affects mn_menu.c :
 * see, for example, Options3Items there...
 */
	{ "mouse_sensitivity", &mouseSensitivity, 5 },
	{ "sfx_volume", &snd_MaxVolume, 10},
	{ "music_volume", &snd_MusicVolume, 10},
	{ "key_right", &key_right, KEY_RIGHTARROW },
	{ "key_left", &key_left, KEY_LEFTARROW },
	{ "key_up", &key_up, KEY_UPARROW },
	{ "key_down", &key_down, KEY_DOWNARROW },
	{ "key_strafeleft", &key_strafeleft, ',' },
	{ "key_straferight", &key_straferight, '.' },
	{ "key_flyup", &key_flyup, KEY_PGUP },
	{ "key_flydown", &key_flydown, KEY_INS },
	{ "key_flycenter", &key_flycenter, KEY_HOME },
	{ "key_lookup", &key_lookup, KEY_PGDN },
	{ "key_lookdown", &key_lookdown, KEY_DEL },
	{ "key_lookcenter", &key_lookcenter, KEY_END },
	{ "key_invleft", &key_invleft, '[' },
	{ "key_invright", &key_invright, ']' },
	{ "key_useartifact", &key_useartifact, KEY_ENTER },

	{ "key_fire", &key_fire, KEY_RCTRL, 1 },
	{ "key_use", &key_use, ' ', 1 },
	{ "key_strafe", &key_strafe, KEY_RALT, 1 },
	{ "key_speed", &key_speed, KEY_RSHIFT, 1 },

	{ "use_mouse", &usemouse, 1 },
	{ "mouseb_fire", &mousebfire, 0 },
	{ "mouseb_strafe", &mousebstrafe, 1 },
	{ "mouseb_forward", &mousebforward, 2 },

	{ "use_joystick", &usejoystick, 0 },
	{ "joyb_fire", &joybfire, 0 },
	{ "joyb_strafe", &joybstrafe, 1 },
	{ "joyb_use", &joybuse, 3 },
	{ "joyb_speed", &joybspeed, 2 },

	{ "screenblocks", &screenblocks, 10 },
	{ "snd_channels", &snd_Channels, 3 },
	{ "snd_musicdevice", &snd_DesiredMusicDevice, 0 },
	{ "snd_sfxdevice", &snd_DesiredSfxDevice, 0 },
	{ "snd_sbport", &snd_SBport, 544 },
	{ "snd_sbirq", &snd_SBirq, -1 },
	{ "snd_sbdma", &snd_SBdma, -1 },
	{ "snd_mport", &snd_Mport, -1 },

	{ "usegamma", &usegamma, 0 },
	{ "messageson", &messageson, 1 },
	{ "mouselook", &mouselook, 1 },
	{ "alwaysrun", &alwaysrun, 0 }
};

default_str_t default_strings[] =
{
	{ "chatmacro0", chat_macros[0] },
	{ "chatmacro1", chat_macros[1] },
	{ "chatmacro2", chat_macros[2] },
	{ "chatmacro3", chat_macros[3] },
	{ "chatmacro4", chat_macros[4] },
	{ "chatmacro5", chat_macros[5] },
	{ "chatmacro6", chat_macros[6] },
	{ "chatmacro7", chat_macros[7] },
	{ "chatmacro8", chat_macros[8] },
	{ "chatmacro9", chat_macros[9] }
};

static int numdefaults, numstrings;

static char defaultfile[MAX_OSPATH];

/*
==============
=
= M_SaveDefaults
=
==============
*/

void M_SaveDefaults (void)
{
	int	i,v;
	FILE	*f;

	f = fopen (defaultfile, "w");
	if (!f)
		return;	// can't write the file, but don't complain

	for (i = 0; i < numdefaults; i++)
	{
		v = *defaults[i].location;
		fprintf (f, "%s\t\t%i\n", defaults[i].name, v);
	}

	for (i = 0; i < numstrings; i++)
	{
		fprintf (f, "%s\t\t\"%s\"\n", default_strings[i].name,
					default_strings[i].location);
	}

	fclose (f);
}


/*
==============
=
= M_LoadDefaults
=
==============
*/

extern byte scantokey[128];

void M_LoadDefaults(const char *fileName)
{
	int i, len;
	FILE *f;
	char def[80];
	char strparm[100];
	int parm;

//
// set everything to base values
//
	numdefaults = sizeof(defaults) / sizeof(defaults[0]);
	numstrings = sizeof(default_strings) / sizeof(default_strings[0]);
	printf("Loading default settings\n");
	for (i = 0; i < numdefaults; i++)
		*defaults[i].location = defaults[i].defaultvalue;
	// Make a backup of all default strings
	for (i = 0; i < numstrings; i++)
	{
		default_strings[i].defaultvalue = (char *) calloc(1, 80);
		strcpy (default_strings[i].defaultvalue, default_strings[i].location);
	}

//
// check for a custom default file
//
	i = M_CheckParm("-config");
	if (i && i < myargc-1)
	{
		snprintf(defaultfile, sizeof(defaultfile), "%s%s", basePath, myargv[i + 1]);
		printf("default file: %s\n", defaultfile);
	}
	else
	{
		snprintf(defaultfile, sizeof(defaultfile), "%s%s", basePath, fileName);
	}

//
// read the file in, overriding any set defaults
//
	f = fopen (defaultfile, "r");
	if (f)
	{
		while (!feof(f))
		{
			if (fscanf(f, "%79s %[^\n]\n", def, strparm) == 2)
			{
				if (strparm[0] == '"') /* string values */
				{
					for (i = 0; i < numstrings; i++)
					{
						if (!strcmp(def, default_strings[i].name))
						{
							len = (int)strlen(strparm) - 2;
							if (len <= 0)
							{
								default_strings[i].location[0] = '\0';
								break;
							}
							if (len > 79)
							{
								len = 79;
							}
							strncpy(default_strings[i].location, strparm + 1, len);
							default_strings[i].location[len] = '\0';
							break;
						}
					}
					continue;
				}

				/* numeric values */
				if (strparm[0] == '0' && strparm[1] == 'x')
				{
					sscanf(strparm + 2, "%x", &parm);
				}
				else
				{
					sscanf(strparm, "%i", &parm);
				}
				for (i = 0; i < numdefaults; i++)
				{
					if (!strcmp(def, defaults[i].name))
					{
						*defaults[i].location = parm;
						break;
					}
				}
			}
		}

		fclose (f);
	}


#ifdef __WATCOMC__
	for(i = 0; i < numdefaults; i++)
	{
		if(defaults[i].scantranslate)
		{
			parm = *defaults[i].location;
			defaults[i].untranslated = parm;
			*defaults[i].location = scantokey[parm];
		}
	}
#endif
}


/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/


typedef struct
{
	char	manufacturer;
	char	version;
	char	encoding;
	char	bits_per_pixel;
	unsigned short	xmin,ymin,xmax,ymax;
	unsigned short	hres,vres;
	unsigned char	palette[48];
	char	reserved;
	char	color_planes;
	unsigned short	bytes_per_line;
	unsigned short	palette_type;
	char	filler[58];
	unsigned char	data;	// unbounded
} pcx_t;

/*
==============
=
= WritePCXfile
=
==============
*/

void WritePCXfile (const char *filename, byte *data, int width, int height, byte *palette)
{
	int	i, length;
	pcx_t	*pcx;
	byte	*pack;
	
	pcx = (pcx_t *) Z_Malloc (width*height*2 + 1000, PU_STATIC, NULL);

	pcx->manufacturer = 0x0a;	// PCX id
	pcx->version = 5;		// 256 color
	pcx->encoding = 1;		// uncompressed
	pcx->bits_per_pixel = 8;	// 256 color
	pcx->xmin = 0;
	pcx->ymin = 0;
	pcx->xmax = SHORT(width-1);
	pcx->ymax = SHORT(height-1);
	pcx->hres = SHORT(width);
	pcx->vres = SHORT(height);
	memset (pcx->palette, 0, sizeof(pcx->palette));
	pcx->color_planes = 1;		// chunky image
	pcx->bytes_per_line = SHORT(width);
	pcx->palette_type = SHORT(2);	// not a grey scale
	memset (pcx->filler, 0, sizeof(pcx->filler));

//
// pack the image
//
	pack = &pcx->data;

	for (i = 0; i < width*height; i++)
	{
		if ((*data & 0xc0) != 0xc0)
			*pack++ = *data++;
		else
		{
			*pack++ = 0xc1;
			*pack++ = *data++;
		}
	}

//
// write the palette
//
	*pack++ = 0x0c;		// palette ID byte
	for (i = 0; i < 768; i++)
		*pack++ = *palette++;

//
// write output file
//
	length = pack - (byte *)pcx;
	M_WriteFile (filename, pcx, length);

	Z_Free (pcx);
}


//==============================================================================

/*
==================
=
= M_ScreenShot
=
==================
*/
#ifdef RENDER3D
void M_ScreenShot (void)
{
	OGL_GrabScreen();
}
#else
void M_ScreenShot (void)
{
	int	i;
	byte	*linear;
	char	lbmname[MAX_OSPATH], *p;
	byte	*pal;

#ifdef _WATCOMC_
	extern  byte *pcscreen;
#endif
//
// munge planar buffer to linear
//
#ifdef _WATCOMC_
	linear = pcscreen;
#else
	linear = screen;
#endif
//
// find a file name to save it to
//
	snprintf (lbmname, sizeof(lbmname), "%shrtic00.pcx", basePath);
	p = lbmname + strlen(basePath);
	for (i = 0; i <= 99; i++)
	{
		p[5] = i/10 + '0';
		p[6] = i%10 + '0';
		if (access(lbmname, F_OK) == -1)
			break;  // file doesn't exist
	}
	if (i == 100)
	{
		P_SetMessage(&players[consoleplayer], "SCREEN SHOT FAILED", false);
		return;
	}

//
// save the pcx file
//
#ifdef __WATCOMC__
	pal = (byte *)Z_Malloc(768, PU_STATIC, NULL);
	outp(0x3c7, 0);
	for(i = 0; i < 768; i++)
	{
		*(pal+i) = inp(0x3c9)<<2;
	}
#else
	pal = (byte *)W_CacheLumpName("PLAYPAL", PU_CACHE);
#endif

	WritePCXfile (lbmname, linear, SCREENWIDTH, SCREENHEIGHT, pal);

	P_SetMessage(&players[consoleplayer], "SCREEN SHOT", false);
#ifdef __WATCOMC__
	Z_Free(pal);
#endif
}
#endif

