//**************************************************************************
//**
//** I_VIDEO.C
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include <SDL2/SDL.h>
#include <h2def.h>
#include <i_video.h>
#include <i_system.h>

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

int UpdateState;
int DisplayTicker = 0;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static SDL_Surface *raw_surface = NULL; // Paletted 8-bit surface
static SDL_Surface *argb_surface = NULL; // intermediate 32-bit ARGB surface

static SDL_Texture *render_texture = NULL;

static SDL_Window *sdl_window = NULL;
static SDL_Renderer *sdl_renderer = NULL;

// CODE --------------------------------------------------------------------

//--------------------------------------------------------------------------
//
// PROC I_InitGraphics
//
//--------------------------------------------------------------------------

void I_InitGraphics(void)
{
	sdl_window = SDL_CreateWindow("HeXeN", SDL_WINDOWPOS_CENTERED,
				      SDL_WINDOWPOS_CENTERED, SCREENWIDTH,
				      SCREENHEIGHT, 0);

	if (sdl_window == NULL) {
		I_Error("Couldn't create SDL2 window: %s\n", SDL_GetError());
	}

	uint32_t pixelformat = SDL_GetWindowPixelFormat(sdl_window);

	if (pixelformat == SDL_PIXELFORMAT_UNKNOWN) {
		I_Error("Couldn't get SDL2 window pixel format: %s\n",
			SDL_GetError());
	}

	sdl_renderer =
		SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);

	if (sdl_renderer == NULL) {
		I_Error("Couldn't create SDL2 renderer: %s\n", SDL_GetError());
	}

	SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
	SDL_RenderClear(sdl_renderer);
	SDL_RenderPresent(sdl_renderer);

	raw_surface = SDL_CreateRGBSurfaceWithFormat(
		0, SCREENWIDTH, SCREENHEIGHT, 8, SDL_PIXELFORMAT_INDEX8);

	if (raw_surface == NULL) {
		I_Error("Couldn't create 8-bit SDL2 surface: %s\n",
			SDL_GetError());
	}

	SDL_FillRect(raw_surface, NULL, 0);

	argb_surface = SDL_CreateRGBSurfaceWithFormat(
		0, SCREENWIDTH, SCREENHEIGHT, 32, pixelformat);

	if (argb_surface == NULL) {
		I_Error("Couldn't create intermediate ARGB SDL2 surface: %s\n",
			SDL_GetError());
	}

	SDL_FillRect(argb_surface, NULL, 0);

	render_texture = SDL_CreateTexture(sdl_renderer, pixelformat,
					   SDL_TEXTUREACCESS_STREAMING,
					   SCREENWIDTH, SCREENHEIGHT);

	if (render_texture == NULL) {
		I_Error("Couldn't create SDL2 render texture: %s\n",
			SDL_GetError());
	}

	screen = (byte *)raw_surface->pixels;

	I_SetPalette(W_CacheLumpName("PLAYPAL", PU_CACHE));

	SDL_ShowCursor(SDL_DISABLE);
}

//--------------------------------------------------------------------------
//
// PROC I_ShutdownGraphics
//
//--------------------------------------------------------------------------

void I_ShutdownGraphics(void)
{
	SDL_FreeSurface(raw_surface);
	SDL_FreeSurface(argb_surface);

	if (render_texture != NULL) {
		SDL_DestroyTexture(render_texture);
	}

	if (sdl_renderer != NULL) {
		SDL_DestroyRenderer(sdl_renderer);
	}

	if (sdl_window != NULL) {
		SDL_DestroyWindow(sdl_window);
	}
}

//--------------------------------------------------------------------------
//
// PROC I_WaitVBL
//
//--------------------------------------------------------------------------

void I_WaitVBL(int vbls)
{
	SDL_Delay((vbls * 1000) / (TICRATE << 1));
}

//--------------------------------------------------------------------------
//
// PROC I_SetPalette
//
// Palette source must use 8 bit RGB elements.
//
//--------------------------------------------------------------------------

void I_SetPalette(unsigned char *palette)
{
	SDL_Color colormap[256];

	for (int i = 0; i < 256; ++i) {
		colormap[i].r = gammatable[usegamma][*palette++];
		colormap[i].g = gammatable[usegamma][*palette++];
		colormap[i].b = gammatable[usegamma][*palette++];
	}

	SDL_SetPaletteColors(raw_surface->format->palette, colormap, 0, 256);
}

/*
==============
=
= I_Update
=
==============
*/

void I_Update(void)
{
	//
	// blit screen to video
	//
	if (DisplayTicker) {
		byte *dest = screen;
		static int lasttic;

		int i = I_GetTime();
		int tics = i - lasttic;
		lasttic = i;

		if (tics > 20) {
			tics = 20;
		}
		for (i = 0; i < tics; i++) {
			*dest = 0xFF;
			dest += 2;
		}
		for (i = tics; i < 20; i++) {
			*dest = 0x00;
			dest += 2;
		}
	}

	if (UpdateState == I_NOUPDATE) {
		return;
	}

	SDL_BlitSurface(raw_surface, NULL, argb_surface, NULL);
	SDL_UpdateTexture(render_texture, NULL, argb_surface->pixels,
			  argb_surface->pitch);

	SDL_RenderClear(sdl_renderer);
	SDL_RenderCopy(sdl_renderer, render_texture, NULL, NULL);
	SDL_RenderPresent(sdl_renderer);

	UpdateState = I_NOUPDATE;
}
