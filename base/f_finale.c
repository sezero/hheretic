// F_finale.c
// $Revision$
// $Date$

#include "h2stdinc.h"
#include <ctype.h>	// for toupper()
#include "doomdef.h"
#include "soundst.h"
#ifdef RENDER3D
#include "ogl_def.h"
#define WR_CacheLumpName(a,b)        W_GetNumForName(a)
#define V_DrawPatch(x,y,p)          OGL_DrawPatch(x,y,p)
#define V_DrawFuzzPatch(x,y,p)      OGL_DrawFuzzPatch(x,y,p)
#define V_DrawAltFuzzPatch(x,y,p)   OGL_DrawAltFuzzPatch(x,y,p)
#define V_DrawShadowedPatch(x,y,p)  OGL_DrawShadowedPatch(x,y,p)
#define V_DrawRawScreen(a)          OGL_DrawRawScreen(a)
#else
#define WR_CacheLumpName(a,b)           W_CacheLumpName(a,b)
#endif

int             finalestage;            // 0 = text, 1 = art screen
int             finalecount;

#define TEXTSPEED       3
#define TEXTWAIT        250

static const char    *e1text = E1TEXT;
static const char    *e2text = E2TEXT;
static const char    *e3text = E3TEXT;
static const char    *e4text = E4TEXT;
static const char    *e5text = E5TEXT;
static const char    *finaletext;
static const char    *finaleflat;

int FontABaseLump;

extern boolean viewactive;

extern void D_StartTitle(void);

/*
=======================
=
= F_StartFinale
=
=======================
*/

void F_StartFinale (void)
{
	gameaction = ga_nothing;
	gamestate = GS_FINALE;
	viewactive = false;
	automapactive = false;
	players[consoleplayer].messageTics = 1;
	players[consoleplayer].message = NULL;

	switch(gameepisode)
	{
		case 1:
			finaleflat = "FLOOR25";
			finaletext = e1text;
			break;
		case 2:
			finaleflat = "FLATHUH1";
			finaletext = e2text;
			break;
		case 3:
			finaleflat = "FLTWAWA2";
			finaletext = e3text;
			break;
		case 4:
			finaleflat = "FLOOR28";
			finaletext = e4text;
			break;
		case 5:
			finaleflat = "FLOOR08";
			finaletext = e5text;
			break;
	}

	finalestage = 0;
	finalecount = 0;
	FontABaseLump = W_GetNumForName("FONTA_S")+1;

//      S_ChangeMusic(mus_victor, true);
	S_StartSong(mus_cptd, true);
}



boolean F_Responder (event_t *event)
{
	if(event->type != ev_keydown)
	{
		return false;
	}
	if(finalestage == 1 && gameepisode == 2)
	{ // we're showing the water pic, make any key kick to demo mode
		finalestage++;
		// Hmm... naughty Heretic again :/ - DDOI
		//memset((byte *)0xa0000, 0, SCREENWIDTH*SCREENHEIGHT);
		//memset(screen, 0, SCREENWIDTH*SCREENHEIGHT);
		I_SetPalette(W_CacheLumpName("PLAYPAL", PU_CACHE));
		return true;
	}
	return false;
}


/*
=======================
=
= F_Ticker
=
=======================
*/

void F_Ticker (void)
{
	finalecount++;
	if (!finalestage && finalecount>strlen (finaletext)*TEXTSPEED + TEXTWAIT)
	{
		finalecount = 0;
		if(!finalestage)
		{
			finalestage = 1;
		}

//              wipegamestate = -1;             // force a wipe
/*
		if (gameepisode == 3)
			S_StartMusic (mus_bunny);
*/
	}
}


/*
=======================
=
= F_TextWrite
=
=======================
*/

//#include "HU_stuff.h"
//extern        patch_t *hu_font[HU_FONTSIZE];

void F_TextWrite (void)
{
#ifndef RENDER3D
	byte    *src, *dest;
	int             x,y;
#endif
	int             count;
	const char    *ch;
	int             c;
	int             cx, cy;
	patch_t *w;

//
// erase the entire screen to a tiled background
//
#ifndef RENDER3D
	src = W_CacheLumpName(finaleflat, PU_CACHE);
	dest = screen;
	for (y=0 ; y<SCREENHEIGHT ; y++)
	{
		for (x=0 ; x<SCREENWIDTH/64 ; x++)
		{
			memcpy (dest, src+((y&63)<<6), 64);
			dest += 64;
		}
		if (SCREENWIDTH&63)
		{
			memcpy (dest, src+((y&63)<<6), SCREENWIDTH&63);
			dest += (SCREENWIDTH&63);
		}
	}
#else
	OGL_SetFlat (R_FlatNumForName(finaleflat));
	OGL_DrawRectTiled(0,0,SCREENWIDTH,SCREENHEIGHT,64,64);
#endif

//      V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);

//
// draw some of the text onto the screen
//
	cx = 20;
	cy = 5;
	ch = finaletext;

	count = (finalecount - 10)/TEXTSPEED;
	if (count < 0)
		count = 0;
	for ( ; count ; count-- )
	{
		c = *ch++;
		if (!c)
			break;
		if (c == '\n')
		{
			cx = 20;
			cy += 9;
			continue;
		}

		c = toupper(c);
		if (c < 33)
		{
			cx += 5;
			continue;
		}

		w = W_CacheLumpNum(FontABaseLump+c-33, PU_CACHE);
		if (cx+w->width > SCREENWIDTH)
			break;
#ifdef RENDER3D
		OGL_DrawPatch (cx,cy,FontABaseLump+c-33);
#else
		V_DrawPatch(cx, cy, w);
#endif
		cx += w->width;
	}

}

void F_DrawPatchCol (int x, patch_t *patch, int col)
{
	column_t        *column;
	byte            *source, *dest, *desttop;
	int                     count;

	column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));
	desttop = screen+x;

// step through the posts in a column

	while (column->topdelta != 0xff )
	{
		source = (byte *)column + 3;
		dest = desttop + column->topdelta*SCREENWIDTH;
		count = column->length;

		while (count--)
		{
			*dest = *source++;
			dest += SCREENWIDTH;
		}
		column = (column_t *)(  (byte *)column + column->length+ 4 );
	}
}

/*
==================
=
= F_DemonScroll
=
==================
*/

void F_DemonScroll(void)
{
	byte *p1, *p2;
	static int yval = 0;
	static int nextscroll = 0;

	if(finalecount < nextscroll)
	{
		return;
	}
	p1 = W_CacheLumpName("FINAL1", PU_LEVEL);
	p2 = W_CacheLumpName("FINAL2", PU_LEVEL);
	if(finalecount < 70)
	{
		memcpy(screen, p1, SCREENHEIGHT*SCREENWIDTH);
		nextscroll = finalecount;
		return;
	}
	if(yval < 64000)
	{
		memcpy(screen, p2+SCREENHEIGHT*SCREENWIDTH-yval, yval);
		memcpy(screen+yval, p1, SCREENHEIGHT*SCREENWIDTH-yval);
		yval += SCREENWIDTH;
		nextscroll = finalecount+3;
	}
	else
	{ //else, we'll just sit here and wait, for now
		memcpy(screen, p2, SCREENWIDTH*SCREENHEIGHT);
	}
}

/*
==================
=
= F_DrawUnderwater
=
==================
*/

void F_DrawUnderwater(void)
{
#ifndef RENDER3D
	static boolean underwawa;
#endif
	extern boolean MenuActive;
	extern boolean askforquit;

	switch(finalestage)
	{
		case 1:
#ifndef RENDER3D	
			if(!underwawa)
			{
				underwawa = true;
				// Naughty Heretic :/ - DDOI
				//memset((byte *)0xa0000, 0, SCREENWIDTH*SCREENHEIGHT);
				I_SetPalette(W_CacheLumpName("E2PAL", PU_CACHE));
				memcpy(screen, W_CacheLumpName("E2END", PU_CACHE),
					SCREENWIDTH*SCREENHEIGHT);
			}
#endif
			paused = false;
			MenuActive = false;
			askforquit = false;

			break;
		case 2:
#ifndef RENDER3D		
			memcpy(screen, W_CacheLumpName("TITLE", PU_CACHE),
				SCREENWIDTH*SCREENHEIGHT);
#else
			OGL_DrawRawScreen(W_GetNumForName("TITLE"));
#endif
			//D_StartTitle(); // go to intro/demo mode.
	}
}


#if 0
/*
==================
=
= F_BunnyScroll
=
==================
*/

void F_BunnyScroll (void)
{
	int                     scrolled, x;
	patch_t         *p1, *p2;
	char            name[10];
	int                     stage;
	static int      laststage;

	p1 = W_CacheLumpName ("PFUB2", PU_LEVEL);
	p2 = W_CacheLumpName ("PFUB1", PU_LEVEL);

	V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);

	scrolled = 320 - (finalecount-230)/2;
	if (scrolled > 320)
		scrolled = 320;
	if (scrolled < 0)
		scrolled = 0;

	for ( x=0 ; x<SCREENWIDTH ; x++)
	{
		if (x+scrolled < 320)
			F_DrawPatchCol (x, p1, x+scrolled);
		else
			F_DrawPatchCol (x, p2, x+scrolled - 320);
	}

	if (finalecount < 1130)
		return;
	if (finalecount < 1180)
	{
		V_DrawPatch ((SCREENWIDTH-13*8)/2, (SCREENHEIGHT-8*8)/2,0, W_CacheLumpName ("END0",PU_CACHE));
		laststage = 0;
		return;
	}

	stage = (finalecount-1180) / 5;
	if (stage > 6)
		stage = 6;
	if (stage > laststage)
	{
		S_StartSound (NULL, sfx_pistol);
		laststage = stage;
	}

	sprintf (name,"END%i",stage);
	V_DrawPatch ((SCREENWIDTH-13*8)/2, (SCREENHEIGHT-8*8)/2, W_CacheLumpName (name,PU_CACHE));
}
#endif

/*
=======================
=
= F_Drawer
=
=======================
*/

void F_Drawer(void)
{
	UpdateState |= I_FULLSCRN;
	if (!finalestage)
		F_TextWrite ();
	else
	{
		switch (gameepisode)
		{
			case 1:
				if(shareware)
				{
					V_DrawRawScreen(WR_CacheLumpName("ORDER", PU_CACHE));
				}
				else
				{
					V_DrawRawScreen(WR_CacheLumpName("CREDIT", PU_CACHE));
				}
				break;
			case 2:
				F_DrawUnderwater();
				break;
			case 3:
				F_DemonScroll();
				break;
			case 4: // Just show credits screen for extended episodes
			case 5:
				V_DrawRawScreen(WR_CacheLumpName("CREDIT", PU_CACHE));
				break;
		}
	}
}
