#ifndef __IVIDEO__
#define __IVIDEO__

void I_InitGraphics(void);

void I_ShutdownGraphics(void);

void I_SetPalette(unsigned char *palette);
// takes full 8 bit values

void I_Update(void);
// Copy buffer to video

void I_WaitVBL(int vbls);
// wait for vertical retrace or pause a bit

#endif
