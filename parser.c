#include "dynasm/dasm_proto.h"
#include "dynasm/dasm_x86.h"
#if _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "parser.h"
#include "expr.h"

|.arch x86
|.globals L_
|.actionlist rubiactions

dasm_State* d;
static dasm_State** Dst = &d;
void* rubilabels[L__MAX];
void* jit_buf;
size_t jit_sz;

int npc;
static int main_address, mainFunc;

struct {
    Variable var[0xFF];
    int count;
    int data[0xFF];
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

void jit_init() {
    dasm_init(&d, 1);
    dasm_setupglobal(&d, rubilabels, L__MAX);
    dasm_setup(&d, rubiactions);
}

void* jit_finalize() {
    dasm_link(&d, &jit_sz);
#ifdef _WIN32
    jit_buf = VirtualAlloc(0, jit_sz, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
    jit_buf = mmap(0, jit_sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
    dasm_encode(&d, jit_buf);
#ifdef _WIN32
    {DWORD dwOld; VirtualProtect(jit_buf, jit_sz, PAGE_EXECUTE_READWRITE, &dwOld); }
#else
    mprotect(jit_buf, jit_sz, PROT_READ | PROT_WRITE | PROT_EXEC);
#endif
    return jit_buf;
}

char* getString()
{
    strings.text[ strings.count ] =
        calloc(sizeof(char), strlen(tok.tok[tok.pos].val) + 1);
    strcpy(strings.text[strings.count], tok.tok[tok.pos++].val);
    return strings.text[strings.count++];
}

Variable *getVar(char *name)
{
    /* local variable */
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
        gblVar.var[gblVar.count].id = (int)&gblVar.data[gblVar.count];
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

static func_t *appendFunc(char *name, int address, int espBgn, int args)
{
    functions.func[functions.count].address = address;
    functions.func[functions.count].espBgn = espBgn;
    functions.func[functions.count].args = args;
    strcpy(functions.func[functions.count].name, name);
    return &functions.func[functions.count++];
}

static int32_t appendBreak()
{
    uint32_t lbl = npc++;
    dasm_growpc(&d, npc);
    | jmp =>lbl
    brks.addr = realloc(brks.addr, 4 * (brks.count + 1));
    brks.addr[brks.count] = lbl;
    return brks.count++;
}

static int32_t appendReturn()
{
    relExpr(); /* get argument */

    int lbl = npc++;
    dasm_growpc(&d, npc);

    | jmp =>lbl

    rets.addr = realloc(rets.addr, 4 * (rets.count + 1));
    if (rets.addr == NULL) error("no enough memory");
    rets.addr[rets.count] = lbl;
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
    relExpr(); /* if condition */
    uint32_t end = npc++;
    dasm_growpc(&d, npc);
    // didn't simply 'jz =>end' to prevent address diff too large
    | test eax, eax
    | jnz >1
    | jmp =>end
    |1:
    return eval(end, NON);
}

static int whileStmt()
{
    uint32_t loopBgn = npc++;
    dasm_growpc(&d, npc);
    |=>loopBgn:
    relExpr(); /* condition */
    uint32_t stepBgn[2], stepOn = 0;
    if (skip(",")) {
        stepOn = 1;
        stepBgn[0] = tok.pos;
        for (; tok.tok[tok.pos].val[0] != ';'; tok.pos++)
            /* next */;
    }
    uint32_t end = npc++;
    dasm_growpc(&d, npc);
    | test eax, eax
    | jnz >1
    | jmp =>end
    |1:

    if (skip(":")) expression(0, BLOCK_LOOP);
    else eval(0, BLOCK_LOOP);

    if (stepOn) {
        stepBgn[1] = tok.pos;
        tok.pos = stepBgn[0];
        if (isassign()) assignment();
        tok.pos = stepBgn[1];
    }
    | jmp =>loopBgn
    |=>end:

    for (--brks.count; brks.count >= 0; brks.count--)
        |=>brks.addr[brks.count]:
    brks.count = 0;

    return 0;
}

static int32_t functionStmt()
{
    int32_t argsc = 0;
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
    int func_addr = npc++;
    dasm_growpc(&d, npc);

    int func_esp = npc++;
    dasm_growpc(&d, npc);

    appendFunc(funcName, func_addr, func_esp, argsc); // append function

    |=>func_addr:
    | push ebp
    | mov ebp, esp
    | sub esp, 0x80000000
    |=>func_esp:

    for(int i = 0; i < argsc; i++) {
        | mov eax, [ebp + ((argsc - i - 1) * sizeof(int32_t) + 8)]
        | mov [ebp - (i + 2)*4], eax
    }
    eval(0, BLOCK_FUNC);

    for (--rets.count; rets.count >= 0; --rets.count) {
        |=>rets.addr[rets.count]:
    }
    rets.count = 0;

    | leave
    | ret

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
               (tok.pos+1 == tok.size || strcmp(";", tok.tok[tok.pos+1].val))) { // main function entry
        functions.inside = IN_FUNC;
        mainFunc = ++nowFunc;

        int main_esp = npc++;
        dasm_growpc(&d, npc);

        appendFunc("main", main_address, main_esp, 0); // append function

        |=>main_address:
        | push ebp
        | mov ebp, esp
        | sub esp, 0x80000000
        |=>main_esp:
        | mov esi, [ebp + 12]
        eval(0, NON);
        | leave
        | ret

        functions.inside = IN_GLOBAL;
    } else if (isassign()) {
        assignment();
    } else if ((isputs = skip("puts")) || skip("output")) {
        do {
            int isstring = 0;
            if (skip("\"")) {
                // mov eax string_address
                | mov eax, getString()
                isstring = 1;
            } else {
                relExpr();
            }
            | push eax
            if (isstring) {
                | call dword [esi + 4]
            } else {
                | call dword [esi]
            }
            | add esp, 4
        } while (skip(","));
        /* new line ? */
        if (isputs) {
            | call dword [esi + 0x8]
        }
    } else if(skip("printf")) {
        // support maximum 5 arguments for now
        if (skip("\"")) {
            // mov eax string_address
            | mov eax, getString()
            | mov [esp], eax
        }
        if (skip(",")) {
            uint32_t a = 4;
            do {
                relExpr();
                | mov [esp + a], eax
                a += 4;
            } while(skip(","));
        }
        | call dword [esi + 0x14]
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
        int32_t end = npc++;
        dasm_growpc(&d, npc);
        // jmp if/elsif/while end
        | jmp =>end
        |=>pos:
        eval(end, NON);
        return 1;
    } else if (skip("elsif")) {
        int32_t endif = npc++;
        dasm_growpc(&d, npc);
        // jmp if/elsif/while end
        | jmp =>endif
        |=>pos:
        relExpr(); /* if condition */
        uint32_t end = npc++;
        dasm_growpc(&d, npc);
        | test eax, eax
        | jnz >1
        | jmp =>end
        |1:
        eval(end, NON);
        |=>endif:
        return 1;
    } else if (skip("break")) {
        appendBreak();
    } else if (skip("end")) {
        if (status == NON) {
            |=>pos:
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
            memmove(pos + 1, pos + 2, strlen(pos + 2) + 1);
        }
    }
    return str;
}

int (*parser())(int *, void **)
{
    jit_init();

    tok.pos = 0;
    strings.addr = calloc(0xFF, sizeof(int32_t));
    main_address = npc++;
    dasm_growpc(&d, npc);

    |->START:
    | jmp =>main_address
    eval(0, 0);

    for (int i = 0; i < strings.count; ++i)
        replaceEscape(strings.text[i]);

    uint8_t* buf = (uint8_t*)jit_finalize();

    for (int i = 0; i < functions.count; i++)
        *(int*)(buf + dasm_getpclabel(&d, functions.func[i].espBgn) - 4) = (varSize[i+1] + 6)*4;

    dasm_free(&d);
    return ((int (*)(int *, void **))rubilabels[L_START]);
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

    int siz = (v->type == T_INT ? sizeof(int32_t) :
               v->type == T_STRING ? sizeof(int32_t *) :
               v->type == T_DOUBLE ? sizeof(double) : 4);

    if (v->loctype == V_LOCAL) {
        if (skip("[")) { // Array?
            relExpr();
            | push eax
            if (skip("]") && skip("=")) {
                relExpr();
                // mov ecx, array
                | mov ecx, [ebp - siz*v->id]
                | pop edx
                if (v->type == T_INT) {
                    | mov [ecx+edx*4], eax
                } else {
                    | mov [ecx+edx], eax
                }
            } else if ((inc = skip("++")) || (dec = skip("--"))) {
                // TODO
            } else 
                error("%d: invalid assignment", tok.tok[tok.pos].nline);
        } else { // Scalar?
            if(skip("=")) {
                relExpr();
            } else if((inc = skip("++")) || (dec = skip("--"))) {
                // mov eax, varaible
                | mov eax, [ebp - siz*v->id]
                | push eax
                if (inc)
                    | inc eax
                else if(dec)
                    | dec eax
            }
            // mov variable, eax
            | mov [ebp - siz*v->id], eax
            if (inc || dec)
                | pop eax
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
                | push eax
                if(skip("]") && skip("=")) {
                    relExpr();
                    // mov ecx GLOBAL_ADDR
                    | mov ecx, [v->id]
                    | pop edx
                    if (v->type == T_INT) {
                        | mov [ecx + edx*4], eax
                    } else {
                        | mov [edx+edx], eax
                    }
                } else error("%d: invalid assignment",
                             tok.tok[tok.pos].nline);
            } else if (skip("=")) {
                relExpr();
                | mov [v->id], eax
            } else if ((inc = skip("++")) || (dec = skip("--"))) {
                // mov eax GLOBAL_ADDR
                | mov eax, [v->id]
                | push eax
                if (inc)
                    | inc eax
                else if (dec)
                    | dec eax
                // mov GLOBAL_ADDR eax
                | mov [v->id], eax
            }
            if (inc || dec)
                | pop eax
        }
    }
    return 0;
}
