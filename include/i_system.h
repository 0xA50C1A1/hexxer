#ifndef __ISYSTEM__
#define __ISYSTEM__

void *I_ZoneBase(int *size);
// called by startup code to get the ammount of memory to malloc
// for the zone management

int I_GetTime(void);
// called by H2_GameLoop
// returns current time in tics

void I_StartFrame(void);
// called by H2_GameLoop
// called before processing any tics in a frame (just after displaying a frame)
// time consuming syncronous operations are performed here (joystick reading)
// can call H2_PostEvent

void I_StartTic(void);
// called by H2_GameLoop
// called before processing each tic in a frame
// quick syncronous operations are performed here
// can call H2_PostEvent

void I_Init(void);
// called by H2_Main
// determines the hardware configuration and sets up the video mode

void I_Error(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
// called by anything that can generate a terminal error
// bad exit with diagnostic message

void I_Quit(void);
// called by M_Responder when quit is selected
// clean exit, displays sell blurb

#endif
