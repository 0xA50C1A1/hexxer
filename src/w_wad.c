
//**************************************************************************
//**
//** w_wad.c : Heretic 2 : Raven Software, Corp.
//**
//** $RCSfile: w_wad.c,v $
//** $Revision: 1.6 $
//** $Date: 95/10/06 20:56:47 $
//** $Author: cjr $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include <SDL2/SDL_stdinc.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <h2def.h>
#include <i_system.h>

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

typedef struct {
	char identification[4];
	int numlumps;
	int infotableofs;
} wadinfo_t;

typedef struct {
	int filepos;
	int size;
	char name[8];
} filelump_t;

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

lumpinfo_t *lumpinfo;
int numlumps;
void **lumpcache;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
// filelength
//
//==========================================================================

int filelength(int handle)
{
	struct stat fileinfo;

	if (fstat(handle, &fileinfo) == -1) {
		I_Error("Error fstating");
	}
	return fileinfo.st_size;
}

//==========================================================================
//
// W_AddFile
//
// Files with a .wad extension are wadlink files with multiple lumps,
// other files are single lumps with the base filename for the lump name.
//
//==========================================================================

static void W_AddFile(const char *filename)
{
	wadinfo_t header;
	lumpinfo_t *lump_p;
	unsigned i;
	int handle, length;
	int startlump;
	filelump_t *fileinfo, singleinfo;
	filelump_t *freeFileInfo;

	if ((handle = open(filename, O_RDONLY)) == -1) { // Didn't find file
		return;
	}
	startlump = numlumps;
	if (strcasecmp(filename + strlen(filename) - 3,
		       "wad")) { // Single lump file
		fileinfo = &singleinfo;
		freeFileInfo = NULL;
		singleinfo.filepos = 0;
		singleinfo.size = LONG(filelength(handle));
		M_ExtractFileBase(filename, singleinfo.name);
		numlumps++;
	} else { // WAD file
		read(handle, &header, sizeof(header));
		if (strncmp(header.identification, "IWAD", 4)) {
			if (strncmp(header.identification, "PWAD",
				    4)) { // Bad file id
				I_Error("Wad file %s doesn't have IWAD or PWAD id\n",
					filename);
			}
		}
		header.numlumps = LONG(header.numlumps);
		header.infotableofs = LONG(header.infotableofs);
		length = header.numlumps * sizeof(filelump_t);
		//		fileinfo = alloca(length);
		if (!(fileinfo = malloc(length))) {
			I_Error("W_AddFile:  fileinfo malloc failed\n");
		}
		freeFileInfo = fileinfo;
		lseek(handle, header.infotableofs, SEEK_SET);
		read(handle, fileinfo, length);
		numlumps += header.numlumps;
	}

	// Fill in lumpinfo
	lumpinfo = realloc(lumpinfo, numlumps * sizeof(lumpinfo_t));
	if (!lumpinfo) {
		I_Error("Couldn't realloc lumpinfo");
	}
	lump_p = &lumpinfo[startlump];
	for (i = startlump; i < numlumps; i++, lump_p++, fileinfo++) {
		lump_p->handle = handle;
		lump_p->position = LONG(fileinfo->filepos);
		lump_p->size = LONG(fileinfo->size);
		strncpy(lump_p->name, fileinfo->name, sizeof(lump_p->name));
	}
	if (freeFileInfo) {
		free(freeFileInfo);
	}
}

//==========================================================================
//
// W_InitMultipleFiles
//
// Pass a null terminated list of files to use.  All files are optional,
// but at least one file must be found.  Lump names can appear multiple
// times.  The name searcher looks backwards, so a later file can
// override an earlier one.
//
//==========================================================================

void W_InitMultipleFiles(const char **filenames)
{
	int size;

	// Open all the files, load headers, and count lumps
	numlumps = 0;
	lumpinfo = malloc(1); // Will be realloced as lumps are added

	for (; *filenames; filenames++) {
		W_AddFile(*filenames);
	}
	if (!numlumps) {
		I_Error("W_InitMultipleFiles: no files found");
	}

	// Set up caching
	size = numlumps * sizeof(*lumpcache);
	lumpcache = malloc(size);
	if (!lumpcache) {
		I_Error("Couldn't allocate lumpcache");
	}
	memset(lumpcache, 0, size);
}

//==========================================================================
//
// W_NumLumps
//
//==========================================================================

int W_NumLumps(void)
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
	int v1, v2;
	lumpinfo_t *lump_p;

	// Make the name into two integers for easy compares
	strncpy(name8, name, 8);
	name8[8] = 0; // in case the name was a full 8 chars
	SDL_strupr(name8); // case insensitive
	v1 = *(int *)name8;
	v2 = *(int *)&name8[4];

	// Scan backwards so patch lump files take precedence
	lump_p = lumpinfo + numlumps;
	while (lump_p-- != lumpinfo) {
		if (*(int *)lump_p->name == v1 &&
		    *(int *)&lump_p->name[4] == v2) {
			return lump_p - lumpinfo;
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

int W_GetNumForName(const char *name)
{
	int i;

	i = W_CheckNumForName(name);
	if (i != -1) {
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
	if (lump >= numlumps) {
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

	if (lump >= numlumps) {
		I_Error("W_ReadLump: %i >= numlumps", lump);
	}
	l = lumpinfo + lump;

	lseek(l->handle, l->position, SEEK_SET);
	c = read(l->handle, dest, l->size);
	if (c < l->size) {
		I_Error("W_ReadLump: only read %i of %i on lump %i", c, l->size,
			lump);
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

	if ((unsigned)lump >= numlumps) {
		I_Error("W_CacheLumpNum: %i >= numlumps", lump);
	}
	if (!lumpcache[lump]) { // Need to read the lump in
		ptr = Z_Malloc(W_LumpLength(lump), tag, &lumpcache[lump]);
		W_ReadLump(lump, lumpcache[lump]);
	} else {
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
