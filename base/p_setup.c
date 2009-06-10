
// P_main.c
// $Revision$
// $Date$

// HEADER FILES ------------------------------------------------------------

#include "h2stdinc.h"
#include <math.h>
#include "doomdef.h"
#include "p_local.h"
#include "soundst.h"
#ifdef RENDER3D
#include "ogl_def.h"
#endif

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

void P_SpawnMapThing(mapthing_t *mthing);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

#ifdef RENDER3D
static float P_AccurateDistance(fixed_t dx, fixed_t dy);
#endif

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

mapthing_t	deathmatchstarts[10], *deathmatch_p;
mapthing_t	playerstarts[MAXPLAYERS];

int		numvertexes;
vertex_t	*vertexes;

int		numsegs;
seg_t		*segs;

int		numsectors;
sector_t	*sectors;

int		numsubsectors;
subsector_t	*subsectors;

int		numnodes;
node_t		*nodes;

int		numlines;
line_t		*lines;

int		numsides;
side_t		*sides;

short		*blockmaplump;		// offsets in blockmap are from here
short		*blockmap;
int		bmapwidth, bmapheight;	// in mapblocks
fixed_t		bmaporgx, bmaporgy;	// origin of block map
mobj_t		**blocklinks;		// for thing chains
byte		*rejectmatrix;		// for fast sight rejection

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------


/*
=================
=
= P_LoadVertexes
=
=================
*/

static void P_LoadVertexes (int lump)
{
	void		*data;
	int		i;
	mapvertex_t	*ml;
	vertex_t	*li;

	numvertexes = W_LumpLength (lump) / sizeof(mapvertex_t);
	vertexes = (vertex_t *) Z_Malloc (numvertexes*sizeof(vertex_t), PU_LEVEL, NULL);
	data = W_CacheLumpNum (lump, PU_STATIC);

	ml = (mapvertex_t *)data;
	li = vertexes;
	for (i = 0; i < numvertexes; i++, li++, ml++)
	{
		li->x = SHORT(ml->x)<<FRACBITS;
		li->y = SHORT(ml->y)<<FRACBITS;
	}

	Z_Free (data);
}


/*
=================
=
= P_LoadSegs
=
=================
*/

static void P_LoadSegs (int lump)
{
	void		*data;
	int		i;
	mapseg_t	*ml;
	seg_t		*li;
	line_t		*ldef;
	int		_linedef, side;

	numsegs = W_LumpLength (lump) / sizeof(mapseg_t);
	segs = (seg_t *) Z_Malloc (numsegs*sizeof(seg_t), PU_LEVEL, NULL);
	memset (segs, 0, numsegs*sizeof(seg_t));
	data = W_CacheLumpNum (lump, PU_STATIC);

	ml = (mapseg_t *)data;
	li = segs;
	for (i = 0; i < numsegs; i++, li++, ml++)
	{
		li->v1 = &vertexes[SHORT(ml->v1)];
		li->v2 = &vertexes[SHORT(ml->v2)];

		li->angle = (SHORT(ml->angle))<<16;
		li->offset = (SHORT(ml->offset))<<16;
		_linedef = SHORT(ml->linedef);
		ldef = &lines[_linedef];
		li->linedef = ldef;
		side = SHORT(ml->side);
		li->sidedef = &sides[ldef->sidenum[side]];
		li->frontsector = sides[ldef->sidenum[side]].sector;
		if (ldef-> flags & ML_TWOSIDED)
			li->backsector = sides[ldef->sidenum[side^1]].sector;
		else
			li->backsector = 0;

#ifdef RENDER3D
	// Calculate the length of the segment. We need this for
	// the texture coordinates. -jk
		li->len = P_AccurateDistance(li->v2->x - li->v1->x, li->v2->y - li->v1->y);
#endif
	}

	Z_Free (data);
}


/*
=================
=
= P_LoadSubsectors
=
=================
*/

static void P_LoadSubsectors (int lump)
{
	void			*data;
	int			i;
	mapsubsector_t		*ms;
	subsector_t		*ss;

	numsubsectors = W_LumpLength (lump) / sizeof(mapsubsector_t);
	subsectors = (subsector_t *) Z_Malloc (numsubsectors*sizeof(subsector_t), PU_LEVEL, NULL);
	data = W_CacheLumpNum (lump, PU_STATIC);

	ms = (mapsubsector_t *)data;
	memset (subsectors, 0, numsubsectors*sizeof(subsector_t));
	ss = subsectors;
	for (i = 0; i < numsubsectors; i++, ss++, ms++)
	{
		ss->numlines = SHORT(ms->numsegs);
		ss->firstline = SHORT(ms->firstseg);
	}

	Z_Free (data);
}


/*
=================
=
= P_LoadSectors
=
=================
*/

static void P_LoadSectors (int lump)
{
	void			*data;
	int			i;
	mapsector_t		*ms;
	sector_t		*ss;

	numsectors = W_LumpLength (lump) / sizeof(mapsector_t);
	sectors = (sector_t *) Z_Malloc (numsectors*sizeof(sector_t), PU_LEVEL, NULL);
	memset (sectors, 0, numsectors*sizeof(sector_t));
	data = W_CacheLumpNum (lump, PU_STATIC);

	ms = (mapsector_t *)data;
	ss = sectors;

	for (i = 0; i < numsectors; i++, ss++, ms++)
	{
		ss->floorheight = SHORT(ms->floorheight)<<FRACBITS;
		ss->ceilingheight = SHORT(ms->ceilingheight)<<FRACBITS;
		ss->floorpic = R_FlatNumForName(ms->floorpic);
		ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);
		ss->lightlevel = SHORT(ms->lightlevel);
		ss->special = SHORT(ms->special);
		ss->tag = SHORT(ms->tag);
		ss->thinglist = NULL;

#ifdef RENDER3D
		ss->flatoffx = ss->flatoffy = 0;// Flat scrolling.
		ss->skyfix = 0;			// Set if needed.
#endif
	}
	Z_Free(data);
}


/*
=================
=
= P_LoadNodes
=
=================
*/

static void P_LoadNodes (int lump)
{
	void		*data;
	int		i, j, k;
	mapnode_t	*mn;
	node_t		*no;

	numnodes = W_LumpLength (lump) / sizeof(mapnode_t);
	nodes = (node_t *) Z_Malloc (numnodes*sizeof(node_t), PU_LEVEL, NULL);
	data = W_CacheLumpNum (lump, PU_STATIC);

	mn = (mapnode_t *)data;
	no = nodes;
	for (i = 0; i < numnodes; i++, no++, mn++)
	{
		no->x = SHORT(mn->x)<<FRACBITS;
		no->y = SHORT(mn->y)<<FRACBITS;
		no->dx = SHORT(mn->dx)<<FRACBITS;
		no->dy = SHORT(mn->dy)<<FRACBITS;
		for (j = 0; j < 2; j++)
		{
			no->children[j] = SHORT(mn->children[j]);
			for (k = 0; k < 4; k++)
				no->bbox[j][k] = SHORT(mn->bbox[j][k])<<FRACBITS;
		}
	}
	Z_Free (data);
}


/*
=================
=
= P_LoadThings
=
=================
*/

static void P_LoadThings(int lump)
{
	void		*data;
	int		i;
	mapthing_t	*mt;
	int		numthings;

	data = W_CacheLumpNum(lump, PU_STATIC);
	numthings = W_LumpLength(lump) / sizeof(mapthing_t);

	mt = (mapthing_t *)data;
	for (i = 0; i < numthings; i++, mt++)
	{
		mt->x = SHORT(mt->x);
		mt->y = SHORT(mt->y);
		mt->angle = SHORT(mt->angle);
		mt->type = SHORT(mt->type);
		mt->options = SHORT(mt->options);
		P_SpawnMapThing(mt);
	}
	Z_Free(data);
}

/*
=================
=
= P_LoadLineDefs
=
= Also counts secret lines for intermissions
=================
*/

static void P_LoadLineDefs(int lump)
{
	void		*data;
	int		i;
	maplinedef_t	*mld;
	line_t		*ld;
	vertex_t	*v1, *v2;

	numlines = W_LumpLength(lump) / sizeof(maplinedef_t);
	lines = (line_t *) Z_Malloc(numlines*sizeof(line_t), PU_LEVEL, NULL);
	memset(lines, 0, numlines*sizeof(line_t));
	data = W_CacheLumpNum(lump, PU_STATIC);

	mld = (maplinedef_t *)data;
	ld = lines;
	for (i = 0; i < numlines; i++, mld++, ld++)
	{
		ld->flags = SHORT(mld->flags);

		ld->special = SHORT(mld->special);
		ld->tag = SHORT(mld->tag);

		v1 = ld->v1 = &vertexes[SHORT(mld->v1)];
		v2 = ld->v2 = &vertexes[SHORT(mld->v2)];
		ld->dx = v2->x - v1->x;
		ld->dy = v2->y - v1->y;
		if (!ld->dx)
			ld->slopetype = ST_VERTICAL;
		else if (!ld->dy)
			ld->slopetype = ST_HORIZONTAL;
		else
		{
			if (FixedDiv (ld->dy , ld->dx) > 0)
				ld->slopetype = ST_POSITIVE;
			else
				ld->slopetype = ST_NEGATIVE;
		}

		if (v1->x < v2->x)
		{
			ld->bbox[BOXLEFT] = v1->x;
			ld->bbox[BOXRIGHT] = v2->x;
		}
		else
		{
			ld->bbox[BOXLEFT] = v2->x;
			ld->bbox[BOXRIGHT] = v1->x;
		}
		if (v1->y < v2->y)
		{
			ld->bbox[BOXBOTTOM] = v1->y;
			ld->bbox[BOXTOP] = v2->y;
		}
		else
		{
			ld->bbox[BOXBOTTOM] = v2->y;
			ld->bbox[BOXTOP] = v1->y;
		}
		ld->sidenum[0] = SHORT(mld->sidenum[0]);
		ld->sidenum[1] = SHORT(mld->sidenum[1]);
		if (ld->sidenum[0] != -1)
			ld->frontsector = sides[ld->sidenum[0]].sector;
		else
			ld->frontsector = 0;
		if (ld->sidenum[1] != -1)
			ld->backsector = sides[ld->sidenum[1]].sector;
		else
			ld->backsector = 0;
	}

	Z_Free (data);
}


/*
=================
=
= P_LoadSideDefs
=
=================
*/

static void P_LoadSideDefs (int lump)
{
	void		*data;
	int		i;
	mapsidedef_t	*msd;
	side_t		*sd;

	numsides = W_LumpLength (lump) / sizeof(mapsidedef_t);
	sides = (side_t *) Z_Malloc (numsides*sizeof(side_t), PU_LEVEL, NULL);
	memset (sides, 0, numsides*sizeof(side_t));
	data = W_CacheLumpNum (lump, PU_STATIC);

	msd = (mapsidedef_t *)data;
	sd = sides;
	for (i = 0; i < numsides; i++, msd++, sd++)
	{
		sd->textureoffset = SHORT(msd->textureoffset)<<FRACBITS;
		sd->rowoffset = SHORT(msd->rowoffset)<<FRACBITS;
		sd->toptexture = R_TextureNumForName(msd->toptexture);
		sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
		sd->midtexture = R_TextureNumForName(msd->midtexture);
		sd->sector = &sectors[SHORT(msd->sector)];
	}
	Z_Free(data);
}

/*
=================
=
= P_LoadBlockMap
=
=================
*/

static void P_LoadBlockMap (int lump)
{
	int		i, count;

	blockmaplump = (short *) W_CacheLumpNum (lump, PU_LEVEL);
	blockmap = blockmaplump + 4;
	count = W_LumpLength (lump) / 2;
	for (i = 0; i < count; i++)
		blockmaplump[i] = SHORT(blockmaplump[i]);

	bmaporgx = blockmaplump[0]<<FRACBITS;
	bmaporgy = blockmaplump[1]<<FRACBITS;
	bmapwidth = blockmaplump[2];
	bmapheight = blockmaplump[3];

// clear out mobj chains
	count = sizeof(*blocklinks) * bmapwidth * bmapheight;
	blocklinks = (mobj_t **) Z_Malloc (count, PU_LEVEL, NULL);
	memset (blocklinks, 0, count);
}


/*
=================
=
= P_GroupLines
=
= Builds sector line lists and subsector sector numbers
= Finds block bounding boxes for sectors
=================
*/

static void P_GroupLines (void)
{
	line_t		**linebuffer;
	int		i, j, total;
	line_t		*li;
	sector_t	*sector;
	subsector_t	*ss;
	seg_t		*seg;
	fixed_t		bbox[4];
	int		block;

// look up sector number for each subsector
	ss = subsectors;
	for (i = 0; i < numsubsectors; i++, ss++)
	{
		seg = &segs[ss->firstline];
		ss->sector = seg->sidedef->sector;
	}

// count number of lines in each sector
	li = lines;
	total = 0;
	for (i = 0; i < numlines; i++, li++)
	{
		total++;
		li->frontsector->linecount++;
		if (li->backsector && li->backsector != li->frontsector)
		{
			li->backsector->linecount++;
			total++;
		}
	}

// build line tables for each sector
	linebuffer = (line_t **) Z_Malloc (total * sizeof(line_t *), PU_LEVEL, NULL);
	sector = sectors;
	for (i = 0; i < numsectors; i++, sector++)
	{
		M_ClearBox (bbox);
		sector->lines = linebuffer;
		li = lines;
		for (j = 0; j < numlines; j++, li++)
		{
			if (li->frontsector == sector || li->backsector == sector)
			{
				*linebuffer++ = li;
				M_AddToBox (bbox, li->v1->x, li->v1->y);
				M_AddToBox (bbox, li->v2->x, li->v2->y);
			}
		}
		if (linebuffer - sector->lines != sector->linecount)
			I_Error ("P_GroupLines: miscounted");

		// set the degenmobj_t to the middle of the bounding box
		sector->soundorg.x = (bbox[BOXRIGHT] + bbox[BOXLEFT]) / 2;
		sector->soundorg.y = (bbox[BOXTOP] + bbox[BOXBOTTOM]) / 2;

		// adjust bounding box to map blocks
		block = (bbox[BOXTOP] - bmaporgy + MAXRADIUS) >> MAPBLOCKSHIFT;
		block = block >= bmapheight ? bmapheight - 1 : block;
		sector->blockbox[BOXTOP] = block;

		block = (bbox[BOXBOTTOM] - bmaporgy - MAXRADIUS) >> MAPBLOCKSHIFT;
		block = block < 0 ? 0 : block;
		sector->blockbox[BOXBOTTOM] = block;

		block = (bbox[BOXRIGHT] - bmaporgx + MAXRADIUS) >> MAPBLOCKSHIFT;
		block = block >= bmapwidth ? bmapwidth - 1 : block;
		sector->blockbox[BOXRIGHT] = block;

		block = (bbox[BOXLEFT] - bmaporgx - MAXRADIUS) >> MAPBLOCKSHIFT;
		block = block < 0 ? 0 : block;
		sector->blockbox[BOXLEFT] = block;
	}
}


#if defined(RENDER3D)

#define MAX_CC_SIDES	64

static float P_AccurateDistance(fixed_t dx, fixed_t dy)
{
	float fx = FIX2FLT(dx), fy = FIX2FLT(dy);
	return (float)sqrt(fx*fx + fy*fy);
}

static int __no_optimize detSideFloat(fvertex_t *pnt, fdivline_t *dline)
{
	/*
	    (AY-CY)(BX-AX)-(AX-CX)(BY-AY)
	s = -----------------------------
			L**2

	If s < 0  C is left of AB (you can just check the numerator)
	If s > 0  C is right of AB
	If s = 0  C is on AB

	We'll return false if the point c is on the left side.
	*/
	float s = (dline->y - pnt->y) * dline->dx - (dline->x - pnt->x) * dline->dy;
	if (s < 0)
		return 0;
	return 1;
}

// Lines start-end and fdiv must intersect.
static float __no_optimize findIntersectionVertex(fvertex_t *start, fvertex_t *end,
						  fdivline_t *fdiv, fvertex_t *inter)
{
	float ax = start->x, ay = start->y, bx = end->x, by = end->y;
	float cx = fdiv->x, cy = fdiv->y, dx = cx + fdiv->dx, dy = cy + fdiv->dy;
	/*
	    (YA-YC)(XD-XC)-(XA-XC)(YD-YC)
	r = -----------------------------  (eqn 1)
	    (XB-XA)(YD-YC)-(YB-YA)(XD-XC)
	*/
	float r = ((ay - cy) * (dx-cx) - (ax-cx) * (dy-cy)) /
		((bx - ax) * (dy - cy) - (by - ay) * (dx-cx));
	/*
	XI=XA+r(XB-XA)
	YI=YA+r(YB-YA)
	*/
	inter->x = ax + r * (bx - ax);
	inter->y = ay + r * (by - ay);
	return r;
}

static void P_ConvexCarver(subsector_t *ssec, int num, divline_t *list)
{
	int		numclippers = num + ssec->numlines;
	fdivline_t	*clippers = (fdivline_t *) malloc(numclippers*sizeof(fdivline_t));
	int		i, k, numedgepoints;
	fvertex_t	*edgepoints;
	unsigned char	sidelist[MAX_CC_SIDES];

// Convert the divlines to float, in reverse order.
	for (i = 0; i < numclippers; i++)
	{
		if (i < num)
		{
			clippers[i].x = FIX2FLT(list[num - i - 1].x);
			clippers[i].y = FIX2FLT(list[num - i - 1].y);
			clippers[i].dx = FIX2FLT(list[num - i - 1].dx);
			clippers[i].dy = FIX2FLT(list[num - i - 1].dy);
		}
		else
		{
			seg_t *seg = segs + (ssec->firstline + i - num);
			clippers[i].x = FIX2FLT(seg->v1->x);
			clippers[i].y = FIX2FLT(seg->v1->y);
			clippers[i].dx = FIX2FLT(seg->v2->x - seg->v1->x);
			clippers[i].dy = FIX2FLT(seg->v2->y - seg->v1->y);
		}
	}

// Setup the 'worldwide' polygon.
	numedgepoints = 4;
	edgepoints = (fvertex_t *) malloc(numedgepoints*sizeof(fvertex_t));

	edgepoints[0].x = -32768;
	edgepoints[0].y = 32768;

	edgepoints[1].x = 32768;
	edgepoints[1].y = 32768;

	edgepoints[2].x = 32768;
	edgepoints[2].y = -32768;

	edgepoints[3].x = -32768;
	edgepoints[3].y = -32768;

// We'll now clip the polygon with each of the divlines. The left side of
// each divline is discarded.
	for (i = 0; i < numclippers; i++)
	{
		fdivline_t *curclip = clippers + i;

		// First we'll determine the side of each vertex.
		// Points are allowed to be on the line.
		for (k = 0; k < numedgepoints; k++)
		{
			sidelist[k] = detSideFloat(edgepoints + k, curclip);
		}

		for (k = 0; k < numedgepoints; k++)
		{
			int startIdx = k, endIdx = k + 1;

			// Check the end index.
			if (endIdx == numedgepoints)
				endIdx = 0;	// Wrap-around.

			// Clipping will happen when the ends are on different sides.
			if (sidelist[startIdx] != sidelist[endIdx])
			{
				fvertex_t newvert;
			// Find the intersection point of intersecting lines.
				findIntersectionVertex(edgepoints + startIdx, edgepoints + endIdx, curclip, &newvert);

			// Add the new vertex. Also modify the sidelist.
				edgepoints = (fvertex_t *) realloc(edgepoints, (++numedgepoints)*sizeof(fvertex_t));
				if (numedgepoints >= MAX_CC_SIDES)
					I_Error("Too many points in carver.\n");

			// Make room for the new vertex.
				memmove(edgepoints + endIdx + 1, edgepoints + endIdx,
					(numedgepoints - endIdx - 1)*sizeof(fvertex_t));
				memcpy (edgepoints + endIdx, &newvert, sizeof(newvert));

				memmove(sidelist + endIdx + 1, sidelist + endIdx, numedgepoints - endIdx - 1);
				sidelist[endIdx] = 1;

			// Skip over the new vertex.
				k++;
			}
		}

	// Now we must discard the points that are on the wrong side.
		for (k = 0; k < numedgepoints; k++)
		{
			if (!sidelist[k])
			{
				memmove(edgepoints + k, edgepoints + k + 1, (numedgepoints-k-1)*sizeof(fvertex_t));
				memmove(sidelist + k, sidelist + k + 1, numedgepoints - k - 1);
				numedgepoints--;
				k--;
			}
		} 
	}

	if (!numedgepoints)
	{
	//	I_Error("All carved away!\n");
		printf( "All carved away: subsector %p\n", ssec);
		ssec->numedgeverts = 0;
		ssec->edgeverts = 0;
		ssec->origedgeverts = 0;
	}
	else
	{
		// Screen out consecutive identical points.
		for (i = 0; i < numedgepoints; i++)
		{
			int previdx = i - 1;
			if (previdx < 0)
				previdx = numedgepoints - 1;
			if (edgepoints[i].x == edgepoints[previdx].x &&
			    edgepoints[i].y == edgepoints[previdx].y)
			{
			// This point (i) must be removed.
				memmove(edgepoints + i, edgepoints + i + 1,
					sizeof(fvertex_t)*(numedgepoints - i - 1));
				numedgepoints--;
				i--;
			}
		}
		// We need these with dynamic lights.
		ssec->origedgeverts = (fvertex_t *) Z_Malloc(sizeof(fvertex_t)*numedgepoints, PU_LEVEL, NULL);
		memcpy(ssec->origedgeverts, edgepoints, sizeof(fvertex_t)*numedgepoints);

		// Find the center point. Do this by first finding the bounding box.
		ssec->bbox[0].x = ssec->bbox[1].x = edgepoints[0].x;
		ssec->bbox[0].y = ssec->bbox[1].y = edgepoints[0].y;
		for (i = 1; i < numedgepoints; i++)
		{
			if (edgepoints[i].x < ssec->bbox[0].x)
				ssec->bbox[0].x = edgepoints[i].x;
			if (edgepoints[i].y < ssec->bbox[0].y)
				ssec->bbox[0].y = edgepoints[i].y;
			if (edgepoints[i].x > ssec->bbox[1].x)
				ssec->bbox[1].x = edgepoints[i].x;
			if (edgepoints[i].y > ssec->bbox[1].y)
				ssec->bbox[1].y = edgepoints[i].y;
		}
		ssec->midpoint.x = (ssec->bbox[1].x + ssec->bbox[0].x) / 2;
		ssec->midpoint.y = (ssec->bbox[1].y + ssec->bbox[0].y) / 2;

		// Make slight adjustments to patch up those ugly, small gaps.
		for (i = 0; i < numedgepoints; i++)
		{
			float dx = edgepoints[i].x - ssec->midpoint.x,
			      dy = edgepoints[i].y - ssec->midpoint.y;
			float dlen = (float) sqrt(dx*dx + dy*dy) * 3;
			if (dlen)
			{
				edgepoints[i].x += dx / dlen;
				edgepoints[i].y += dy / dlen;
			}
		}

		ssec->numedgeverts = numedgepoints;
		ssec->edgeverts = (fvertex_t *) Z_Malloc(sizeof(fvertex_t)*numedgepoints, PU_LEVEL, NULL);
		memcpy(ssec->edgeverts, edgepoints, sizeof(fvertex_t)*numedgepoints);
	}

	// We're done, free the edgepoints memory.
	free(clippers);
	free(edgepoints);
}

static void P_CreateFloorsAndCeilings(int bspnode, int numdivlines, divline_t* divlines)
{
	node_t		*nod;
	divline_t	*childlist, *dl;
	int		childlistsize = numdivlines + 1;

	// If this is a subsector we are dealing with, begin carving with the
	// given list.
	if (bspnode & NF_SUBSECTOR)
	{
	// We have arrived at a subsector. The divline list contains all
	// the partition lines that carve out the subsector.
		int ssidx = bspnode & (~NF_SUBSECTOR);
		OGL_DEBUG("subsector %d: %d divlines\n", ssidx, numdivlines);
	//	if (ssidx < 10)
		P_ConvexCarver(subsectors+ssidx, numdivlines, divlines);

		OGL_DEBUG("subsector %d: %d edgeverts\n", ssidx, subsectors[ssidx].numedgeverts);
		return;	// This leaf is done.
	}

	// Get a pointer to the node.
	nod = nodes + bspnode;

	// Allocate a new list for each child.
	childlist = (divline_t *) malloc(childlistsize*sizeof(divline_t));

	// Copy the previous lines.
	if (divlines)
		memcpy(childlist, divlines, numdivlines*sizeof(divline_t));

	dl = childlist + numdivlines;
	dl->x = nod->x;
	dl->y = nod->y;
	// The right child gets the original line (LEFT side clipped).
	dl->dx = nod->dx;
	dl->dy = nod->dy;
	P_CreateFloorsAndCeilings(nod->children[0], childlistsize, childlist);

	// The left side. We must reverse the line, otherwise the wrong
	// side would get clipped.
	dl->dx = -nod->dx;
	dl->dy = -nod->dy;
	P_CreateFloorsAndCeilings(nod->children[1], childlistsize, childlist);

	// We are finishing with this node, free the allocated list.
	free(childlist);
}

static void P_SkyFix(void)
{
	int		i;

	// We need to check all the linedefs.
	for (i = 0; i < numlines; i++)
	{
		line_t *line = lines + i;
		sector_t *front = line->frontsector, *back = line->backsector;
		int fix = 0;
		// The conditions!
		if (!front || !back)
			continue;
		// Both the front and back sectors must have the sky ceiling.
		if (front->ceilingpic != skyflatnum || back->ceilingpic != skyflatnum)
			continue;
		// Operate on the lower sector.
		OGL_DEBUG("Line %d (f:%d, b:%d).\n", i, front->ceilingheight >> FRACBITS,
							 back->ceilingheight >> FRACBITS);
		if (front->ceilingheight < back->ceilingheight)
		{
			fix = (back->ceilingheight - front->ceilingheight) >> FRACBITS;
			if (fix > front->skyfix)
				front->skyfix = fix;
		}
		else if (front->ceilingheight > back->ceilingheight)
		{
			fix = (front->ceilingheight - back->ceilingheight) >> FRACBITS;
			if (fix > back->skyfix)
				back->skyfix = fix;
		}
	}
}
#endif	/* RENDER3D */

//=============================================================================

/*
=================
=
= P_SetupLevel
=
=================
*/

void P_SetupLevel(int episode, int map, int playermask, skill_t skill)
{
	int		i;
	int		parm;
	char		lumpname[9];
	int		lumpnum;
	mobj_t		*mobj;

	totalkills = totalitems = totalsecret = 0;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		players[i].killcount = players[i].secretcount = players[i].itemcount = 0;
	}
	players[consoleplayer].viewz = 1;	// will be set by player think

	S_Start ();	// make sure all sounds are stopped before Z_FreeTags

	Z_FreeTags(PU_LEVEL, PU_PURGELEVEL - 1);

#ifdef RENDER3D
	OGL_ResetData();
#endif

	P_InitThinkers();

//
// look for a regular (development) map first
//
	lumpname[0] = 'E';
	lumpname[1] = '0' + episode;
	lumpname[2] = 'M';
	lumpname[3] = '0' + map;
	lumpname[4] = 0;
	leveltime = 0;

	lumpnum = W_GetNumForName (lumpname);

	// Note: most of this ordering is important
	P_LoadBlockMap(lumpnum + ML_BLOCKMAP);
	P_LoadVertexes(lumpnum + ML_VERTEXES);
	P_LoadSectors(lumpnum + ML_SECTORS);
	P_LoadSideDefs(lumpnum + ML_SIDEDEFS);
	P_LoadLineDefs(lumpnum + ML_LINEDEFS);
	P_LoadSubsectors(lumpnum + ML_SSECTORS);
	P_LoadNodes(lumpnum + ML_NODES);
	P_LoadSegs(lumpnum + ML_SEGS);

#ifdef RENDER3D
	// We need to carve out the floor/ceiling polygons of each subsector.
	// Walk the tree to do this.
	//OGL_DEBUG("Floor/ceiling creation: begin at %d, ", ticcount);
	P_CreateFloorsAndCeilings(numnodes - 1, 0, 0);
	// Also check if the sky needs a fix.
	P_SkyFix();
#endif

	rejectmatrix = (byte *) W_CacheLumpNum(lumpnum + ML_REJECT, PU_LEVEL);
	P_GroupLines();
	bodyqueslot = 0;
	deathmatch_p = deathmatchstarts;
	P_InitAmbientSound();
	P_InitMonsters();
	P_OpenWeapons();
	P_LoadThings(lumpnum + ML_THINGS);
	P_CloseWeapons();

	// If deathmatch, randomly spawn the active players
	TimerGame = 0;
	if (deathmatch)
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
			{   // must give a player spot before deathmatchspawn
				mobj = P_SpawnMobj (playerstarts[i].x<<16,
						    playerstarts[i].y<<16,
						    0, MT_PLAYER);
				players[i].mo = mobj;
				G_DeathMatchSpawnPlayer (i);
				P_RemoveMobj (mobj);
			}
		}
		parm = M_CheckParm("-timer");
		if (parm && parm < myargc - 1)
		{
			TimerGame = atoi(myargv[parm + 1]) * 35 * 60;
		}
	}

// set up world state
	P_SpawnSpecials ();

// build subsector connect matrix
//	P_ConnectSubsectors ();

// preload graphics
	if (precache)
		R_PrecacheLevel ();

//	printf ("free memory: 0x%x\n", Z_FreeMemory());
}


/*
=================
=
= P_Init
=
=================
*/

void P_Init(void)
{
	P_InitSwitchList();
	P_InitPicAnims();
	P_InitTerrainTypes();
	P_InitLava();
	R_InitSprites(sprnames);
}

