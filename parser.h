#ifndef RUBI_PARSER_INCLUDED
#define RUBI_PARSER_INCLUDED

#include "rubi.h"

#include <stddef.h>

extern void* jit_buf;
extern size_t jit_sz;

extern int npc;

typedef struct {
    int address, args, espBgn;
    char name[0xFF];
} func_t;

int expression(int, int);

int (*parser())(int *, void **);
char* getString();

func_t *getFunc(char *);

typedef struct {
    char name[32];
    unsigned int id;
    int type;
    int loctype;
} Variable;
Variable *getVar(char *);

int isassign();
int assignment();

#endif
