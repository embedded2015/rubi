#ifndef RUBI_INCLUDED
#define RUBI_INCLUDED

enum { IN_GLOBAL = 0, IN_FUNC };

enum { BLOCK_LOOP = 1, BLOCK_FUNC };

typedef struct { char val[32]; int nline; } Token;
struct {
    Token *tok;
    int size, pos;
} tok;

enum { V_LOCAL, V_GLOBAL };

enum { T_INT, T_STRING, T_DOUBLE };

struct {
    unsigned int *addr;
    int count;
} brks, rets;

int error(char *, ...);

#endif
