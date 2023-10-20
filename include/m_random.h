#ifndef __MRANDOM__
#define __MRANDOM__

// Most damage defined using HITDICE
#define HITDICE(a) ((1 + (P_Random() & 7)) * a)

int M_Random(void);
// returns a number from 0 to 255

int P_Random(void);
// as M_Random, but used only by the play simulation

int P_SubRandom(void);

void M_ClearRandom(void);
// fix randoms for demos

#endif
