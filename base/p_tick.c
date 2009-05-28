
// P_tick.c
// $Revision$
// $Date$

#include "h2stdinc.h"
#include "doomdef.h"
#include "p_local.h"

int leveltime;
int TimerGame;

thinker_t	thinkercap;	// both the head and tail of the thinker list


/*
===============================================================================

								THINKERS

All thinkers should be allocated by Z_Malloc so they can be operated on uniformly.  The actual
structures will vary in size, but the first element must be thinker_t.

===============================================================================
*/

/*
===============
=
= P_InitThinkers
=
===============
*/

void P_InitThinkers (void)
{
	thinkercap.prev = thinkercap.next  = &thinkercap;
}


/*
===============
=
= P_AddThinker
=
= Adds a new thinker at the end of the list
=
===============
*/

void P_AddThinker (thinker_t *thinker)
{
	thinkercap.prev->next = thinker;
	thinker->next = &thinkercap;
	thinker->prev = thinkercap.prev;
	thinkercap.prev = thinker;
}

/*
===============
=
= P_RemoveThinker
=
= Deallocation is lazy -- it will not actually be freed until its
= thinking turn comes up
=
===============
*/

void P_RemoveThinker (thinker_t *thinker)
{
	thinker->function = (think_t)-1;
}

/*
===============
=
= P_AllocateThinker
=
= Allocates memory and adds a new thinker at the end of the list
=
===============
*/

void P_AllocateThinker (thinker_t *thinker)
{
}


/*
===============
=
= P_RunThinkers
=
===============
*/

void P_RunThinkers (void)
{
	thinker_t	*currentthinker;

	currentthinker = thinkercap.next;
	while (currentthinker != &thinkercap)
	{
		if (currentthinker->function == (think_t)-1)
		{	// time to remove it
			currentthinker->next->prev = currentthinker->prev;
			currentthinker->prev->next = currentthinker->next;
			Z_Free (currentthinker);
		}
		else
		{
			if (currentthinker->function)
				currentthinker->function (currentthinker);
		}
		currentthinker = currentthinker->next;
	}
}

//----------------------------------------------------------------------------
//
// PROC P_Ticker
//
//----------------------------------------------------------------------------

void P_Ticker(void)
{
	int i;

	if(paused)
	{
		return;
	}
	for(i = 0; i < MAXPLAYERS; i++)
	{
		if(playeringame[i])
		{
			P_PlayerThink(&players[i]);
		}
	}
	if(TimerGame)
	{
		if(!--TimerGame)
		{
			G_ExitLevel();
		}
	}
	P_RunThinkers();
	P_UpdateSpecials();
	P_AmbientSound();
	leveltime++;
}

