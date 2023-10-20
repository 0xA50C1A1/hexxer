
//**************************************************************************
//**
//** I_IBM.C
//**
//**************************************************************************

#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <graph.h>
#include "h2def.h"
#include "r_local.h"
#include "p_local.h" // for P_AproxDistance
#include "sounds.h"
#include "i_sound.h"
#include <s_sound.h>
#include "st_start.h"
#include "dmx.h"
#include "dpmiapi.h"

// Macros

#define DPMI_INT 0x31

#define SEQ_ADDR 0x3C4
#define SEQ_DATA 0x3C5
#define REG_MAPMASK 0x02

#define MASK_PLANE0 0x01
#define MASK_PLANE1 0x02
#define MASK_PLANE2 0x04
#define MASK_PLANE3 0x08

#define P0OFFSET 38400 * 0
#define P1OFFSET 38400 * 1
#define P2OFFSET 38400 * 2
#define P3OFFSET 38400 * 3

#define VID_INT 0x10
#define VB_SYNC                           \
	{                                 \
		while (!(inp(0x3da) & 8)) \
			;                 \
		while (inp(0x3da) & 8)    \
			;                 \
	}
#define BITPLANE(p) (outp(SEQ_ADDR, REG_MAPMASK), outp(SEQ_DATA, (p)))

//#define NOKBD
//#define NOTIMER

// Public Data

// Code

void main(int argc, char **argv)
{
	myargc = argc;
	myargv = argv;
	H2_Main();
}

void I_StartupNet(void);
void I_ShutdownNet(void);
void I_ReadExternDriver(void);

typedef struct {
	unsigned edi, esi, ebp, reserved, ebx, edx, ecx, eax;
	unsigned short flags, es, ds, fs, gs, ip, cs, sp, ss;
} dpmiregs_t;

extern dpmiregs_t dpmiregs;

void I_ReadMouse(void);

extern int usemouse, usejoystick;

int i_Vector;
externdata_t *i_ExternData;
boolean useexterndriver;

/*
============================================================================

							CONSTANTS

============================================================================
*/

#define SC_INDEX 0x3C4
#define SC_RESET 0
#define SC_CLOCK 1
#define SC_MAPMASK 2
#define SC_CHARMAP 3
#define SC_MEMMODE 4

#define CRTC_INDEX 0x3D4
#define CRTC_H_TOTAL 0
#define CRTC_H_DISPEND 1
#define CRTC_H_BLANK 2
#define CRTC_H_ENDBLANK 3
#define CRTC_H_RETRACE 4
#define CRTC_H_ENDRETRACE 5
#define CRTC_V_TOTAL 6
#define CRTC_OVERFLOW 7
#define CRTC_ROWSCAN 8
#define CRTC_MAXSCANLINE 9
#define CRTC_CURSORSTART 10
#define CRTC_CURSOREND 11
#define CRTC_STARTHIGH 12
#define CRTC_STARTLOW 13
#define CRTC_CURSORHIGH 14
#define CRTC_CURSORLOW 15
#define CRTC_V_RETRACE 16
#define CRTC_V_ENDRETRACE 17
#define CRTC_V_DISPEND 18
#define CRTC_OFFSET 19
#define CRTC_UNDERLINE 20
#define CRTC_V_BLANK 21
#define CRTC_V_ENDBLANK 22
#define CRTC_MODE 23
#define CRTC_LINECOMPARE 24

#define GC_INDEX 0x3CE
#define GC_SETRESET 0
#define GC_ENABLESETRESET 1
#define GC_COLORCOMPARE 2
#define GC_DATAROTATE 3
#define GC_READMAP 4
#define GC_MODE 5
#define GC_MISCELLANEOUS 6
#define GC_COLORDONTCARE 7
#define GC_BITMASK 8

#define ATR_INDEX 0x3c0
#define ATR_MODE 16
#define ATR_OVERSCAN 17
#define ATR_COLORPLANEENABLE 18
#define ATR_PELPAN 19
#define ATR_COLORSELECT 20

#define STATUS_REGISTER_1 0x3da

#define PEL_WRITE_ADR 0x3c8
#define PEL_READ_ADR 0x3c7
#define PEL_DATA 0x3c9
#define PEL_MASK 0x3c6

// boolean grmode;

//==================================================
//
// joystick vars
//
//==================================================

boolean joystickpresent;
extern unsigned joystickx, joysticky;
boolean I_ReadJoystick(void); // returns false if not connected

//==================================================

#define VBLCOUNTER 34000 // hardware tics to a frame

#define TIMERINT 8
#define KEYBOARDINT 9

#define CRTCOFF (_inbyte(STATUS_REGISTER_1) & 1)
#define CLI _disable()
#define STI _enable()

#define _outbyte(x, y) (outp(x, y))
#define _outhword(x, y) (outpw(x, y))

#define _inbyte(x) (inp(x))
#define _inhword(x) (inpw(x))

#define MOUSEB1 1
#define MOUSEB2 2
#define MOUSEB3 4

boolean mousepresent;
//static  int tsm_ID = -1; // tsm init flag

//===============================

int ticcount;

// REGS stuff used for int calls
union REGS regs;
struct SREGS segregs;

boolean novideo; // if true, stay in text mode for debugging

#define KBDQUESIZE 32
byte keyboardque[KBDQUESIZE];
int kbdtail, kbdhead;

#define KEY_LSHIFT 0xfe

#define KEY_INS (0x80 + 0x52)
#define KEY_DEL (0x80 + 0x53)
#define KEY_PGUP (0x80 + 0x49)
#define KEY_PGDN (0x80 + 0x51)
#define KEY_HOME (0x80 + 0x47)
#define KEY_END (0x80 + 0x4f)

#define SC_RSHIFT 0x36
#define SC_LSHIFT 0x2a

byte scantokey[128] = {
	//  0           1       2       3       4       5       6       7
	//  8           9       A       B       C       D       E       F
	0,
	27,
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	'9',
	'0',
	'-',
	'=',
	KEY_BACKSPACE,
	9, // 0
	'q',
	'w',
	'e',
	'r',
	't',
	'y',
	'u',
	'i',
	'o',
	'p',
	'[',
	']',
	13,
	KEY_RCTRL,
	'a',
	's', // 1
	'd',
	'f',
	'g',
	'h',
	'j',
	'k',
	'l',
	';',
	39,
	'`',
	KEY_LSHIFT,
	92,
	'z',
	'x',
	'c',
	'v', // 2
	'b',
	'n',
	'm',
	',',
	'.',
	'/',
	KEY_RSHIFT,
	'*',
	KEY_RALT,
	' ',
	0,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5, // 3
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	0,
	0,
	KEY_HOME,
	KEY_UPARROW,
	KEY_PGUP,
	'-',
	KEY_LEFTARROW,
	'5',
	KEY_RIGHTARROW,
	'+',
	KEY_END, //4
	KEY_DOWNARROW,
	KEY_PGDN,
	KEY_INS,
	KEY_DEL,
	0,
	0,
	0,
	KEY_F11,
	KEY_F12,
	0,
	0,
	0,
	0,
	0,
	0,
	0, // 5
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, // 6
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0 // 7
};

//==========================================================================

//--------------------------------------------------------------------------
//
// PROC I_ColorBorder
//
//--------------------------------------------------------------------------

/*
void I_ColorBorder(void)
{
	int i;

	I_WaitVBL(1);
	_outbyte(PEL_WRITE_ADR, 0);
	for(i = 0; i < 3; i++)
	{
		_outbyte(PEL_DATA, 63);
	}
}
*/

//--------------------------------------------------------------------------
//
// PROC I_UnColorBorder
//
//--------------------------------------------------------------------------

/*
void I_UnColorBorder(void)
{
	int i;

	I_WaitVBL(1);
	_outbyte(PEL_WRITE_ADR, 0);
	for(i = 0; i < 3; i++)
	{
		_outbyte(PEL_DATA, 0);
	}
}
*/

/*
============================================================================

								USER INPUT

============================================================================
*/

/*
============================================================================

							GRAPHICS MODE

============================================================================
*/

//--------------------------------------------------------------------------
//
// PROC I_ReadScreen
//
// Reads the screen currently displayed into a linear buffer.
//
//--------------------------------------------------------------------------

/*
void I_ReadScreen(byte *scr)
{
	memcpy(scr, screen, SCREENWIDTH*SCREENHEIGHT);
}
*/

//===========================================================================

/*
void   I_ReadKeys (void)
{
	int             k;


	while (1)
	{
	   while (kbdtail < kbdhead)
	   {
		   k = keyboardque[kbdtail&(KBDQUESIZE-1)];
		   kbdtail++;
		   printf ("0x%x\n",k);
		   if (k == 1)
			   I_Quit ();
	   }
	}
}
*/

/*
============================================================================

					TIMER INTERRUPT

============================================================================
*/

/*
void I_ColorBlack (int r, int g, int b)
{
_outbyte (PEL_WRITE_ADR,0);
_outbyte(PEL_DATA,r);
_outbyte(PEL_DATA,g);
_outbyte(PEL_DATA,b);
}
*/

/*
================
=
= I_TimerISR
=
================
*/

int I_TimerISR(void)
{
	ticcount++;
	return 0;
}

/*
============================================================================

						KEYBOARD

============================================================================
*/

void(__interrupt __far *oldkeyboardisr)() = NULL;

int lastpress;

/*
================
=
= I_KeyboardISR
=
================
*/

void __interrupt I_KeyboardISR(void)
{
	// Get the scan code

	keyboardque[kbdhead & (KBDQUESIZE - 1)] = lastpress = _inbyte(0x60);
	kbdhead++;

	// acknowledge the interrupt

	_outbyte(0x20, 0x20);
}

/*
===============
=
= I_StartupKeyboard
=
===============
*/

void I_StartupKeyboard(void)
{
#ifndef NOKBD
	oldkeyboardisr = _dos_getvect(KEYBOARDINT);
	_dos_setvect(0x8000 | KEYBOARDINT, I_KeyboardISR);
#endif

	//I_ReadKeys ();
}

void I_ShutdownKeyboard(void)
{
	if (oldkeyboardisr)
		_dos_setvect(KEYBOARDINT, oldkeyboardisr);
	*(short *)0x41c = *(short *)0x41a; // clear bios key buffer
}

/*
============================================================================

							MOUSE

============================================================================
*/

int I_ResetMouse(void)
{
	regs.w.ax = 0; // reset
	int386(0x33, &regs, &regs);
	return regs.w.ax;
}

/*
================
=
= StartupMouse
=
================
*/

void I_StartupCyberMan(void);

void I_StartupMouse(void)
{
	//
	// General mouse detection
	//
	mousepresent = 0;
	if (M_CheckParm("-nomouse") || !usemouse)
		return;

	if (I_ResetMouse() != 0xffff) {
		ST_Message("Mouse: not present\n");
		return;
	}
	ST_Message("Mouse: detected\n");

	mousepresent = 1;

	I_StartupCyberMan();
}

/*
================
=
= ShutdownMouse
=
================
*/

void I_ShutdownMouse(void)
{
	if (!mousepresent)
		return;

	I_ResetMouse();
}

/*
================
=
= I_ReadMouse
=
================
*/

void I_ReadMouse(void)
{
	event_t ev;

	//
	// mouse events
	//
	if (!mousepresent)
		return;

	ev.type = ev_mouse;

	memset(&dpmiregs, 0, sizeof(dpmiregs));
	dpmiregs.eax = 3; // read buttons / position
	DPMIInt(0x33);
	ev.data1 = dpmiregs.ebx;

	dpmiregs.eax = 11; // read counters
	DPMIInt(0x33);
	ev.data2 = (short)dpmiregs.ecx;
	ev.data3 = -(short)dpmiregs.edx;

	H2_PostEvent(&ev);
}

/*
============================================================================

					JOYSTICK

============================================================================
*/

int joyxl, joyxh, joyyl, joyyh;

boolean WaitJoyButton(void)
{
	int oldbuttons, buttons;

	oldbuttons = 0;
	do {
		I_WaitVBL(1);
		buttons = ((inp(0x201) >> 4) & 1) ^ 1;
		if (buttons != oldbuttons) {
			oldbuttons = buttons;
			continue;
		}

		if ((lastpress & 0x7f) == 1) {
			joystickpresent = false;
			return false;
		}
	} while (!buttons);

	do {
		I_WaitVBL(1);
		buttons = ((inp(0x201) >> 4) & 1) ^ 1;
		if (buttons != oldbuttons) {
			oldbuttons = buttons;
			continue;
		}

		if ((lastpress & 0x7f) == 1) {
			joystickpresent = false;
			return false;
		}
	} while (buttons);

	return true;
}

/*
===============
=
= I_StartupJoystick
=
===============
*/

int basejoyx, basejoyy;

void I_StartupJoystick(void)
{
	int centerx, centery;

	joystickpresent = 0;
	if (M_CheckParm("-nojoy") || !usejoystick)
		return;

	if (!I_ReadJoystick()) {
		joystickpresent = false;
		ST_Message("joystick not found\n ");
		return;
	}
	ST_Message("joystick found\n");
	joystickpresent = true;

	ST_RealMessage("CENTER the joystick and press button 1:");
	if (!WaitJoyButton())
		return;
	I_ReadJoystick();
	centerx = joystickx;
	centery = joysticky;

	ST_RealMessage(
		"\nPush the joystick to the UPPER LEFT corner and press button 1:");
	if (!WaitJoyButton())
		return;
	I_ReadJoystick();
	joyxl = (centerx + joystickx) / 2;
	joyyl = (centerx + joysticky) / 2;

	ST_RealMessage(
		"\nPush the joystick to the LOWER RIGHT corner and press button 1:");
	if (!WaitJoyButton())
		return;
	I_ReadJoystick();
	joyxh = (centerx + joystickx) / 2;
	joyyh = (centery + joysticky) / 2;
	ST_RealMessage("\n");
}

/*
===============
=
= I_JoystickEvents
=
===============
*/

void I_JoystickEvents(void)
{
	event_t ev;

	//
	// joystick events
	//
	if (!joystickpresent)
		return;

	I_ReadJoystick();
	ev.type = ev_joystick;
	ev.data1 = ((inp(0x201) >> 4) & 15) ^ 15;

	if (joystickx < joyxl)
		ev.data2 = -1;
	else if (joystickx > joyxh)
		ev.data2 = 1;
	else
		ev.data2 = 0;
	if (joysticky < joyyl)
		ev.data3 = -1;
	else if (joysticky > joyyh)
		ev.data3 = 1;
	else
		ev.data3 = 0;

	H2_PostEvent(&ev);
}

/*
============================================================================

					DPMI STUFF

============================================================================
*/

#define REALSTACKSIZE 1024

dpmiregs_t dpmiregs;

unsigned realstackseg;

void I_DivException(void);
int I_SetDivException(void);

/*
void DPMIFarCall (void)
{
	segread (&segregs);
	regs.w.ax = 0x301;
	regs.w.bx = 0;
	regs.w.cx = 0;
	regs.x.edi = (unsigned)&dpmiregs;
	segregs.es = segregs.ds;
	int386x( DPMI_INT, &regs, &regs, &segregs );
}
*/

void DPMIInt(int i)
{
	dpmiregs.ss = realstackseg;
	dpmiregs.sp = REALSTACKSIZE - 4;

	segread(&segregs);
	regs.w.ax = 0x300;
	regs.w.bx = i;
	regs.w.cx = 0;
	regs.x.edi = (unsigned)&dpmiregs;
	segregs.es = segregs.ds;
	int386x(DPMI_INT, &regs, &regs, &segregs);
}

/*
==============
=
= I_StartupDPMI
=
==============
*/

void I_StartupDPMI(void)
{
	//	extern char __begtext;
	//	extern char ___argc;
	//	int     n,d;

	//
	// allocate a decent stack for real mode ISRs
	//
	realstackseg = (int)I_AllocLow(1024) >> 4;

//
// lock the entire program down
//

//      _dpmi_lockregion (&__begtext, &___argc - &__begtext);

//
// catch divide by 0 exception
//
#if 0
	segread(&segregs);
	regs.w.ax = 0x0203;             // DPMI set processor exception handler vector
	regs.w.bx = 0;                  // int 0
	regs.w.cx = segregs.cs;
	regs.x.edx = (int)&I_DivException;
 printf ("%x : %x\n",regs.w.cx, regs.x.edx);
	int386( DPMI_INT, &regs, &regs);
#endif

#if 0
	n = I_SetDivException ();
	printf ("return: %i\n",n);
	n = 100;
	d = 0;
   printf ("100 / 0 = %i\n",n/d);

exit (1);
#endif
}

/*
============================================================================

					TIMER INTERRUPT

============================================================================
*/

void(__interrupt __far *oldtimerisr)();

/*
void IO_ColorBlack (int r, int g, int b)
{
_outbyte (PEL_WRITE_ADR,0);
_outbyte(PEL_DATA,r);
_outbyte(PEL_DATA,g);
_outbyte(PEL_DATA,b);
}
*/

/*
================
=
= IO_TimerISR
=
================
*/

//void __interrupt IO_TimerISR (void)

void __interrupt __far IO_TimerISR(void)
{
	ticcount++;
	_outbyte(0x20, 0x20); // Ack the interrupt
}

/*
=====================
=
= IO_SetTimer0
=
= Sets system timer 0 to the specified speed
=
=====================
*/

void IO_SetTimer0(int speed)
{
	if (speed > 0 && speed < 150)
		I_Error("INT_SetTimer0: %i is a bad value", speed);

	_outbyte(0x43, 0x36); // Change timer 0
	_outbyte(0x40, speed);
	_outbyte(0x40, speed >> 8);
}

/*
===============
=
= IO_StartupTimer
=
===============
*/

/*
void IO_StartupTimer (void)
{
	oldtimerisr = _dos_getvect(TIMERINT);

	_dos_setvect (0x8000 | TIMERINT, IO_TimerISR);
	IO_SetTimer0 (VBLCOUNTER);
}
*/

void IO_ShutdownTimer(void)
{
	if (oldtimerisr) {
		IO_SetTimer0(0); // back to 18.4 ips
		_dos_setvect(TIMERINT, oldtimerisr);
	}
}

//===========================================================================

/*
=============
=
= I_AllocLow
=
=============
*/

byte *I_AllocLow(int length)
{
	byte *mem;

	// DPMI call 100h allocates DOS memory
	segread(&segregs);
	regs.w.ax = 0x0100; // DPMI allocate DOS memory
	regs.w.bx = (length + 15) / 16;
	int386(DPMI_INT, &regs, &regs);
	//      segment = regs.w.ax;
	//   selector = regs.w.dx;
	if (regs.w.cflag != 0)
		I_Error("I_AllocLow: DOS alloc of %i failed, %i free", length,
			regs.w.bx * 16);

	mem = (void *)((regs.x.eax & 0xFFFF) << 4);

	memset(mem, 0, length);
	return mem;
}

/*
============================================================================

						NETWORKING

============================================================================
*/

/* // FUCKED LINES
typedef struct
{
	char    priv[508];
} doomdata_t;
*/ // FUCKED LINES

#define DOOMCOM_ID 0x12345678l

/* // FUCKED LINES
typedef struct
{
	long    id;
	short   intnum;                 // DOOM executes an int to execute commands

// communication between DOOM and the driver
	short   command;                // CMD_SEND or CMD_GET
	short   remotenode;             // dest for send, set by get (-1 = no packet)
	short   datalength;             // bytes in doomdata to be sent

// info common to all nodes
	short   numnodes;               // console is allways node 0
	short   ticdup;                 // 1 = no duplication, 2-5 = dup for slow nets
	short   extratics;              // 1 = send a backup tic in every packet
	short   deathmatch;             // 1 = deathmatch
	short   savegame;               // -1 = new game, 0-5 = load savegame
	short   episode;                // 1-3
	short   map;                    // 1-9
	short   skill;                  // 1-5

// info specific to this node
	short   consoleplayer;
	short   numplayers;
	short   angleoffset;    // 1 = left, 0 = center, -1 = right
	short   drone;                  // 1 = drone

// packet data to be sent
	doomdata_t      data;
} doomcom_t;
*/ // FUCKED LINES

extern doomcom_t *doomcom;

/*
====================
=
= I_InitNetwork
=
====================
*/

void I_InitNetwork(void)
{
	int i;

	i = M_CheckParm("-net");
	if (!i) {
		//
		// single player game
		//
		doomcom = malloc(sizeof(*doomcom));
		memset(doomcom, 0, sizeof(*doomcom));
		netgame = false;
		doomcom->id = DOOMCOM_ID;
		doomcom->numplayers = doomcom->numnodes = 1;
		doomcom->deathmatch = false;
		doomcom->consoleplayer = 0;
		doomcom->ticdup = 1;
		doomcom->extratics = 0;
		return;
	}

	netgame = true;
	doomcom = (doomcom_t *)atoi(myargv[i + 1]);
	//DEBUG
	doomcom->skill = startskill;
	doomcom->episode = startepisode;
	doomcom->map = startmap;
	doomcom->deathmatch = deathmatch;
}

void I_NetCmd(void)
{
	if (!netgame)
		I_Error("I_NetCmd when not in netgame");
	DPMIInt(doomcom->intnum);
}

//=========================================================================
//
// I_CheckExternDriver
//
//		Checks to see if a vector, and an address for an external driver
//			have been passed.
//=========================================================================

void I_CheckExternDriver(void)
{
	int i;

	if (!(i = M_CheckParm("-externdriver"))) {
		return;
	}
	i_ExternData = (externdata_t *)atoi(myargv[i + 1]);
	i_Vector = i_ExternData->vector;

	useexterndriver = true;
}

//=========================================================================
//=========================================================================
// Hi-Res (mode 12) stuff
//=========================================================================
//=========================================================================

//==========================================================================
//
// SetVideoModeHR - Set video mode to 640x480x16
//
//==========================================================================

void SetVideoModeHR(void)
{
	union REGS regs;
	regs.x.eax = 0x12;
	int386(VID_INT, &regs, &regs);
}

//==========================================================================
//
// ClearScreenHR - Clear the screen to color 0
//
//==========================================================================

void ClearScreenHR(void)
{
	BITPLANE(MASK_PLANE0 | MASK_PLANE1 | MASK_PLANE2 | MASK_PLANE3);
	memset((char *)0xa0000, 0, 38400);
}

//==========================================================================
//
// SlamHR - copy 4-plane buffer to screen
//
//==========================================================================

void SlamHR(char *buffer)
{
	BITPLANE(MASK_PLANE0);
	memcpy((char *)0xA0000, buffer + P0OFFSET, 38400);
	BITPLANE(MASK_PLANE1);
	memcpy((char *)0xA0000, buffer + P1OFFSET, 38400);
	BITPLANE(MASK_PLANE2);
	memcpy((char *)0xA0000, buffer + P2OFFSET, 38400);
	BITPLANE(MASK_PLANE3);
	memcpy((char *)0xA0000, buffer + P3OFFSET, 38400);
}

//==========================================================================
//
// SlamHR - copy 4-plane buffer to screen
//
// X and Width should be a multiple of 8
// src should be 4 planes of block size, back to back
//==========================================================================

void SlamBlockHR(int x, int y, int w, int h, char *src)
{
	int srcwid = w >> 3;
	char *dest = ((char *)0xA0000) + (y * (640 / 8)) + (x >> 3);
	char *dst;
	int i;

	VB_SYNC;

	BITPLANE(MASK_PLANE0);
	dst = dest;
	for (i = 0; i < h; i++) {
		memcpy(dst, src, srcwid);
		dst += 640 / 8;
		src += srcwid;
	}
	BITPLANE(MASK_PLANE1);
	dst = dest;
	for (i = 0; i < h; i++) {
		memcpy(dst, src, srcwid);
		dst += 640 / 8;
		src += srcwid;
	}
	BITPLANE(MASK_PLANE2);
	dst = dest;
	for (i = 0; i < h; i++) {
		memcpy(dst, src, srcwid);
		dst += 640 / 8;
		src += srcwid;
	}
	BITPLANE(MASK_PLANE3);
	dst = dest;
	for (i = 0; i < h; i++) {
		memcpy(dst, src, srcwid);
		dst += 640 / 8;
		src += srcwid;
	}
}

//==========================================================================
//
// InitPaletteHR
//
//==========================================================================

void InitPaletteHR(void)
{
	int i;
	union REGS regs;

	// Set palette registers to point into color registers
	for (i = 0; i < 16; i++) {
		regs.x.eax = (0x10 << 8) | 0x00;
		regs.x.ebx = (i << 8) | i;
		int386(VID_INT, &regs, &regs);
	}
}

//==========================================================================
//
// SetPaletteHR - Set the HR palette
//
//==========================================================================

void SetPaletteHR(byte *palette)
{
	int i;
	VB_SYNC;
	outp(PEL_WRITE_ADR, 0);

	for (i = 0; i < 16 * 3; i++) {
		outp(PEL_DATA, (*palette++));
	}
}

//==========================================================================
//
// GetPaletteHR - Get the HR palette
//
//==========================================================================

void GetPaletteHR(byte *palette)
{
	int i;
	outp(PEL_READ_ADR, 0);
	for (i = 0; i < 16 * 3; i++) {
		*palette++ = inp(PEL_DATA);
	}
}

//==========================================================================
//
// FadeToPaletteHR
//
//==========================================================================

void FadeToPaletteHR(byte *palette)
{
	int i, j;
	int steps = 140; // two-seconds
	byte basep[16 * 3];
	byte work[16 * 3];
	int delta;

	GetPaletteHR(basep);
	for (i = 0; i < steps; i++) {
		for (j = 0; j < 16 * 3; j++) {
			delta = palette[j] - basep[j];
			work[j] = basep[j] + delta * i / steps;
		}
		SetPaletteHR(work);
	}
	SetPaletteHR(palette);
}

//==========================================================================
//
// FadeToBlackHR - Fades the palette out to black
//
//==========================================================================

/*
void FadeToBlackHR(void)
{
	char work[16*3];
	char base[16*3];
	int i,j,steps=70;

	GetPaletteHR(base);
	for (i=0; i<steps; i++)
	{
		for (j=0; j<16*3; j++)
		{
			work[j] = base[j]-(base[j]*i/steps);
		}
		VB_SYNC;
		SetPaletteHR(work);
	}
	memset(work,0,16*3);
	SetPaletteHR(work);
}
*/

//==========================================================================
//
// BlackPaletteHR - Instantly blacks out the palette
//
//==========================================================================

void BlackPaletteHR(void)
{
	char blackpal[16 * 3];

	memset(blackpal, 0, 16 * 3);
	SetPaletteHR(blackpal);
}

//==========================================================================
//
//
// I_StartupReadKeys
//
//
//==========================================================================

void I_StartupReadKeys(void)
{
	int k;

	while (kbdtail < kbdhead) {
		k = keyboardque[kbdtail & (KBDQUESIZE - 1)];
		kbdtail++;
		if (k == 1)
			I_Quit();
	}
}
