#ifndef RUBI_ASM_INCLUDED
#define RUBI_ASM_INCLUDED

#include <stdint.h>

unsigned char *ntvCode;
int ntvCount;

enum { EAX = 0, ECX, EDX, EBX, ESP, EBP, ESI, EDI };

static inline void emit(unsigned char val)
{
    ntvCode[ntvCount++] = (val);
}
static inline void emitI32(unsigned int val)
{
    emit(val << 24 >> 24);
    emit(val << 16 >> 24);
    emit(val <<  8 >> 24);
    emit(val <<  0 >> 24);
}
static inline void emitI32Insert(unsigned int val, int pos)
{
    ntvCode[pos + 0] = (val << 24 >> 24);
    ntvCode[pos + 1] = (val << 16 >> 24);
    ntvCode[pos + 2] = (val <<  8 >> 24);
    ntvCode[pos + 3] = (val <<  0 >> 24);
}

#endif
