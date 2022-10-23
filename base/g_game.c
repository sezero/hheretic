// G_game.c

#include "h2stdinc.h"
#include "doomdef.h"
#include "p_local.h"
#include "soundst.h"

// Macros

#define AM_STARTKEY		9

// Functions

boolean G_CheckDemoStatus (void);
static void G_ReadDemoTiccmd (ticcmd_t *cmd);
static void G_WriteDemoTiccmd (ticcmd_t *cmd);

void G_InitNew (skill_t skill, int episode, int map);
void G_PlayerReborn (int player);

static void G_DoReborn (int playernum);

static void G_DoLoadLevel(void);
static void G_DoNewGame(void);

void G_DoLoadGame(void);

static void G_DoPlayDemo(void);
static void G_DoCompleted(void);
static void G_DoWorldDone(void);
static void G_DoSaveGame(void);

void D_PageTicker(void);
void D_AdvanceDemo(void);

static struct
{
    mobjtype_t type;
    int speed[2];
} MonsterMissileInfo[] =
{
    { MT_IMPBALL, {10, 20} },
    { MT_MUMMYFX1, {9, 18} },
    { MT_KNIGHTAXE, {9, 18} },
    { MT_REDAXE, {9, 18} },
    { MT_BEASTBALL, {12, 20} },
    { MT_WIZFX1, {18, 24} },
    { MT_SNAKEPRO_A, {14, 20} },
    { MT_SNAKEPRO_B, {14, 20} },
    { MT_HEADFX1, {13, 20} },
    { MT_HEADFX3, {10, 18} },
    { MT_MNTRFX1, {20, 26} },
    { MT_MNTRFX2, {14, 20} },
    { MT_SRCRFX1, {20, 28} },
    { MT_SOR2FX1, {20, 28} },
    { -1, {-1, -1} } // Terminator
};

gameaction_t	gameaction;
gamestate_t	gamestate;
skill_t		gameskill;
boolean		respawnmonsters;
int		gameepisode;
int		gamemap;
int		prevmap;

boolean		paused;

boolean		usergame;		// ok to save / end game

static boolean	sendpause;		// send a pause event next tic
static boolean	sendsave;		// send a save event next tic

static boolean	timingdemo;		// if true, exit with report on completion
static int	starttime;		// for comparative timing purposes

boolean		viewactive;

boolean		deathmatch;		// only if started as net death
boolean		netgame;		// only true if packets are broadcast
boolean		playeringame[MAXPLAYERS];
player_t	players[MAXPLAYERS];

int		consoleplayer;		// player taking events and displaying
int		displayplayer;		// view being displayed
int		gametic;
int		levelstarttic;		// gametic at level start

int		totalkills, totalitems,
        totalsecret;		// for intermission
static skill_t	d_skill;
static int	d_episode;
static int	d_map;

boolean		demorecording;
boolean		demoplayback;
boolean		singledemo;		// quit after playing a demo from cmdline
static byte	*demobuffer, *demo_p;
static const char	*defdemoname;
static char	demoname[MAX_OSPATH];

static short	consistancy[MAXPLAYERS][BACKUPTICS];

static int	loadgameslot;
static int	savegameslot;
static char	savedescription[32];

boolean		precache = true;	// if true, load all graphics at start

//
// controls (have defaults)
//
int	key_right, key_left, key_up, key_down;
int	key_strafeleft, key_straferight;
int	key_fire, key_use, key_strafe, key_speed;
int	key_flyup, key_flydown, key_flycenter;
int	key_lookup, key_lookdown, key_lookcenter;
int	key_invleft, key_invright, key_useartifact;

int	mouselook;
int	alwaysrun;	/* boolean */

int	mousebfire;
int	mousebstrafe;
int	mousebforward;

int	joybfire;
int	joybstrafe;
int	joybuse;
int	joybspeed;

#define MAXPLMOVE		0x32

static fixed_t	forwardmove[2] = {0x19, 0x32};
static fixed_t	sidemove[2] = {0x18, 0x28};
static fixed_t	angleturn[3] = {640, 1280, 320};	// + slow turn
#define SLOWTURNTICS		6

boolean	gamekeydown[MAXKEYS];
static int	turnheld;			// for accelerative turning
static int	lookheld;

static boolean	mousearray[4];
static boolean	*mousebuttons = &mousearray[1];	// allow [-1]

static int	mousex, mousey;			// mouse values are used once
static int	dclicktime, dclickstate, dclicks;
static int	dclicktime2, dclickstate2, dclicks2;

static int	joyxmove, joyymove;		// joystick values are repeated
static boolean	joyarray[5];
static boolean	*joybuttons = &joyarray[1];	// allow [-1]

static int	inventoryTics;

boolean		usearti = true;

#if defined(__WATCOMC__) && defined(_DOS)
extern externdata_t *i_ExternData;
#endif

//=============================================================================

/*
====================
=
= G_BuildTiccmd
=
= Builds a ticcmd from all of the available inputs or reads it from the
= demo buffer.
= If recording a demo, write it out
====================
*/

extern boolean inventory;
extern boolean noartiskip;
extern int curpos;
extern int inv_ptr;

extern int isCyberPresent;	// is CyberMan present?
void I_ReadCyberCmd (ticcmd_t *cmd);

void G_BuildTiccmd (ticcmd_t *cmd)
{
    int	i;
    boolean	strafe, bstrafe;
    int	speed, tspeed, lspeed;
    int	forward, side;
    int	look, arti;
    int	flyheight;

    memset (cmd, 0, sizeof(*cmd));
//	cmd->consistancy =
//		consistancy[consoleplayer][(maketic*ticdup)%BACKUPTICS];
    cmd->consistancy =
        consistancy[consoleplayer][maketic%BACKUPTICS];

//printf ("cons: %i\n",cmd->consistancy);
    strafe = gamekeydown[key_strafe] || mousebuttons[mousebstrafe] || joybuttons[joybstrafe];
    speed = gamekeydown[key_speed] || joybuttons[joybspeed] || joybuttons[joybspeed];

    if (alwaysrun && !demoplayback && !demorecording)
        speed = !speed;
    forward = side = look = arti = flyheight = 0;

//
// use two stage accelerative turning on the keyboard and joystick
//
    if (joyxmove < 0 || joyxmove > 0 || gamekeydown[key_right] || gamekeydown[key_left])
        turnheld += ticdup;
    else
        turnheld = 0;
    if (turnheld < SLOWTURNTICS)
        tspeed = 2;		// slow turn
    else
        tspeed = speed;

    if (gamekeydown[key_lookdown] || gamekeydown[key_lookup])
    {
        lookheld += ticdup;
    }
    else
    {
        lookheld = 0;
    }
    if (lookheld < SLOWTURNTICS)
    {
        lspeed = 1;
    }
    else
    {
        lspeed = 2;
    }

//
// let movement keys cancel each other out
//
    if (strafe)
    {
        if (gamekeydown[key_right])
            side += sidemove[speed];
        if (gamekeydown[key_left])
            side -= sidemove[speed];
        if (joyxmove > 0)
            side += sidemove[speed];
        if (joyxmove < 0)
            side -= sidemove[speed];
    }
    else
    {
        if (gamekeydown[key_right])
            cmd->angleturn -= angleturn[tspeed];
        if (gamekeydown[key_left])
            cmd->angleturn += angleturn[tspeed];
        if (joyxmove > 0)
            cmd->angleturn -= angleturn[tspeed];
        if (joyxmove < 0)
            cmd->angleturn += angleturn[tspeed];
    }

    if (gamekeydown[key_up])
        forward += forwardmove[speed];
    if (gamekeydown[key_down])
        forward -= forwardmove[speed];
    if (joyymove < 0)
        forward += forwardmove[speed];
    if (joyymove > 0)
        forward -= forwardmove[speed];
    if (gamekeydown[key_straferight])
        side += sidemove[speed];
    if (gamekeydown[key_strafeleft])
        side -= sidemove[speed];

    // Look up/down/center keys
    if (gamekeydown[key_lookup])
    {
        look = lspeed;
    }
    if (gamekeydown[key_lookdown])
    {
        look = -lspeed;
    }
    if (gamekeydown[key_lookcenter])
    {
        look = TOCENTER;
    }
    // Fly up/down/drop keys
    if (gamekeydown[key_flyup])
    {
        flyheight = 5; // note that the actual flyheight will be twice this
    }
    if (gamekeydown[key_flydown])
    {
        flyheight = -5;
    }
    if (gamekeydown[key_flycenter])
    {
        flyheight = TOCENTER;
        look = TOCENTER;
    }

    // Use artifact key
    if (gamekeydown[key_useartifact])
    {
        if (gamekeydown[key_speed] && !noartiskip)
        {
            if (players[consoleplayer].inventory[inv_ptr].type != arti_none)
            {
                gamekeydown[key_useartifact] = false;
                cmd->arti = 0xff; // skip artifact code
            }
        }
        else
        {
            if (inventory)
            {
                players[consoleplayer].readyArtifact =
                    players[consoleplayer].inventory[inv_ptr].type;
                inventory = false;
                cmd->arti = 0;
                usearti = false;
            }
            else if (usearti)
            {
                cmd->arti = players[consoleplayer].inventory[inv_ptr].type;
                usearti = false;
            }
        }
    }
    if (gamekeydown[127] && !cmd->arti && !players[consoleplayer].powers[pw_weaponlevel2])
    {
        gamekeydown[127] = false;
        cmd->arti = arti_tomeofpower;
    }

//
// buttons
//
    cmd->chatchar = CT_dequeueChatChar();
    if (gamekeydown[key_fire] || mousebuttons[mousebfire]
                  || joybuttons[joybfire])
        cmd->buttons |= BT_ATTACK;

    if (gamekeydown[key_use] || joybuttons[joybuse] )
    {
        cmd->buttons |= BT_USE;
        dclicks = 0;		// clear double clicks if hit use button
    }

    for (i = 0; i < NUMWEAPONS-2; i++)
    {
        if (gamekeydown['1'+i])
        {
            cmd->buttons |= BT_CHANGE;
            cmd->buttons |= i<<BT_WEAPONSHIFT;
            break;
        }
    }

//
// mouse
//
    if (mousebuttons[mousebforward])
    {
        forward += forwardmove[speed];
    }

//
// forward double click
//
    if (mousebuttons[mousebforward] != dclickstate && dclicktime > 1)
    {
        dclickstate = mousebuttons[mousebforward];
        if (dclickstate)
            dclicks++;
        if (dclicks == 2)
        {
            cmd->buttons |= BT_USE;
            dclicks = 0;
        }
        else
            dclicktime = 0;
    }
    else
    {
        dclicktime += ticdup;
        if (dclicktime > 20)
        {
            dclicks = 0;
            dclickstate = 0;
        }
    }

//
// strafe double click
//
    bstrafe = mousebuttons[mousebstrafe] || joybuttons[joybstrafe];
    if (bstrafe != dclickstate2 && dclicktime2 > 1 )
    {
        dclickstate2 = bstrafe;
        if (dclickstate2)
            dclicks2++;
        if (dclicks2 == 2)
        {
            cmd->buttons |= BT_USE;
            dclicks2 = 0;
        }
        else
            dclicktime2 = 0;
    }
    else
    {
        dclicktime2 += ticdup;
        if (dclicktime2 > 20)
        {
            dclicks2 = 0;
            dclickstate2 = 0;
        }
    }

    if (strafe)
    {
        side += mousex*2;
    }
    else
    {
        cmd->angleturn -= mousex*0x8;
    }

    if (demorecording || demoplayback || (mouselook == 0))
    {
        forward += mousey;
    }
    else if (mousey && !paused)	/* mouselook, but not when paused */
    {
        /* We'll directly change the viewing pitch of the console player. */
        float adj = ((mousey*0x4) << 16) / (float) ANGLE_180*180*110.0/85.0;
        float newlookdir = 0;

        adj *= 2;	/* Speed up the X11 mlook a little. */

        if (mouselook == 1)
            newlookdir = players[consoleplayer].lookdir + adj;
        else if (mouselook == 2)
            newlookdir = players[consoleplayer].lookdir - adj;

        // vertical view angle taken from p_user.c line 249.
        if (newlookdir > 90)
            newlookdir = 90;
        else if (newlookdir < -110)
            newlookdir = -110;

        players[consoleplayer].lookdir = newlookdir;
    }

    mousex = mousey = 0;

    if (forward > MAXPLMOVE)
        forward = MAXPLMOVE;
    else if (forward < -MAXPLMOVE)
        forward = -MAXPLMOVE;
    if (side > MAXPLMOVE)
        side = MAXPLMOVE;
    else if (side < -MAXPLMOVE)
        side = -MAXPLMOVE;

    cmd->forwardmove += forward;
    cmd->sidemove += side;
    if (players[consoleplayer].playerstate == PST_LIVE)
    {
        if (look < 0)
        {
            look += 16;
        }
        cmd->lookfly = look;
    }
    if (flyheight < 0)
    {
        flyheight += 16;
    }
    cmd->lookfly |= flyheight<<4;

//
// special buttons
//
    if (sendpause)
    {
        sendpause = false;
        cmd->buttons = BT_SPECIAL | BTS_PAUSE;
    }

    if (sendsave)
    {
        sendsave = false;
        cmd->buttons = BT_SPECIAL | BTS_SAVEGAME | (savegameslot<<BTS_SAVESHIFT);
    }
}


/*
==============
=
= G_DoLoadLevel
=
==============
*/

static void G_DoLoadLevel (void)
{
    int		i;

    levelstarttic = gametic;	// for time calculation
    gamestate = GS_LEVEL;
    for (i = 0; i < MAXPLAYERS; i++)
    {
        if (playeringame[i] && players[i].playerstate == PST_DEAD)
            players[i].playerstate = PST_REBORN;
        memset (players[i].frags, 0, sizeof(players[i].frags));
    }

    P_SetupLevel (gameepisode, gamemap, 0, gameskill);
    displayplayer = consoleplayer;	// view the guy you are playing
    starttime = I_GetTime ();
    gameaction = ga_nothing;
    Z_CheckHeap ();

//
// clear cmd building stuff
//
    memset (gamekeydown, 0, sizeof(gamekeydown));
    joyxmove = joyymove = 0;
    mousex = mousey = 0;
    sendpause = sendsave = paused = false;
//	memset (mousebuttons, 0, sizeof(mousebuttons));
//	memset (joybuttons, 0, sizeof(joybuttons));
    memset (joyarray, 0, sizeof(joyarray));
    memset (mousearray, 0, sizeof(mousearray));
}


/*
===============================================================================
=
= G_Responder
=
= get info needed to make ticcmd_ts for the players
=
===============================================================================
*/

boolean G_Responder(event_t *ev)
{
    player_t *plr;
    extern boolean MenuActive;

    plr = &players[consoleplayer];
    if (ev->type == ev_keyup && ev->data1 == key_useartifact)
    { // flag to denote that it's okay to use an artifact
        if (!inventory)
        {
            plr->readyArtifact = plr->inventory[inv_ptr].type;
        }
        usearti = true;
    }

    // Check for spy mode player cycle
    if  (gamestate == GS_LEVEL && ev->type == ev_keydown
            && ev->data1 == KEY_F12 && !deathmatch)
    { // Cycle the display player
        do
        {
            displayplayer++;
            if (displayplayer == MAXPLAYERS)
            {
                displayplayer = 0;
            }
        }
        while (!playeringame[displayplayer] && displayplayer != consoleplayer);
        return true;
    }

    if (gamestate == GS_LEVEL)
    {
        if (CT_Responder(ev))
        { // Chat ate the event
            return true;
        }
        if (SB_Responder(ev))
        { // Status bar ate the event
            return true;
        }
        if(AM_Responder(ev))
        { // Automap ate the event
            return true;
        }
    }

    switch (ev->type)
    {
    case ev_keydown:
        if (ev->data1 == key_invleft)
        {
            inventoryTics = 5*35;
            if (!inventory)
            {
                inventory = true;
                break;
            }
            inv_ptr--;
            if (inv_ptr < 0)
            {
                inv_ptr = 0;
            }
            else
            {
                curpos--;
                if (curpos < 0)
                {
                    curpos = 0;
                }
            }
            return true;
        }
        if (ev->data1 == key_invright)
        {
            inventoryTics = 5*35;
            if (!inventory)
            {
                inventory = true;
                break;
            }
            inv_ptr++;
            if (inv_ptr >= plr->inventorySlotNum)
            {
                inv_ptr--;
                if (inv_ptr < 0)
                    inv_ptr = 0;
            }
            else
            {
                curpos++;
                if (curpos > 6)
                {
                    curpos = 6;
                }
            }
            return true;
        }
        if (ev->data1 == KEY_PAUSE)
        {
            if (!MenuActive && gamestate != GS_FINALE)
                sendpause = true;
            return true;
        }
        if (ev->data1 < MAXKEYS)
        {
            gamekeydown[ev->data1] = true;
        }
        return true;	// eat key down events

    case ev_keyup:
        if (ev->data1 < MAXKEYS)
        {
            gamekeydown[ev->data1] = false;
        }
        return false;	// always let key up events filter down

    case ev_mouse:
        mousebuttons[0] = ev->data1 & 1;
        mousebuttons[1] = ev->data1 & 2;
        mousebuttons[2] = ev->data1 & 4;
        mousex = ev->data2 * (mouseSensitivity + 5) / 10;
        mousey = ev->data3 * (mouseSensitivity + 5) / 10;
        return true;	// eat events

    case ev_joystick:
        joybuttons[0] = ev->data1 & 1;
        joybuttons[1] = ev->data1 & 2;
        joybuttons[2] = ev->data1 & 4;
        joybuttons[3] = ev->data1 & 8;
        joyxmove = ev->data2;
        joyymove = ev->data3;
        return true;	// eat events

    default:
        break;
    }
    return false;
}

/*
===============================================================================
=
= G_Ticker
=
===============================================================================
*/

void G_Ticker(void)
{
    int		i, buf;
    ticcmd_t	*cmd = NULL;

//
// do player reborns if needed
//
    for (i = 0; i < MAXPLAYERS; i++)
    {
        if (playeringame[i] && players[i].playerstate == PST_REBORN)
            G_DoReborn (i);
    }

//
// do things to change the game state
//
    while (gameaction != ga_nothing)
    {
        switch (gameaction)
        {
        case ga_loadlevel:
            G_DoLoadLevel();
            break;
        case ga_newgame:
            G_DoNewGame();
            break;
        case ga_loadgame:
            G_DoLoadGame();
            break;
        case ga_savegame:
            G_DoSaveGame();
            break;
        case ga_playdemo:
            G_DoPlayDemo();
            break;
        case ga_screenshot:
            M_ScreenShot();
            gameaction = ga_nothing;
            break;
        case ga_completed:
            G_DoCompleted();
            break;
        case ga_worlddone:
            G_DoWorldDone();
            break;
        case ga_victory:
            F_StartFinale();
            break;
        default:
            break;
        }
    }

//
// get commands, check consistancy, and build new consistancy check
//
    //buf = gametic % BACKUPTICS;
    buf = (gametic / ticdup) % BACKUPTICS;

    for (i = 0; i < MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            cmd = &players[i].cmd;

            memcpy (cmd, &netcmds[i][buf], sizeof(ticcmd_t));

            if (demoplayback)
                G_ReadDemoTiccmd (cmd);
            if (demorecording)
                G_WriteDemoTiccmd (cmd);

            if (netgame && !(gametic%ticdup))
            {
                if (gametic > BACKUPTICS && consistancy[i][buf] != cmd->consistancy)
                {
                    I_Error ("consistency failure (%i should be %i)",cmd->consistancy, consistancy[i][buf]);
                }
                if (players[i].mo)
                    consistancy[i][buf] = players[i].mo->x;
                else
                    consistancy[i][buf] = rndindex;
            }
        }
    }

//
// check for special buttons
//
    for (i = 0; i < MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            if (players[i].cmd.buttons & BT_SPECIAL)
            {
                switch (players[i].cmd.buttons & BT_SPECIALMASK)
                {
                case BTS_PAUSE:
                    paused ^= 1;
                    if (paused)
                    {
                        S_PauseSound();
                    }
                    else
                    {
                        S_ResumeSound();
                    }
                    break;

                case BTS_SAVEGAME:
                    if (!savedescription[0])
                    {
                        if (netgame)
                        {
                            strcpy (savedescription, "NET GAME");
                        }
                        else
                        {
                            strcpy(savedescription, "SAVE GAME");
                        }
                    }
                    savegameslot =
                        (players[i].cmd.buttons & BTS_SAVEMASK)>>BTS_SAVESHIFT;
                    gameaction = ga_savegame;
                    break;
                }
            }
        }
    }

// turn inventory off after a certain amount of time
    if (inventory && !(--inventoryTics))
    {
        players[consoleplayer].readyArtifact =
            players[consoleplayer].inventory[inv_ptr].type;
        inventory = false;
        cmd->arti = 0;
    }

//
// do main actions
//
    switch (gamestate)
    {
    case GS_LEVEL:
        P_Ticker ();
        SB_Ticker ();
        AM_Ticker ();
        CT_Ticker();
        break;
    case GS_INTERMISSION:
        IN_Ticker ();
        break;
    case GS_FINALE:
        F_Ticker();
        break;
    case GS_DEMOSCREEN:
        D_PageTicker ();
        break;
    }
}


/*
==============================================================================

                        PLAYER STRUCTURE FUNCTIONS

also see P_SpawnPlayer in P_Things
==============================================================================
*/

/*
====================
=
= G_InitPlayer
=
= Called at the start
= Called by the game initialization functions
====================
*/

void G_InitPlayer (int player)
{
//	player_t	*p;

// set up the saved info
//	p = &players[player];

// clear everything else to defaults
    G_PlayerReborn (player);
}

/*
====================
=
= G_PlayerFinishLevel
=
= Can when a player completes a level
====================
*/
extern int playerkeys;

void G_PlayerFinishLevel(int player)
{
    player_t *p;
    int i;

/*      // BIG HACK
    inv_ptr = 0;
    curpos = 0;
*/
    // END HACK
    p = &players[player];
    for (i = 0; i < p->inventorySlotNum; i++)
    {
        p->inventory[i].count = 1;
    }
    p->artifactCount = p->inventorySlotNum;

    if (!deathmatch)
    {
        for (i = 0; i < 16; i++)
        {
            P_PlayerUseArtifact(p, arti_fly);
        }
    }
    memset(p->powers, 0, sizeof(p->powers));
    memset(p->keys, 0, sizeof(p->keys));
    playerkeys = 0;
//	memset(p->inventory, 0, sizeof(p->inventory));
    if (p->chickenTics)
    {
        p->readyweapon = p->mo->special1; // Restore weapon
        p->chickenTics = 0;
    }
    p->messageTics = 0;
    p->lookdir = 0;
    p->mo->flags &= ~MF_SHADOW; // Remove invisibility
    p->extralight = 0; // Remove weapon flashes
    p->fixedcolormap = 0; // Remove torch
    p->damagecount = 0; // No palette changes
    p->bonuscount = 0;
    p->rain1 = NULL;
    p->rain2 = NULL;
    if (p == &players[consoleplayer])
    {
        SB_state = -1; // refresh the status bar
    }
}

/*
====================
=
= G_PlayerReborn
=
= Called after a player dies
= almost everything is cleared and initialized
====================
*/

void G_PlayerReborn(int player)
{
    player_t *p;
    int i;
    int frags[MAXPLAYERS];
    int killcount, itemcount, secretcount;
    boolean secret;

    secret = false;
    memcpy(frags, players[player].frags, sizeof(frags));
    killcount = players[player].killcount;
    itemcount = players[player].itemcount;
    secretcount = players[player].secretcount;

    p = &players[player];
    if (p->didsecret)
    {
        secret = true;
    }
    memset(p, 0, sizeof(*p));

    memcpy(players[player].frags, frags, sizeof(players[player].frags));
    players[player].killcount = killcount;
    players[player].itemcount = itemcount;
    players[player].secretcount = secretcount;

    p->usedown = p->attackdown = true;	// don't do anything immediately
    p->playerstate = PST_LIVE;
    p->health = MAXHEALTH;
    p->readyweapon = p->pendingweapon = wp_goldwand;
    p->weaponowned[wp_staff] = true;
    p->weaponowned[wp_goldwand] = true;
    p->messageTics = 0;
    p->lookdir = 0;
    p->ammo[am_goldwand] = 50;
    for (i = 0; i < NUMAMMO; i++)
    {
        p->maxammo[i] = maxammo[i];
    }
    if (gamemap == 9 || secret)
    {
        p->didsecret = true;
    }
    if (p == &players[consoleplayer])
    {
        SB_state = -1;	// refresh the status bar
        inv_ptr = 0;	// reset the inventory pointer
        curpos = 0;
    }
}

/*
====================
=
= G_CheckSpot
=
= Returns false if the player cannot be respawned at the given mapthing_t spot
= because something is occupying it
====================
*/

void P_SpawnPlayer (mapthing_t *mthing);

boolean G_CheckSpot (int playernum, mapthing_t *mthing)
{
    fixed_t		x, y;
    subsector_t	*ss;
    unsigned int	an;
    mobj_t		*mo;

    x = mthing->x << FRACBITS;
    y = mthing->y << FRACBITS;

    players[playernum].mo->flags2 &= ~MF2_PASSMOBJ;
    if (! P_CheckPosition(players[playernum].mo, x, y))
    {
        players[playernum].mo->flags2 |= MF2_PASSMOBJ;
        return false;
    }
    players[playernum].mo->flags2 |= MF2_PASSMOBJ;

// spawn a teleport fog
    ss = R_PointInSubsector (x, y);
    an = (ANG45 * (mthing->angle / 45)) >> ANGLETOFINESHIFT;

    mo = P_SpawnMobj (x + 20*finecosine[an], y + 20*finesine[an],
        ss->sector->floorheight + TELEFOGHEIGHT, MT_TFOG);

    if (players[consoleplayer].viewz != 1)
        S_StartSound (mo, sfx_telept);	// don't start sound on first frame

    return true;
}

/*
====================
=
= G_DeathMatchSpawnPlayer
=
= Spawns a player at one of the random death match spots
= called at level load and each death
====================
*/

void G_DeathMatchSpawnPlayer (int playernum)
{
    int		i, j;
    int		selections;

    selections = deathmatch_p - deathmatchstarts;
    if (selections < 4)
        I_Error ("Only %i deathmatch spots, 4 required", selections);

    for (j = 0; j < 20; j++)
    {
        i = P_Random() % selections;
        if (G_CheckSpot (playernum, &deathmatchstarts[i]))
        {
            deathmatchstarts[i].type = playernum + 1;
            P_SpawnPlayer (&deathmatchstarts[i]);
            return;
        }
    }

// no good spot, so the player will probably get stuck
    P_SpawnPlayer (&playerstarts[playernum]);
}

/*
====================
=
= G_DoReborn
=
====================
*/

static void G_DoReborn(int playernum)
{
    int i;

    if (G_CheckDemoStatus())
        return;
    if (!netgame)
        gameaction = ga_loadlevel;		// reload the level from scratch
    else
    {       // respawn at the start
        players[playernum].mo->player = NULL;	// dissasociate the corpse

        // spawn at random spot if in death match
        if (deathmatch)
        {
            G_DeathMatchSpawnPlayer(playernum);
            return;
        }

        if (G_CheckSpot(playernum, &playerstarts[playernum]))
        {
            P_SpawnPlayer(&playerstarts[playernum]);
            return;
        }
        // try to spawn at one of the other players spots
        for (i = 0; i < MAXPLAYERS; i++)
            if (G_CheckSpot(playernum, &playerstarts[i]))
            {
                playerstarts[i].type = playernum + 1;	// fake as other player
                P_SpawnPlayer(&playerstarts[i]);
                playerstarts[i].type = i + 1;		// restore
                return;
            }
        // he's going to be inside something.  Too bad.
        P_SpawnPlayer(&playerstarts[playernum]);
    }
}


void G_ScreenShot (void)
{
    gameaction = ga_screenshot;
}


/*
====================
=
= G_DoCompleted
=
====================
*/

boolean secretexit;
void G_ExitLevel (void)
{
    secretexit = false;
    gameaction = ga_completed;
}
void G_SecretExitLevel (void)
{
    secretexit = true;
    gameaction = ga_completed;
}

static void G_DoCompleted(void)
{
    int i;
    static int afterSecret[5] = { 7, 5, 5, 5, 4 };

    gameaction = ga_nothing;
    if (G_CheckDemoStatus())
    {
        return;
    }
    for (i = 0; i < MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            G_PlayerFinishLevel(i);
        }
    }
    prevmap = gamemap;
    if (secretexit == true)
    {
        gamemap = 9;
    }
    else if (gamemap == 9)
    { // Finished secret level
        gamemap = afterSecret[gameepisode - 1];
    }
    else if (gamemap == 8)
    {
        gameaction = ga_victory;
        return;
    }
    else
    {
        gamemap++;
    }
    gamestate = GS_INTERMISSION;
    IN_Start();
}

//============================================================================
//
// G_WorldDone
//
//============================================================================

void G_WorldDone(void)
{
    gameaction = ga_worlddone;
}

//============================================================================
//
// G_DoWorldDone
//
//============================================================================

static void G_DoWorldDone(void)
{
    gamestate = GS_LEVEL;
    G_DoLoadLevel();
    gameaction = ga_nothing;
    viewactive = true;
}

//---------------------------------------------------------------------------
//
// PROC G_LoadGame
//
// Can be called by the startup code or the menu task.
//
//---------------------------------------------------------------------------

void G_LoadGame(int slot)
{
    loadgameslot = slot;
    gameaction = ga_loadgame;
}

//---------------------------------------------------------------------------
//
// PROC G_DoLoadGame
//
// Called by G_Ticker based on gameaction.
//
//---------------------------------------------------------------------------

void G_DoLoadGame(void)
{
    gameaction = ga_nothing;
    SV_LoadGame(loadgameslot);
}


/*
====================
=
= G_InitNew
=
= Can be called by the startup code or the menu task
= consoleplayer, displayplayer, playeringame[] should be set
====================
*/

void G_DeferedInitNew (skill_t skill, int episode, int map)
{
    d_skill = skill;
    d_episode = episode;
    d_map = map;
    gameaction = ga_newgame;
}

static void G_DoNewGame (void)
{
    G_InitNew (d_skill, d_episode, d_map);
    gameaction = ga_nothing;
}

void G_InitNew(skill_t skill, int episode, int map)
{
    int i;
    int speed;
    static const char *skyLumpNames[5] =
    {
        "SKY1", "SKY2", "SKY3", "SKY1", "SKY3"
    };

    if (paused)
    {
        paused = false;
        S_ResumeSound();
    }
    if (skill < sk_baby)
        skill = sk_baby;
    if (skill > sk_nightmare)
        skill = sk_nightmare;
    if (episode < 1)
        episode = 1;
    // Up to 9 episodes for testing
    if (episode > 9)
        episode = 9;
    if (map < 1)
        map = 1;
    if (map > 9)
        map = 9;
    M_ClearRandom();
    if (respawnparm)
    {
        respawnmonsters = true;
    }
    else
    {
        respawnmonsters = false;
    }
    // Set monster missile speeds
    speed = skill == sk_nightmare;
    for (i = 0; MonsterMissileInfo[i].type != -1; i++)
    {
        mobjinfo[MonsterMissileInfo[i].type].speed
            = MonsterMissileInfo[i].speed[speed]<<FRACBITS;
    }
    // Force players to be initialized upon first level load
    for (i = 0; i < MAXPLAYERS; i++)
    {
        players[i].playerstate = PST_REBORN;
        players[i].didsecret = false;
    }
    // Set up a bunch of globals
    usergame = true;	// will be set false if a demo
    paused = false;
    demorecording = false;
    demoplayback = false;
    viewactive = true;
    gameepisode = episode;
    gamemap = map;
    gameskill = skill;
    viewactive = true;
    BorderNeedRefresh = true;

    // Set the sky map
    if (episode > 5)
    {
        skytexture = R_TextureNumForName("SKY1");
    }
    else
    {
        skytexture = R_TextureNumForName(skyLumpNames[episode-1]);
    }

//
// give one null ticcmd_t
//
#if 0
    gametic = 0;
    maketic = 1;
    for (i = 0; i < MAXPLAYERS; i++)
        nettics[i] = 1;		// one null event for this gametic
    memset (localcmds,0,sizeof(localcmds));
    memset (netcmds,0,sizeof(netcmds));
#endif
    G_DoLoadLevel();
}


/*
===============================================================================

                            DEMO RECORDING

===============================================================================
*/

#define DEMOMARKER	0x80

static void G_ReadDemoTiccmd (ticcmd_t *cmd)
{
    if (*demo_p == DEMOMARKER)
    {	// end of demo data stream
        G_CheckDemoStatus ();
        return;
    }
    cmd->forwardmove = ((signed char)*demo_p++);
    cmd->sidemove = ((signed char)*demo_p++);
    cmd->angleturn = ((unsigned char)*demo_p++)<<8;
    cmd->buttons = (unsigned char)*demo_p++;
    cmd->lookfly = (unsigned char)*demo_p++;
    cmd->arti = (unsigned char)*demo_p++;
}

static void G_WriteDemoTiccmd (ticcmd_t *cmd)
{
    if (gamekeydown['q'])		// press q to end demo recording
        G_CheckDemoStatus ();
    *demo_p++ = (byte) cmd->forwardmove;
    *demo_p++ = (byte) cmd->sidemove;
    *demo_p++ = cmd->angleturn>>8;
    *demo_p++ = cmd->buttons;
    *demo_p++ = cmd->lookfly;
    *demo_p++ = cmd->arti;
    demo_p -= 6;
    G_ReadDemoTiccmd (cmd);		// make SURE it is exactly the same
}


/*
===================
=
= G_RecordDemo
=
===================
*/

void G_RecordDemo (skill_t skill, int numplayers, int episode, int map, const char *name)
{
    int		i;

    G_InitNew (skill, episode, map);
    usergame = false;
    snprintf (demoname, sizeof(demoname), "%s%s.lmp", basePath, name);
    demobuffer = demo_p = (byte *) Z_Malloc (0x20000, PU_STATIC, NULL);
    *demo_p++ = skill;
    *demo_p++ = episode;
    *demo_p++ = map;

    for (i = 0; i < MAXPLAYERS; i++)
        *demo_p++ = playeringame[i];

    demorecording = true;
}


/*
===================
=
= G_PlayDemo
=
===================
*/

void G_DeferedPlayDemo (const char *name)
{
    defdemoname = name;
    gameaction = ga_playdemo;
}

static void G_DoPlayDemo (void)
{
    skill_t	skill;
    int	i, episode, map;

    gameaction = ga_nothing;
    demobuffer = demo_p = (byte *) W_CacheLumpName (defdemoname, PU_STATIC);
    skill = *demo_p++;
    episode = *demo_p++;
    map = *demo_p++;

    for (i = 0; i < MAXPLAYERS; i++)
        playeringame[i] = *demo_p++;

    precache = false;		// don't spend a lot of time in loadlevel
    G_InitNew (skill, episode, map);
    precache = true;
    usergame = false;
    demoplayback = true;
}


/*
===================
=
= G_TimeDemo
=
===================
*/

void G_TimeDemo (const char *name)
{
    skill_t	skill;
    int	episode, map;

    demobuffer = demo_p = (byte *) W_CacheLumpName (name, PU_STATIC);
    skill = *demo_p++;
    episode = *demo_p++;
    map = *demo_p++;
    G_InitNew (skill, episode, map);
    usergame = false;
    demoplayback = true;
    timingdemo = true;
    singletics = true;
}


/*
===================
=
= G_CheckDemoStatus
=
= Called after a death or level completion to allow demos to be cleaned up
= Returns true if a new demo loop action will take place
===================
*/

boolean G_CheckDemoStatus (void)
{
    int		endtime;

    if (timingdemo)
    {
        endtime = I_GetTime ();
        I_Error ("timed %i gametics in %i realtics", gametic,
                        endtime - starttime);
    }

    if (demoplayback)
    {
        if (singledemo)
            I_Quit ();

        Z_ChangeTag (demobuffer, PU_CACHE);
        demoplayback = false;
        D_AdvanceDemo ();
        return true;
    }

    if (demorecording)
    {
        *demo_p++ = DEMOMARKER;
        M_WriteFile (demoname, demobuffer, demo_p - demobuffer);
        Z_Free (demobuffer);
        demorecording = false;
        I_Error ("Recorded demo: %s", demoname);
    }

    return false;
}

/**************************************************************************/

//==========================================================================
//
// G_SaveGame
//
// Called by the menu task.  <description> is a 24 byte text string.
//
//==========================================================================

void G_SaveGame(int slot, const char *description)
{
    savegameslot = slot;
    strcpy(savedescription, description);
    sendsave = true;
}

//==========================================================================
//
// G_DoSaveGame
//
// Called by G_Ticker based on gameaction.
//
//==========================================================================

static void G_DoSaveGame(void)
{
    SV_SaveGame(savegameslot, savedescription);
    gameaction = ga_nothing;
    savedescription[0] = 0;
    P_SetMessage(&players[consoleplayer], TXT_GAMESAVED, true);
}

