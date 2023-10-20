
//**************************************************************************
//**
//** m_misc.c : Heretic 2 : Raven Software, Corp.
//**
//** $RCSfile: m_misc.c,v $
//** $Revision: 1.18 $
//** $Date: 96/01/07 16:51:18 $
//** $Author: bgokey $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#ifdef __NeXT__
#include <libc.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <direct.h>
#include <fcntl.h>
#include <stdlib.h>
#include <io.h>
#include <conio.h>
#endif
#include <ctype.h>
#include "h2def.h"
#include "p_local.h"
#include <s_sound.h>

// MACROS ------------------------------------------------------------------

#define MALLOC_CLIB 1
#define MALLOC_ZONE 2

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static int ReadFile(char const *name, byte **buffer, int mallocType);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------
extern char *SavePath;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

int myargc;
char **myargv;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
// M_CheckParm
//
// Checks for the given parameter in the program's command line arguments.
// Returns the argument number (1 to argc-1) or 0 if not present.
//
//==========================================================================

int M_CheckParm(char *check)
{
	int i;

	for (i = 1; i < myargc; i++) {
		if (!strcasecmp(check, myargv[i])) {
			return i;
		}
	}
	return 0;
}

//==========================================================================
//
// M_ParmExists
//
// Returns true if the given parameter exists in the program's command
// line arguments, false if not.
//
//==========================================================================

boolean M_ParmExists(char *check)
{
	return M_CheckParm(check) != 0 ? true : false;
}

//==========================================================================
//
// M_ExtractFileBase
//
//==========================================================================

void M_ExtractFileBase(char *path, char *dest)
{
	char *src;
	int length;

	src = path + strlen(path) - 1;

	// Back up until a \ or the start
	while (src != path && *(src - 1) != '\\' && *(src - 1) != '/') {
		src--;
	}

	// Copy up to eight characters
	memset(dest, 0, 8);
	length = 0;
	while (*src && *src != '.') {
		if (++length == 9) {
			I_Error("Filename base of %s > 8 chars", path);
		}
		*dest++ = toupper((int)*src++);
	}
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

// This is the new flat distribution table
unsigned char rndtable[256] = {
	201, 1,	  243, 19,  18,	 42,  183, 203, 101, 123, 154, 137, 34,	 118,
	10,  216, 135, 246, 0,	 107, 133, 229, 35,  113, 177, 211, 110, 17,
	139, 84,  251, 235, 182, 166, 161, 230, 143, 91,  24,  81,  22,	 94,
	7,   51,  232, 104, 122, 248, 175, 138, 127, 171, 222, 213, 44,	 16,
	9,   33,  88,  102, 170, 150, 136, 114, 62,  3,	  142, 237, 6,	 252,
	249, 56,  74,  30,  13,	 21,  180, 199, 32,  132, 187, 234, 78,	 210,
	46,  131, 197, 8,   206, 244, 73,  4,	236, 178, 195, 70,  121, 97,
	167, 217, 103, 40,  247, 186, 105, 39,	95,  163, 99,  149, 253, 29,
	119, 83,  254, 26,  202, 65,  130, 155, 60,  64,  184, 106, 221, 93,
	164, 196, 112, 108, 179, 141, 54,  109, 11,  126, 75,  165, 191, 227,
	87,  225, 156, 15,  98,	 162, 116, 79,	169, 140, 190, 205, 168, 194,
	41,  250, 27,  20,  14,	 241, 50,  214, 72,  192, 220, 233, 67,	 148,
	96,  185, 176, 181, 215, 207, 172, 85,	89,  90,  209, 128, 124, 2,
	55,  173, 66,  152, 47,	 129, 59,  43,	159, 240, 239, 12,  189, 212,
	144, 28,  200, 77,  219, 198, 134, 228, 45,  92,  125, 151, 5,	 53,
	255, 52,  68,  245, 160, 158, 61,  86,	58,  82,  117, 37,  242, 145,
	69,  188, 115, 76,  63,	 100, 49,  111, 153, 80,  38,  57,  174, 224,
	71,  231, 23,  25,  48,	 218, 120, 147, 208, 36,  226, 223, 193, 238,
	157, 204, 146, 31
};

int rndindex = 0;
int prndindex = 0;

int M_Random(void)
{
	rndindex = (rndindex + 1) & 0xff;
	return rndtable[rndindex];
}

void M_ClearRandom(void)
{
	rndindex = prndindex = 0;
}

void M_ClearBox(fixed_t *box)
{
	box[BOXTOP] = box[BOXRIGHT] = MININT;
	box[BOXBOTTOM] = box[BOXLEFT] = MAXINT;
}

void M_AddToBox(fixed_t *box, fixed_t x, fixed_t y)
{
	if (x < box[BOXLEFT])
		box[BOXLEFT] = x;
	else if (x > box[BOXRIGHT])
		box[BOXRIGHT] = x;
	if (y < box[BOXBOTTOM])
		box[BOXBOTTOM] = y;
	else if (y > box[BOXTOP])
		box[BOXTOP] = y;
}

/*
==================
=
= M_WriteFile
=
==================
*/

#ifndef O_BINARY
#define O_BINARY 0
#endif

boolean M_WriteFile(char const *name, void *source, int length)
{
	int handle, count;

	handle = open(name, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
	if (handle == -1)
		return false;
	count = write(handle, source, length);
	close(handle);

	if (count < length)
		return false;

	return true;
}

//==========================================================================
//
// M_ReadFile
//
// Read a file into a buffer allocated using Z_Malloc().
//
//==========================================================================

int M_ReadFile(char const *name, byte **buffer)
{
	return ReadFile(name, buffer, MALLOC_ZONE);
}

//==========================================================================
//
// M_ReadFileCLib
//
// Read a file into a buffer allocated using malloc().
//
//==========================================================================

int M_ReadFileCLib(char const *name, byte **buffer)
{
	return ReadFile(name, buffer, MALLOC_CLIB);
}

//==========================================================================
//
// ReadFile
//
//==========================================================================

static int ReadFile(char const *name, byte **buffer, int mallocType)
{
	int handle, count, length;
	struct stat fileinfo;
	byte *buf;

	handle = open(name, O_RDONLY | O_BINARY, 0666);
	if (handle == -1) {
		I_Error("Couldn't read file %s", name);
	}
	if (fstat(handle, &fileinfo) == -1) {
		I_Error("Couldn't read file %s", name);
	}
	length = fileinfo.st_size;
	if (mallocType == MALLOC_ZONE) { // Use zone memory allocation
		buf = Z_Malloc(length, PU_STATIC, NULL);
	} else { // Use c library memory allocation
		buf = malloc(length);
		if (buf == NULL) {
			I_Error("Couldn't malloc buffer %d for file %s.",
				length, name);
		}
	}
	count = read(handle, buf, length);
	close(handle);
	if (count < length) {
		I_Error("Couldn't read file %s", name);
	}
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

	for (i = 1; i < myargc; i++) {
		if (myargv[i][0] == '@') {
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
			if (!handle) {
				printf("\nNo such response file!");
				exit(1);
			}
			ST_Message("Found response file %s!\n", &myargv[i][1]);
			fseek(handle, 0, SEEK_END);
			size = ftell(handle);
			fseek(handle, 0, SEEK_SET);
			file = malloc(size);
			fread(file, size, 1, handle);
			fclose(handle);

			// KEEP ALL CMDLINE ARGS FOLLOWING @RESPONSEFILE ARG
			for (index = 0, k = i + 1; k < myargc; k++)
				moreargs[index++] = myargv[k];

			firstargv = myargv[0];
			myargv = malloc(sizeof(char *) * MAXARGVS);
			memset(myargv, 0, sizeof(char *) * MAXARGVS);
			myargv[0] = firstargv;

			infile = file;
			indexinfile = k = 0;
			indexinfile++; // SKIP PAST ARGV[0] (KEEP IT)
			do {
				myargv[indexinfile++] = infile + k;
				while (k < size &&

				       ((*(infile + k) >= ' ' + 1) &&
					(*(infile + k) <= 'z')))
					k++;
				*(infile + k) = 0;
				while (k < size && ((*(infile + k) <= ' ') ||
						    (*(infile + k) > 'z')))
					k++;
			} while (k < size);

			for (k = 0; k < index; k++)
				myargv[indexinfile++] = moreargs[k];
			myargc = indexinfile;
			// DISPLAY ARGS
			if (M_CheckParm("-debug")) {
				ST_Message("%d command-line args:\n", myargc);
				for (k = 1; k < myargc; k++) {
					ST_Message("%s\n", myargv[k]);
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

	while ((c = *text) != 0) {
		if (c >= 'a' && c <= 'z') {
			*text++ = c - ('a' - 'A');
		} else {
			text++;
		}
	}
}

/*
==============================================================================

							DEFAULTS

==============================================================================
*/

int usemouse;
int usejoystick;

extern int key_right, key_left, key_up, key_down;
extern int key_strafeleft, key_straferight, key_jump;
extern int key_fire, key_use, key_strafe, key_speed;
extern int key_flyup, key_flydown, key_flycenter;
extern int key_lookup, key_lookdown, key_lookcenter;
extern int key_invleft, key_invright, key_useartifact;

extern int mousebfire;
extern int mousebstrafe;
extern int mousebforward;
extern int mousebjump;

extern int joybfire;
extern int joybstrafe;
extern int joybuse;
extern int joybspeed;
extern int joybjump;

extern boolean messageson;

extern int viewwidth, viewheight;

int mouseSensitivity;

extern int screenblocks;

extern char *chat_macros[10];

typedef struct {
	char *name;
	int *location;
	int defaultvalue;
	int scantranslate; // PC scan code hack
	int untranslated; // lousy hack
} default_t;

#ifndef __NeXT__
extern int snd_Channels;

#endif

default_t defaults[] = {
	{ "mouse_sensitivity", &mouseSensitivity, 5 },

	{ "sfx_volume", &snd_MaxVolume, 10 },
	{ "music_volume", &snd_MusicVolume, 10 },

	{ "key_right", &key_right, KEY_RIGHTARROW },
	{ "key_left", &key_left, KEY_LEFTARROW },
	{ "key_up", &key_up, KEY_UPARROW },
	{ "key_down", &key_down, KEY_DOWNARROW },
	{ "key_strafeleft", &key_strafeleft, ',' },
	{ "key_straferight", &key_straferight, '.' },
	{ "key_jump", &key_jump, '/' },
	{ "key_flyup", &key_flyup, 'u' },
	{ "key_flydown", &key_flydown, 'j' },
	{ "key_flycenter", &key_flycenter, 'k' },
	{ "key_lookup", &key_lookup, 'm' },
	{ "key_lookdown", &key_lookdown, 'b' },
	{ "key_lookcenter", &key_lookcenter, 'n' },
	{ "key_invleft", &key_invleft, '[' },
	{ "key_invright", &key_invright, ']' },
	{ "key_useartifact", &key_useartifact, 13 },

	{ "key_fire", &key_fire, ' ', 1 },
	{ "key_use", &key_use, 'x', 1 },
	{ "key_strafe", &key_strafe, 'c', 1 },
	{ "key_speed", &key_speed, 'z', 1 },

	{ "use_mouse", &usemouse, 1 },
	{ "mouseb_fire", &mousebfire, 0 },
	{ "mouseb_strafe", &mousebstrafe, 1 },
	{ "mouseb_forward", &mousebforward, 2 },
	{ "mouseb_jump", &mousebjump, -1 },

	{ "use_joystick", &usejoystick, 0 },
	{ "joyb_fire", &joybfire, 0 },
	{ "joyb_strafe", &joybstrafe, 1 },
	{ "joyb_use", &joybuse, 3 },
	{ "joyb_speed", &joybspeed, 2 },
	{ "joyb_jump", &joybjump, -1 },

	{ "screenblocks", &screenblocks, 10 },

#ifndef __NeXT__
	{ "snd_channels", &snd_Channels, 3 },
#endif

	{ "usegamma", &usegamma, 0 },

	//	{ "savedir", (int *)&SavePath, (int)DEFAULT_SAVEPATH },

	{ "messageson", (int *)&messageson, 1 },

	{ "chatmacro0", (int *)&chat_macros[0], (int)HUSTR_CHATMACRO0 },
	{ "chatmacro1", (int *)&chat_macros[1], (int)HUSTR_CHATMACRO1 },
	{ "chatmacro2", (int *)&chat_macros[2], (int)HUSTR_CHATMACRO2 },
	{ "chatmacro3", (int *)&chat_macros[3], (int)HUSTR_CHATMACRO3 },
	{ "chatmacro4", (int *)&chat_macros[4], (int)HUSTR_CHATMACRO4 },
	{ "chatmacro5", (int *)&chat_macros[5], (int)HUSTR_CHATMACRO5 },
	{ "chatmacro6", (int *)&chat_macros[6], (int)HUSTR_CHATMACRO6 },
	{ "chatmacro7", (int *)&chat_macros[7], (int)HUSTR_CHATMACRO7 },
	{ "chatmacro8", (int *)&chat_macros[8], (int)HUSTR_CHATMACRO8 },
	{ "chatmacro9", (int *)&chat_macros[9], (int)HUSTR_CHATMACRO9 }
};

int numdefaults;
char defaultfile[128];

/*
==============
=
= M_SaveDefaults
=
==============
*/

void M_SaveDefaults(void)
{
	int i, v;
	FILE *f;

	f = fopen(defaultfile, "w");
	if (!f)
		return; // can't write the file, but don't complain

	for (i = 0; i < numdefaults; i++) {
#ifdef __WATCOMC__
		if (defaults[i].scantranslate)
			defaults[i].location = &defaults[i].untranslated;
#endif
		if (defaults[i].defaultvalue > -0xfff &&
		    defaults[i].defaultvalue < 0xfff) {
			v = *defaults[i].location;
			fprintf(f, "%s\t\t%i\n", defaults[i].name, v);
		} else {
			fprintf(f, "%s\t\t\"%s\"\n", defaults[i].name,
				*(char **)(defaults[i].location));
		}
	}

	fclose(f);
}

//==========================================================================
//
// M_LoadDefaults
//
//==========================================================================

extern byte scantokey[128];

void M_LoadDefaults(char *fileName)
{
	int i;
	int len;
	FILE *f;
	char def[80];
	char strparm[100];
	char *newstring;
	int parm;
	boolean isstring;

	// Set everything to base values
	numdefaults = sizeof(defaults) / sizeof(defaults[0]);
	for (i = 0; i < numdefaults; i++) {
		*defaults[i].location = defaults[i].defaultvalue;
	}

	// Check for a custom config file
	i = M_CheckParm("-config");
	if (i && i < myargc - 1) {
		strcpy(defaultfile, myargv[i + 1]);
		ST_Message("config file: %s\n", defaultfile);
	} else if (cdrom) {
		sprintf(defaultfile, "c:\\hexndata\\%s", fileName);
	} else {
		strcpy(defaultfile, fileName);
	}

	// Scan the config file
	f = fopen(defaultfile, "r");
	if (f) {
		while (!feof(f)) {
			isstring = false;
			if (fscanf(f, "%79s %[^\n]\n", def, strparm) == 2) {
				if (strparm[0] == '"') {
					// Get a string default
					isstring = true;
					len = strlen(strparm);
					newstring = (char *)malloc(len);
					if (newstring == NULL)
						I_Error("can't malloc newstring");
					strparm[len - 1] = 0;
					strcpy(newstring, strparm + 1);
				} else if (strparm[0] == '0' &&
					   strparm[1] == 'x') {
					sscanf(strparm + 2, "%x", &parm);
				} else {
					sscanf(strparm, "%i", &parm);
				}
				for (i = 0; i < numdefaults; i++) {
					if (!strcmp(def, defaults[i].name)) {
						if (!isstring) {
							*defaults[i].location =
								parm;
						} else {
							*defaults[i].location =
								(int)newstring;
						}
						break;
					}
				}
			}
		}

		fclose(f);
	}

#ifdef __WATCOMC__
	// Translate the key scancodes
	for (i = 0; i < numdefaults; i++) {
		if (defaults[i].scantranslate) {
			parm = *defaults[i].location;
			defaults[i].untranslated = parm;
			*defaults[i].location = scantokey[parm];
		}
	}
#endif
}
