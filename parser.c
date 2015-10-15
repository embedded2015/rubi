#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "parser.h"
#include "expr.h"
#include "asm.h"

struct {
    Variable var[0xFF];
    int count;
} gblVar;

static Variable locVar[0xFF][0xFF];
static int varSize[0xFF], varCounter;

static int nowFunc; // number of function                                              
struct {
    char *text[0xff];
    int *addr;
    int count;
} strings;

struct {
    func_t func[0xff];
    int count, inside;
} functions;

#define NON 0

int32_t getString()
{
    strings.text[ strings.count ] =
        calloc(sizeof(char), strlen(tok.tok[tok.pos].val) + 1);
    strcpy(strings.text[strings.count], tok.tok[tok.pos++].val);

    *strings.addr++ = ntvCount;
    return strings.count++;
}

Variable *getVar(char *name)
{
    /* loval variable */
    for (int i = 0; i < varCounter; i++) {
        if (!strcmp(name, locVar[nowFunc][i].name))
            return &locVar[nowFunc][i];
    }
    /* global variable */
    for (int i = 0; i < gblVar.count; i++) {
        if (!strcmp(name, gblVar.var[i].name))
            return &gblVar.var[i];
    }
    return NULL;
}

static Variable *appendVar(char *name, int type)
{
    if (functions.inside == IN_FUNC) {
        int32_t sz = 1 + ++varSize[nowFunc];
        strcpy(locVar[nowFunc][varCounter].name, name);
        locVar[nowFunc][varCounter].type = type;
        locVar[nowFunc][varCounter].id = sz;
        locVar[nowFunc][varCounter].loctype = V_LOCAL;

        return &locVar[nowFunc][varCounter++];
    } else if (functions.inside == IN_GLOBAL) {
        /* global varibale */
        strcpy(gblVar.var[gblVar.count].name, name);
        gblVar.var[gblVar.count].type = type;
        gblVar.var[gblVar.count].loctype = V_GLOBAL;
        gblVar.var[gblVar.count].id = (uint32_t) &ntvCode[ntvCount];
        ntvCount += sizeof(int32_t); /* type */

        return &gblVar.var[gblVar.count++];
    }
    return NULL;
}

func_t *getFunc(char *name)
{
    for (int i = 0; i < functions.count; i++) {
        if (!strcmp(functions.func[i].name, name))
            return &functions.func[i];
    }
    return NULL;
}

static func_t *appendFunc(char *name, int address, int args)
{
    functions.func[functions.count].address = address;
    functions.func[functions.count].args = args;
    strcpy(functions.func[functions.count].name, name);
    return &functions.func[functions.count++];
}

static int32_t appendBreak()
{
    emit(0xe9); // jmp
    brks.addr = realloc(brks.addr, 4 * (brks.count + 1));
    brks.addr[brks.count] = ntvCount;
    emitI32(0);
    return brks.count++;
}

static int32_t appendReturn()
{
    relExpr(); /* get argument */
    emit(0xe9); // jmp
    rets.addr = realloc(rets.addr, 4 * (rets.count + 1));
    if (rets.addr == NULL) error("no enough memory");
    rets.addr[rets.count] = ntvCount;
    emitI32(0);
    return rets.count++;
}

int32_t skip(char *s)
{
    if (!strcmp(s, tok.tok[tok.pos].val)) {
        tok.pos++;
        return 1;
    }
    return 0;
}

int32_t error(char *errs, ...)
{
    va_list args;
    va_start(args, errs);
    printf("error: ");
    vprintf(errs, args);
    puts("");
    va_end(args);
    exit(0);
    return 0;
}

static int eval(int pos, int status)
{
    while (tok.pos < tok.size)
        if (expression(pos, status)) return 1;
    return 0;
}

static Variable *declareVariable()
{
    int32_t npos = tok.pos;
    if (isalpha(tok.tok[tok.pos].val[0])) {
        tok.pos++;
        if (skip(":")) {
            if (skip("int")) {
                --tok.pos;
                return appendVar(tok.tok[npos].val, T_INT);
            }
            if (skip("string")) {
                --tok.pos;
                return appendVar(tok.tok[npos].val, T_STRING);
            }
            if (skip("double")) {
                --tok.pos;
                return appendVar(tok.tok[npos].val, T_DOUBLE);
            }
        } else {
            --tok.pos;
            return appendVar(tok.tok[npos].val, T_INT);
        }
    } else error("%d: can't declare variable", tok.tok[tok.pos].nline);
    return NULL;
}

static int ifStmt()
{
    uint32_t end;
    relExpr(); /* if condition */
    emit(0x83); emit(0xf8); emit(0x00);// cmp eax, 0
    emit(0x75); emit(0x05); // jne 5
    emit(0xe9); end = ntvCount; emitI32(0);// jmp
    return eval(end, NON);
}

static int whileStmt()
{
    uint32_t loopBgn = ntvCount, end, stepBgn[2], stepOn = 0;
    relExpr(); /* condition */
    if (skip(",")) {
        stepOn = 1;
        stepBgn[0] = tok.pos;
        for (; tok.tok[tok.pos].val[0] != ';'; tok.pos++)
            /* next */;
    }
    emit(0x83); emit(0xf8); emit(0x00);// cmp eax, 0
    emit(0x75); emit(0x05); // jne 5
    emit(0xe9); end = ntvCount; emitI32(0);// jmp while end

    if (skip(":")) expression(0, BLOCK_LOOP);
    else eval(0, BLOCK_LOOP);

    if (stepOn) {
        stepBgn[1] = tok.pos;
        tok.pos = stepBgn[0];
        if (isassign()) assignment();
        tok.pos = stepBgn[1];
    }
    uint32_t n = 0xFFFFFFFF - ntvCount + loopBgn - 4;
    emit(0xe9); emitI32(n); // jmp n
    emitI32Insert(ntvCount - end - 4, end);

    for (--brks.count; brks.count >= 0; brks.count--)
        emitI32Insert(ntvCount - brks.addr[brks.count] - 4,
                      brks.addr[brks.count]);
    brks.count = 0;

    return 0;
}

static int32_t functionStmt()
{
    int32_t espBgn, argsc = 0;
    char *funcName = tok.tok[tok.pos++].val;
    nowFunc++; functions.inside = IN_FUNC;
    if (skip("(")) {
        do {
            declareVariable();
            tok.pos++;
            argsc++;
        } while(skip(","));
        skip(")");
    }
    appendFunc(funcName, ntvCount, argsc); // append function
    emit(0x50 + EBP); // push ebp
    emit(0x89); emit(0xc0 + ESP * 8 + EBP); // mov ebp esp
    espBgn = ntvCount + 2;
    emit(0x81); emit(0xe8 + ESP); emitI32(0); // sub esp 0 ; align
    int32_t argpos[128], i;
    for(i = 0; i < argsc; i++) {
        emit(0x8b); emit(0x45); emit(0x08 + (argsc - i - 1) * sizeof(int32_t));
        emit(0x89); emit(0x44); emit(0x24);
        argpos[i] = ntvCount; emit(0x00);
    }
    eval(0, BLOCK_FUNC);

    for (--rets.count; rets.count >= 0; --rets.count) {
        emitI32Insert(ntvCount - rets.addr[rets.count] - 4,
                      rets.addr[rets.count]);
    }
    rets.count = 0;

    emit(0x81); emit(0xc0 + ESP);
    emitI32(sizeof(int32_t) * (varSize[nowFunc] + 6)); // add esp nn
    emit(0xc9); // leave
    emit(0xc3); // ret

    emitI32Insert(sizeof(int32_t) * (varSize[nowFunc] + 6), espBgn);
    for (i = 1; i <= argsc; i++)
        ntvCode[argpos[i - 1]] = 256 - sizeof(int32_t) * i +
            (((varSize[nowFunc] + 6) * sizeof(int32_t)) - 4);

    return 0;
}

int expression(int pos, int status)
{
    int isputs = 0;

    if (skip("$")) { // global varibale?
        if (isassign()) assignment();
    } else if (skip("def")) {
        functionStmt();
    } else if (functions.inside == IN_GLOBAL &&
               strcmp("def", tok.tok[tok.pos+1].val) &&
               strcmp("$", tok.tok[tok.pos+1].val) &&
               strcmp(";", tok.tok[tok.pos+1].val)) { // main function entry
        functions.inside = IN_FUNC;
        nowFunc++;
        appendFunc("main", ntvCount, 0); // append function
        emit(0x50 + EBP); // push ebp
        emit(0x89); emit(0xc0 + ESP * 8 + EBP); // mov ebp esp
        uint32_t espBgn = ntvCount + 2;
        emit(0x81); emit(0xe8 + ESP); emitI32(0); // sub esp 0
        emit(0x8b); emit(0x75); emit(0x0c); // mov esi, 0xc(%ebp)

        eval(0, NON);

        emit(0x81); emit(0xc4);
        emitI32(sizeof(int32_t) * (varSize[nowFunc] + 6)); // add %esp nn
        emit(0xc9);// leave
        emit(0xc3);// ret
        emitI32Insert(sizeof(int32_t) * (varSize[nowFunc] + 6), espBgn);
        functions.inside = IN_GLOBAL;
    } else if (isassign()) {
        assignment();
    } else if ((isputs = skip("puts")) || skip("output")) {
        do {
            int isstring = 0;
            if (skip("\"")) {
                emit(0xb8); getString(); emitI32(0x00);
                    // mov eax string_address
                isstring = 1;
            } else {
                relExpr();
            }
            emit(0x50 + EAX); // push eax
            if (isstring) {
                emit(0xff); emit(0x56); emit(4);// call *0x04(esi) putString
            } else {
                emit(0xff); emit(0x16); // call (esi) putNumber
            }
            emit(0x81); emit(0xc0 + ESP); emitI32(4); // add esp 4
        } while (skip(","));
        /* new line ? */
        if (isputs) {
            emit(0xff); emit(0x56); emit(8);// call *0x08(esi) putLN
        }
    } else if(skip("printf")) {
        if (skip("\"")) {
            emit(0xb8); getString(); emitI32(0x00); // mov eax string_address
            emit(0x89); emit(0x44); emit(0x24); emit(0x00); // mov [esp+0], eax
        }
        if (skip(",")) {
            uint32_t a = 4;
            do {
                relExpr();
                emit(0x89); emit(0x44); emit(0x24); emit(a); // mov [esp+a], eax
                a += 4;
            } while(skip(","));
        }
        emit(0xff); emit(0x56); emit(12 + 8); // call printf
    } else if (skip("for")) {
        assignment();
        skip(",");
        whileStmt();
    } else if (skip("while")) {
        whileStmt();
    } else if(skip("return")) {
        appendReturn();
    } else if(skip("if")) {
        ifStmt();
    } else if(skip("else")) {
        int32_t end;
        emit(0xe9); end = ntvCount; emitI32(0);// jmp while end
        emitI32Insert(ntvCount - pos - 4, pos);
        eval(end, NON);
        return 1;
    } else if (skip("elsif")) {
        int32_t endif, end;
        emit(0xe9); endif = ntvCount; emitI32(0);// jmp while end
        emitI32Insert(ntvCount - pos - 4, pos);
        relExpr(); /* if condition */
        emit(0x83); emit(0xf8); emit(0x00);// cmp eax, 0
        emit(0x75); emit(0x05); // jne 5
        emit(0xe9); end = ntvCount; emitI32(0);// jmp while end
        eval(end, NON);
        emitI32Insert(ntvCount - endif - 4, endif);
        return 1;
    } else if (skip("break")) {
        appendBreak();
    } else if (skip("end")) {
        if (status == NON) {
            emitI32Insert(ntvCount - pos - 4, pos);
        } else if (status == BLOCK_FUNC) functions.inside = IN_GLOBAL;
        return 1;
    } else if (!skip(";")) { relExpr(); }

    return 0;
}

static char *replaceEscape(char *str)
{
    char escape[12][3] = {
        "\\a", "\a", "\\r", "\r", "\\f", "\f",
        "\\n", "\n", "\\t", "\t", "\\b", "\b"
    };
    for (int32_t i = 0; i < 12; i += 2) {
        char *pos;
        while ((pos = strstr(str, escape[i])) != NULL) {
            *pos = escape[i + 1][0];
            memmove(pos + 1, pos + 2, strlen(str) - 2 + 1);
        }
    }
    return str;
}

int32_t parser()
{
    tok.pos = ntvCount = 0;
    strings.addr = calloc(0xFF, sizeof(int32_t));
    uint32_t main_address;
    emit(0xe9); main_address = ntvCount; emitI32(0);
    eval(0, 0);

    uint32_t addr = getFunc("main")->address;
    emitI32Insert(addr - 5, main_address);

    for (strings.addr--; strings.count; strings.addr--) {
        emitI32Insert((uint32_t) &ntvCode[ntvCount], *strings.addr);
        replaceEscape(strings.text[--strings.count]);
        for (int32_t i = 0; strings.text[strings.count][i]; i++)
            emit(strings.text[strings.count][i]);
        emit(0); // '\0'
    }

    return 1;
}

int32_t isassign()
{
    char *val = tok.tok[tok.pos + 1].val;
    if (!strcmp(val, "=") || !strcmp(val, "++") || !strcmp(val, "--")) return 1;
    if (!strcmp(val, "[")) {
        int32_t i = tok.pos + 2, t = 1;
        while (t) {
            val = tok.tok[i].val;
            if (!strcmp(val, "[")) t++; if (!strcmp(val, "]")) t--;
            if (!strcmp(val, ";"))
                error("%d: invalid expression", tok.tok[tok.pos].nline);
            i++;
        }
        if (!strcmp(tok.tok[i].val, "=")) return 1;
    } else if (!strcmp(val, ":") && !strcmp(tok.tok[tok.pos + 3].val, "=")) {
        return 1;
    }
    return 0;
}

int32_t assignment()
{
    Variable *v = getVar(tok.tok[tok.pos].val);
    int32_t inc = 0, dec = 0, declare = 0;
    if (v == NULL) {
        declare++;
        v = declareVariable();
    }
    tok.pos++;

    if (v->loctype == V_LOCAL) {
        if (skip("[")) { // Array?
            relExpr();
            emit(0x50 + EAX); // push eax
            if (skip("]") && skip("=")) {
                relExpr();
                emit(0x8b); emit(0x4d);
                emit(256 -
                        (v->type == T_INT ? sizeof(int32_t) :
                         v->type == T_STRING ? sizeof(int32_t *) :
                         v->type == T_DOUBLE ? sizeof(double) : 4) * v->id);
                    // mov ecx [ebp-n]
                emit(0x58 + EDX); // pop edx
                if (v->type == T_INT) {
                    emit(0x89); emit(0x04); emit(0x91); // mov [ecx+edx*4], eax
                } else {
                    emit(0x89); emit(0x04); emit(0x11); // mov [ecx+edx], eax
                }
            } else if ((inc = skip("++")) || (dec = skip("--"))) {
            } else 
                error("%d: invalid assignment", tok.tok[tok.pos].nline);
        } else { // Scalar?
            if(skip("=")) {
                relExpr();
            } else if((inc = skip("++")) || (dec = skip("--"))) {
                emit(0x8b); emit(0x45);
                emit(256 -
                        (v->type == T_INT ? sizeof(int32_t) :
                         v->type == T_STRING ? sizeof(int32_t *) :
                         v->type == T_DOUBLE ? sizeof(double) : 4) * v->id);
                    // mov eax varaible
                emit(0x50 + EAX); // push eax
                if (inc) emit(0x40); // inc eax
                else if(dec) emit(0x48); // dec eax
            }
            emit(0x89); emit(0x45);
            emit(256 -
                    (v->type == T_INT ? sizeof(int32_t) :
                     v->type == T_STRING ? sizeof(int32_t *) :
                     v->type == T_DOUBLE ? sizeof(double) : 4) * v->id);
                // mov var eax
            if (inc || dec) emit(0x58 + EAX); // pop eax
        }
    } else if (v->loctype == V_GLOBAL) {
        if (declare) { // first declare for global variable?
            // assignment only int32_terger
            if (skip("=")) {
                unsigned *m = (unsigned *) v->id;
                *m = atoi(tok.tok[tok.pos++].val);
            }
        } else {
            if (skip("[")) { // Array?
                relExpr();
                emit(0x50 + EAX); // push eax
                if(skip("]") && skip("=")) {
                    relExpr();
                    emit(0x8b); emit(0x0d); emitI32(v->id);
                        // mov ecx GLOBAL_ADDR
                    emit(0x58 + EDX); // pop edx
                    if (v->type == T_INT) {
                        emit(0x89); emit(0x04); emit(0x91);
                            // mov [ecx+edx*4], eax
                    } else {
                        emit(0x89); emit(0x04); emit(0x11);
                            // mov [ecx+edx], eax
                    }
                } else error("%d: invalid assignment",
                             tok.tok[tok.pos].nline);
            } else if (skip("=")) {
                relExpr();
                emit(0xa3); emitI32(v->id); // mov GLOBAL_ADDR eax
            } else if ((inc = skip("++")) || (dec = skip("--"))) {
                emit(0xa1); emitI32(v->id); // mov eax GLOBAL_ADDR
                emit(0x50 + EAX); // push eax
                if (inc) emit(0x40); // inc eax
                else if (dec) emit(0x48); // dec eax
                emit(0xa3); emitI32(v->id); // mov GLOBAL_ADDR eax
            }
            if (inc || dec) emit(0x58 + EAX); // pop eax
        }
    }
    return 0;
}
