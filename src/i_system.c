//**************************************************************************
//**
//** I_SYSTEM.C
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include <SDL2/SDL.h>
#include <h2def.h>
#include <i_system.h>
#include <i_video.h>
#include <s_sound.h>

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void I_Shutdown(void);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

/*
===============
=
= I_ZoneBase
=
===============
*/

void *I_ZoneBase(int *size)
{
	return NULL; // stub
}

//--------------------------------------------------------------------------
//
// FUNC I_GetTime
//
// Returns time in 1/35th second tics.
//
//--------------------------------------------------------------------------

int I_GetTime(void)
{
	return 0; // stub
}

/*
===============
=
= I_StartFrame
=
===============
*/

void I_StartFrame(void)
{
	// nothing to do there
}

/*
===================
=
= I_StartTic
=
// called by D_DoomLoop
// called before processing each tic in a frame
// can call D_PostEvent
// asyncronous interrupt functions should maintain private ques that are
// read by the syncronous functions to be converted into events
===================
*/

void I_StartTic(void)
{
	// TODO
}

/*
===============
=
= I_Init
=
= hook interrupts and set graphics mode
=
===============
*/

void I_Init(void)
{
	ST_Message("  S_Init... ");
	S_Init();
	S_Start();
}

/*
===============
=
= I_Shutdown
=
= return to default system state
=
===============
*/

void I_Shutdown(void)
{
	I_ShutdownGraphics();
	S_ShutDown();
	SDL_Quit();
}

/*
================
=
= I_Error
=
================
*/

void I_Error(const char *fmt, ...)
{
	D_QuitNetGame();
	I_Shutdown();

	// TODO

	exit(1);
}

//--------------------------------------------------------------------------
//
// I_Quit
//
// Shuts down net game, saves defaults, prints the exit text message,
// goes to text mode, and exits.
//
//--------------------------------------------------------------------------

void I_Quit(void)
{
	D_QuitNetGame();
	M_SaveDefaults();
	I_Shutdown();

	printf("\nHexen: Beyond Heretic\n");
	exit(0);
}

//==========================================================================
//
// ST_Message - gives debug message
//
//==========================================================================

void ST_Message(const char *fmt, ...)
{
}
