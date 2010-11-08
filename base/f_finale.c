// F_finale.c
// $Revision$
// $Date$

#include "h2stdinc.h"
#include <ctype.h>
#include "doomdef.h"
#include "soundst.h"
#include "v_compat.h"

#define TEXTSPEED	3
#define TEXTWAIT	250

#ifdef RENDER3D
#define V_DrawPatch(x,y,p)		OGL_DrawPatch((x),(y),(p))
#define V_DrawRawScreen(a)		OGL_DrawRawScreen((a))
#endif

extern boolean	viewactive;
extern boolean	MenuActive;
extern boolean	askforquit;

static int	finalestage;		/* 0 = text, 1 = art screen */
static int	finalecount;
static int	finaletextcount;

static boolean	underwater_init;
static int	nextscroll;
static int	scroll_yval;

static int	FontABaseLump;

static const char *e1text = E1TEXT;
static const char *e2text = E2TEXT;
static const char *e3text = E3TEXT;
static const char *e4text = E4TEXT;
static const char *e5text = E5TEXT;
static const char *finaletext;
static const char *finaleflat;

static void F_TextWrite (void);
static void F_DrawBackground(void);
static void F_DemonScroll(void);
static void F_DrawUnderwater(void);
static void F_InitUnderWater(void);
static void F_KillUnderWater(void);


void F_StartFinale (void)
{
	gameaction = ga_nothing;
	gamestate = GS_FINALE;
	viewactive = false;
	automapactive = false;
	players[consoleplayer].messageTics = 1;
	players[consoleplayer].message = NULL;

	switch (gameepisode)
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
	finaletextcount = strlen(finaletext)*TEXTSPEED + TEXTWAIT;
#ifndef RENDER3D
	scroll_yval = 0;
#else
	scroll_yval = 200;
#endif
	nextscroll = 0;
	underwater_init = false;
	FontABaseLump = W_GetNumForName("FONTA_S") + 1;

	S_StartSong(mus_cptd, true);
}

boolean F_Responder (event_t *event)
{
	if (event->type != ev_keydown)
	{
		return false;
	}
	if (finalestage == 1 && gameepisode == 2)
	{
	/* we're showing the water pic,
	 * make any key kick to demo mode
	 */
		finalestage++;
		F_KillUnderWater();
		return true;
	}
	return false;
}

void F_Ticker (void)
{
	finalecount++;
	if (!finalestage && finalecount > finaletextcount)
	{
		finalecount = 0;
		finalestage = 1;
	}
}

static void F_TextWrite (void)
{
	int		count;
	const char	*ch;
	int		c;
	int		cx, cy;
	patch_t		*w;
	int		width;

	F_DrawBackground();

	cx = 20;
	cy = 5;
	ch = finaletext;

	count = (finalecount - 10)/TEXTSPEED;
	if (count < 0)
		count = 0;
	for ( ; count; count--)
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

		w = (patch_t *) W_CacheLumpNum(FontABaseLump + c - 33, PU_CACHE);
		width = SHORT(w->width);
		if (cx + width > SCREENWIDTH)
			break;
#ifdef RENDER3D
		OGL_DrawPatch (cx, cy, FontABaseLump + c - 33);
#else
		V_DrawPatch(cx, cy, w);
#endif
		cx += width;
	}
}

#if defined(RENDER3D)
static void F_DrawBackground(void)
{
/* erase the entire screen to a tiled background.
 */
	OGL_SetFlat (R_FlatNumForName(finaleflat));
	OGL_DrawRectTiled(0, 0, SCREENWIDTH, SCREENHEIGHT, 64, 64);
}

static void F_DemonScroll(void)
{
	if (finalecount < 70)
	{
		OGL_DrawRawScreen(W_GetNumForName("FINAL1"));
		nextscroll = finalecount;
		return;
	}
	if (finalecount >= nextscroll && scroll_yval > 0)
		--scroll_yval;
	if (scroll_yval > 0)
	{
		OGL_DrawRawScreenOfs(W_GetNumForName("FINAL2"), 0, -scroll_yval);
		OGL_DrawRawScreenOfs(W_GetNumForName("FINAL1"), 0, 200 - scroll_yval);
		if (finalecount >= nextscroll)
			nextscroll = finalecount + 2;	// + 3;
	}
	else
	{
		OGL_DrawRawScreen(W_GetNumForName("FINAL2"));
	}
}

static void F_InitUnderWater(void)
{
	OGL_SetPaletteLump("E2PAL");
}

static void F_KillUnderWater(void)
{
	OGL_SetPaletteLump("PLAYPAL");
}

static void F_DrawUnderwater(void)
{
	switch (finalestage)
	{
	case 1:
		paused = false;
		MenuActive = false;
		askforquit = false;

		if (!underwater_init)
		{
			underwater_init = true;
			F_InitUnderWater();
		}
		OGL_DrawRawScreen(W_GetNumForName("E2END"));
		break;

	case 2:
		OGL_DrawRawScreen(W_GetNumForName("TITLE"));
		break;
	}
}

#else	/* RENDER3D */

static void F_DrawBackground(void)
{
/* erase the entire screen to a tiled background.
 */
	int		x, y;
	byte	*src, *dest;

	src = (byte *) W_CacheLumpName(finaleflat, PU_CACHE);
	dest = screen;
	for (y = 0; y < SCREENHEIGHT; y++)
	{
		for (x = 0; x < SCREENWIDTH/64; x++)
		{
			memcpy (dest, src + ((y & 63)<<6), 64);
			dest += 64;
		}
		if (SCREENWIDTH & 63)
		{
			memcpy (dest, src + ((y & 63)<<6), SCREENWIDTH & 63);
			dest += (SCREENWIDTH & 63);
		}
	}
}

static void F_DemonScroll(void)
{
	byte *p1, *p2;

	if (finalecount < nextscroll)
	{
		return;
	}
	p1 = (byte *) W_CacheLumpName("FINAL1", PU_LEVEL);
	p2 = (byte *) W_CacheLumpName("FINAL2", PU_LEVEL);
	if (finalecount < 70)
	{
		memcpy(screen, p1, SCREENHEIGHT*SCREENWIDTH);
		nextscroll = finalecount;
		return;
	}
	if (scroll_yval < 64000)
	{
		memcpy(screen, p2 + SCREENHEIGHT*SCREENWIDTH - scroll_yval, scroll_yval);
		memcpy(screen + scroll_yval, p1, SCREENHEIGHT*SCREENWIDTH - scroll_yval);
		scroll_yval += SCREENWIDTH;
		nextscroll = finalecount + 3;
	}
	else
	{
		memcpy(screen, p2, SCREENWIDTH*SCREENHEIGHT);
	}
}

static void F_InitUnderWater(void)
{
# ifdef _WATCOMC_
	memset((byte *)0xa0000, 0, SCREENWIDTH * SCREENHEIGHT);	/* pcscreen */
# endif /* DOS */
	I_SetPalette((byte *)W_CacheLumpName("E2PAL", PU_CACHE));
}

static void F_KillUnderWater(void)
{
# ifdef _WATCOMC_
	memset((byte *)0xa0000, 0, SCREENWIDTH * SCREENHEIGHT);	/* pcscreen */
	memset(screen, 0, SCREENWIDTH * SCREENHEIGHT);
# endif /* DOS */
	I_SetPalette((byte *)W_CacheLumpName("PLAYPAL", PU_CACHE));
}

static void F_DrawUnderwater(void)
{
	switch (finalestage)
	{
	case 1:
		paused = false;
		MenuActive = false;
		askforquit = false;

		if (!underwater_init)
		{
			underwater_init = true;
			F_InitUnderWater();
			/* draw underwater picture only once during finalestage 1,
			 * no need to update it thereafter. */
			V_DrawRawScreen((byte *)W_CacheLumpName("E2END", PU_CACHE));
		}
		break;

	case 2:
		V_DrawRawScreen((byte *) W_CacheLumpName("TITLE", PU_CACHE));
		break;
	}
}
#endif	/* ! RENDER3D */

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
			if (shareware)
			  V_DrawRawScreen((BYTE_REF) WR_CacheLumpName("ORDER", PU_CACHE));
			else
			  V_DrawRawScreen((BYTE_REF) WR_CacheLumpName("CREDIT", PU_CACHE));
			break;
		case 2:
			F_DrawUnderwater();
			break;
		case 3:
			F_DemonScroll();
			break;
		case 4: /* Just show credits screen for extended episodes */
		case 5:
			V_DrawRawScreen((BYTE_REF) WR_CacheLumpName("CREDIT", PU_CACHE));
			break;
		}
	}
}

