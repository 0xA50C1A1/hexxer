#ifndef __MFIXED__
#define __MFIXED__

#define FRACBITS 16
#define FRACUNIT (1 << FRACBITS)

typedef int fixed_t;

fixed_t FixedMul(fixed_t a, fixed_t b);
fixed_t FixedDiv(fixed_t a, fixed_t b);

#endif
