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
	*size = MAXIMUM_HEAP_SIZE;
	return malloc(*size);
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
	// TODO: Rewrite using SDL_GetTicks64

	static uint32_t basetime = 0;

	if (basetime == 0) {
		basetime = SDL_GetTicks();
	}

	return ((SDL_GetTicks() - basetime) * TICRATE) / 1000;
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

static int xlatekey(SDL_Keysym *key)
{
	switch (key->sym) {
	case SDLK_RIGHT:
		return KEY_RIGHTARROW;
	case SDLK_LEFT:
		return KEY_LEFTARROW;
	case SDLK_UP:
		return KEY_UPARROW;
	case SDLK_DOWN:
		return KEY_DOWNARROW;
	case SDLK_ESCAPE:
		return KEY_ESCAPE;
	case SDLK_RETURN:
	case SDLK_KP_ENTER:
		return KEY_ENTER;
	case SDLK_F1:
		return KEY_F1;
	case SDLK_F2:
		return KEY_F2;
	case SDLK_F3:
		return KEY_F3;
	case SDLK_F4:
		return KEY_F4;
	case SDLK_F5:
		return KEY_F5;
	case SDLK_F6:
		return KEY_F6;
	case SDLK_F7:
		return KEY_F7;
	case SDLK_F8:
		return KEY_F8;
	case SDLK_F9:
		return KEY_F9;
	case SDLK_F10:
		return KEY_F10;
	case SDLK_F11:
		return KEY_F11;
	case SDLK_F12:
		return KEY_F12;

	case SDLK_BACKSPACE:
		return KEY_BACKSPACE;
	case SDLK_PAUSE:
		return KEY_PAUSE;

	case SDLK_EQUALS:
	case SDLK_KP_EQUALS:
		return KEY_EQUALS;
	case SDLK_MINUS:
	case SDLK_KP_MINUS:
		return KEY_MINUS;

	case SDLK_RSHIFT:
	case SDLK_LSHIFT:
		return KEY_RSHIFT;
	case SDLK_RCTRL:
	case SDLK_LCTRL:
		return KEY_RCTRL;
	case SDLK_RALT:
	case SDLK_LALT:
		return KEY_RALT;

	case SDLK_BACKSLASH:
		return KEY_BACKSLASH;

	default:
		return key->sym;
	}
}

static int xlatemouse(uint32_t buttonstate)
{
	// clang-format off
	return 0
	       | ((buttonstate & SDL_BUTTON(1)) ? 1 : 0)
	       | ((buttonstate & SDL_BUTTON(2)) ? 2 : 0)
	       | ((buttonstate & SDL_BUTTON(3)) ? 4 : 0);
	// clang-format on
}

void I_StartTic(void)
{
	SDL_Event sdl_ev;
	event_t hexen_ev;

	while (SDL_PollEvent(&sdl_ev)) {
		switch (sdl_ev.type) {
		case SDL_KEYDOWN:
			hexen_ev.type = ev_keydown;
			hexen_ev.data1 = xlatekey(&sdl_ev.key.keysym);
			H2_PostEvent(&hexen_ev);
			break;
		case SDL_KEYUP:
			hexen_ev.type = ev_keyup;
			hexen_ev.data1 = xlatekey(&sdl_ev.key.keysym);
			H2_PostEvent(&hexen_ev);
			break;
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
			hexen_ev.type = ev_mouse;
			hexen_ev.data1 =
				xlatemouse(SDL_GetMouseState(NULL, NULL));
			hexen_ev.data2 = 0;
			hexen_ev.data3 = 0;
			H2_PostEvent(&hexen_ev);
		case SDL_MOUSEMOTION:
			if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
				hexen_ev.data1 =
					xlatemouse(sdl_ev.motion.state);
				hexen_ev.data2 = (sdl_ev.motion.xrel << 3);
				hexen_ev.data3 = -(sdl_ev.motion.yrel << 3);
				H2_PostEvent(&hexen_ev);
			}
			break;
		case SDL_QUIT:
			I_Quit();
			break;
		}
	}
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

	va_list ap;
	va_start(ap, fmt);
	SDL_LogMessageV(SDL_LOG_CATEGORY_ERROR, SDL_LOG_PRIORITY_CRITICAL, fmt,
			ap);
	va_end(ap);

	exit(-1);
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
	va_list ap;
	va_start(ap, fmt);
	SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO,
			fmt, ap);
	va_end(ap);
}
