
// W_wad.c
// $Revision$
// $Date$

// HEADER FILES ------------------------------------------------------------

#include "h2stdinc.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "doomdef.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

typedef struct
{
	char identification[4];		// should be IWAD
	int numlumps;
	int infotableofs;
} wadinfo_t;

typedef struct
{
	int filepos;
	int size;
	char name[8];
} filelump_t;

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

lumpinfo_t *lumpinfo;		// location of each lump on disk
int numlumps;
void **lumpcache;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

boolean W_IsWadPresent(const char *filename)
{
	char path[MAX_OSPATH], *waddir;
	int handle = -1;

	/* try the directory specified by the
	 * shared data environment variable first.
	 */
	waddir = getenv(DATA_ENVVAR);
	if (waddir && *waddir)
	{
		snprintf (path, sizeof(path), "%s/%s", waddir, filename);
		handle = open(path, O_RDONLY|O_BINARY);
	}
	if (handle == -1)	/* Try UserDIR */
	{
		snprintf (path, sizeof(path), "%s%s", basePath, filename);
		handle = open(path, O_RDONLY|O_BINARY);
	}
	if (handle == -1)	/* Now try CWD */
	{
		handle = open(filename, O_RDONLY|O_BINARY);
	}
	if (handle == -1)
		return false;	/* Didn't find the file. */
	close(handle);
	return true;
}

//==========================================================================
//
// W_AddFile
//
// All files are optional, but at least one file must be found.
// Files with a .wad extension are wadlink files with multiple lumps,
// other files are single lumps with the base filename for the lump name.
//
//==========================================================================

void W_AddFile(const char *filename)
{
	wadinfo_t header;
	lumpinfo_t *lump_p;
	char path[MAX_OSPATH], *waddir;
	int handle, length;
	int startlump;
	filelump_t *fileinfo, singleinfo;
	filelump_t *freeFileInfo;
	int i;

	handle = -1;
	/* try the directory specified by the
	 * shared data environment variable first.
	 */
	waddir = getenv(DATA_ENVVAR);
	if (waddir && *waddir)
	{
		snprintf (path, sizeof(path), "%s/%s", waddir, filename);
		handle = open(path, O_RDONLY|O_BINARY);
	}
	if (handle == -1)	/* Try UserDIR */
	{
		snprintf (path, sizeof(path), "%s%s", basePath, filename);
		handle = open(path, O_RDONLY|O_BINARY);
	}
	if (handle == -1)	/* Now try CWD */
	{
		handle = open(filename, O_RDONLY|O_BINARY);
	}
	if (handle == -1)
		return;		/* Didn't find the file. */

	startlump = numlumps;
	if (strcasecmp(filename + strlen(filename) - 3, "wad") != 0)
	{ // Single lump file
		fileinfo = &singleinfo;
		freeFileInfo = NULL;
		singleinfo.filepos = 0;
		singleinfo.size = LONG(filelength(handle));
		M_ExtractFileBase(filename, singleinfo.name);
		numlumps++;
	}
	else
	{ // WAD file
		read(handle, &header, sizeof(header));
		if (strncmp(header.identification, "IWAD", 4) != 0)
		{
			if (strncmp(header.identification, "PWAD", 4) != 0)
			{ // Bad file id
				I_Error("Wad file %s doesn't have IWAD or PWAD id\n", filename);
			}
		}
		header.numlumps = LONG(header.numlumps);
		header.infotableofs = LONG(header.infotableofs);
		length = header.numlumps * sizeof(filelump_t);
		fileinfo = (filelump_t *) malloc(length);
		if (!fileinfo)
		{
			I_Error("W_AddFile: fileinfo malloc failed\n");
		}
		freeFileInfo = fileinfo;
		lseek(handle, header.infotableofs, SEEK_SET);
		read(handle, fileinfo, length);
		numlumps += header.numlumps;
	}

	// Fill in lumpinfo
	lumpinfo = (lumpinfo_t *) realloc(lumpinfo, numlumps * sizeof(lumpinfo_t));
	if (!lumpinfo)
	{
		I_Error("Couldn't realloc lumpinfo");
	}
	lump_p = &lumpinfo[startlump];
	for (i = startlump; i < numlumps; i++, lump_p++, fileinfo++)
	{
		memset(lump_p->name, 0, 8);
		lump_p->handle = handle;
		lump_p->position = LONG(fileinfo->filepos);
		lump_p->size = LONG(fileinfo->size);
		strncpy(lump_p->name, fileinfo->name, 8);
	}
	if (freeFileInfo)
	{
		free(freeFileInfo);
	}
}

//==========================================================================
//
// W_InitMultipleFiles
//
// Pass a null terminated list of files to use.  All files are optional,
// but at least one file must be found.  Files with a .wad extension are
// idlink files with multiple lumps.  Other files are single lumps with
// the base filename for the lump name.  Lump names can appear multiple
// times.  The name searcher looks backwards, so a later file can
// override an earlier one.
//
//==========================================================================

void W_InitMultipleFiles(const char **filenames)
{
	int size;

	// Open all the files, load headers, and count lumps
	numlumps = 0;
	lumpinfo = (lumpinfo_t *) malloc(1); // Will be realloced as lumps are added

	for ( ; *filenames; filenames++)
	{
		W_AddFile(*filenames);
	}
	if (!numlumps)
	{
		I_Error("W_InitMultipleFiles: no files found");
	}

	// Set up caching
	size = numlumps * sizeof(*lumpcache);
	lumpcache = (void **) malloc(size);
	if (!lumpcache)
	{
		I_Error("Couldn't allocate lumpcache");
	}
	memset(lumpcache, 0, size);
}

//==========================================================================
//
// W_InitFile
//
// Just initialize from a single file
//
//==========================================================================

void W_InitFile(const char *filename)
{
	const char *names[2];

	names[0] = filename;
	names[1] = NULL;
	W_InitMultipleFiles(names);
}

//==========================================================================
//
// W_NumLumps
//
//==========================================================================

int	W_NumLumps(void)
{
	return numlumps;
}

//==========================================================================
//
// W_CheckNumForName
//
// Returns -1 if name not found.
//
//==========================================================================

int W_CheckNumForName(const char *name)
{
	char name8[9];
	lumpinfo_t *lump_p;

	// Make the name into two integers for easy compares
	memset(name8, 0, sizeof(name8));
	strncpy(name8, name, 8);
	strupr(name8); // case insensitive

	// Scan backwards so patch lump files take precedence
	lump_p = lumpinfo + numlumps;
	while (lump_p-- != lumpinfo)
	{
		if (memcmp(lump_p->name, name8, 8) == 0)
		{
			return (int)(lump_p - lumpinfo);
		}
	}
	return -1;
}

//==========================================================================
//
// W_GetNumForName
//
// Calls W_CheckNumForName, but bombs out if not found.
//
//==========================================================================

int	W_GetNumForName (const char *name)
{
	int	i;

	i = W_CheckNumForName(name);
	if (i != -1)
	{
		return i;
	}
	I_Error("W_GetNumForName: %s not found!", name);
	return -1;
}

//==========================================================================
//
// W_LumpLength
//
// Returns the buffer size needed to load the given lump.
//
//==========================================================================

int W_LumpLength(int lump)
{
	if (lump >= numlumps)
	{
		I_Error("W_LumpLength: %i >= numlumps", lump);
	}
	return lumpinfo[lump].size;
}

//==========================================================================
//
// W_ReadLump
//
// Loads the lump into the given buffer, which must be >= W_LumpLength().
//
//==========================================================================

void W_ReadLump(int lump, void *dest)
{
	int c;
	lumpinfo_t *l;

	if (lump >= numlumps)
	{
		I_Error("W_ReadLump: %i >= numlumps", lump);
	}
	l = lumpinfo+lump;
	lseek(l->handle, l->position, SEEK_SET);
	c = read(l->handle, dest, l->size);
	if (c < l->size)
	{
		I_Error("W_ReadLump: only read %i of %i on lump %i",
			c, l->size, lump);
	}
}

//==========================================================================
//
// W_CacheLumpNum
//
//==========================================================================

void *W_CacheLumpNum(int lump, int tag)
{
	byte *ptr;

	if ((unsigned)lump >= numlumps)
	{
		I_Error("W_CacheLumpNum: %i >= numlumps", lump);
	}
	if (!lumpcache[lump])
	{ // Need to read the lump in
		ptr = (byte *) Z_Malloc(W_LumpLength(lump), tag, &lumpcache[lump]);
		W_ReadLump(lump, lumpcache[lump]);
	}
	else
	{
		Z_ChangeTag(lumpcache[lump], tag);
	}
	return lumpcache[lump];
}

//==========================================================================
//
// W_CacheLumpName
//
//==========================================================================

void *W_CacheLumpName(const char *name, int tag)
{
	return W_CacheLumpNum(W_GetNumForName(name), tag);
}

//==========================================================================
//
// W_Profile
//
//==========================================================================

// Ripped out for Heretic
/*
static int	info[2500][10];
static int	profilecount;

void W_Profile (void)
{
	int		i;
	memblock_t	*block;
	void	*ptr;
	char	ch;
	FILE	*f;
	int		j;
	char	name[9];

	for (i = 0; i < numlumps; i++)
	{
		ptr = lumpcache[i];
		if (!ptr)
		{
			ch = ' ';
			continue;
		}
		else
		{
			block = (memblock_t *) ( (byte *)ptr - sizeof(memblock_t));
			if (block->tag < PU_PURGELEVEL)
				ch = 'S';
			else
				ch = 'P';
		}
		info[i][profilecount] = ch;
	}
	profilecount++;

	f = fopen ("waddump.txt","w");
	name[8] = 0;
	for (i = 0; i < numlumps; i++)
	{
		memcpy (name, lumpinfo[i].name, 8);
		for (j = 0; j < 8; j++)
			if (!name[j])
				break;
		for ( ; j < 8; j++)
			name[j] = ' ';
		fprintf (f,"%s ",name);
		for (j = 0; j < profilecount; j++)
			fprintf (f,"    %c",info[i][j]);
		fprintf (f,"\n");
	}
	fclose (f);
}
*/

