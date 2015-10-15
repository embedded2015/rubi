#ifndef RUBI_PARSER_INCLUDED
#define RUBI_PARSER_INCLUDED

#include "rubi.h"

typedef struct {
    int address, args;
    char name[0xFF];
} func_t;

int expression(int, int);

int parser();
int getString();

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
