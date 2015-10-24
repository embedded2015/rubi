#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "expr.h"
#include "rubi.h"
#include "parser.h"

extern int make_stdfunc(char *);

static inline int32_t isIndex() { return !strcmp(tok.tok[tok.pos].val, "["); }

static void primExpr()
{
    if (isdigit(tok.tok[tok.pos].val[0])) { // number?
        | mov eax, atoi(tok.tok[tok.pos++].val)
    } else if (skip("'")) { // char?
        | mov eax, tok.tok[tok.pos++].val[0]
        skip("'");
    } else if (skip("\"")) { // string?
        // mov eax string_address
        | mov eax, getString()
    } else if (isalpha(tok.tok[tok.pos].val[0])) { // variable or inc or dec
        char *name = tok.tok[tok.pos].val;
        Variable *v;

        if (isassign()) assignment();
        else {
            tok.pos++;
            if (skip("[")) { // Array?
                if ((v = getVar(name)) == NULL)
                    error("%d: '%s' was not declared",
                          tok.tok[tok.pos].nline, name);
                relExpr();
                | mov ecx, eax

                if (v->loctype == V_LOCAL) {
                    | mov edx, [ebp - v->id*4]
                } else if (v->loctype == V_GLOBAL) {
                    // mov edx, GLOBAL_ADDR
                    | mov edx, [v->id]
                }

                if (v->type == T_INT) {
                    | mov eax, [edx + ecx * 4]
                } else {
                    | movzx eax, byte [edx + ecx]
                }

                if (!skip("]"))
                    error("%d: expected expression ']'",
                          tok.tok[tok.pos].nline);
            } else if (skip("(")) { // Function?
                if (!make_stdfunc(name)) {	// standard function
                    func_t *function = getFunc(name);
                    char *val = tok.tok[tok.pos].val;
                    if (isalpha(val[0]) || isdigit(val[0]) ||
                        !strcmp(val, "\"") || !strcmp(val, "(")) { // has arg?
                        for (int i = 0; i < function->args; i++) {
                            relExpr();
                            | push eax
                            skip(",");
                        }
                    }
                    // call func
                    | call =>function->address
                    | add esp, function->args * sizeof(int32_t)
                }
                if (!skip(")"))
                    error("func: %d: expected expression ')'",
                          tok.tok[tok.pos].nline);
            } else {
                if ((v = getVar(name)) == NULL)
                    error("var: %d: '%s' was not declared",
                          tok.tok[tok.pos].nline, name);
                if (v->loctype == V_LOCAL) {
                    // mov eax variable
                    | mov eax, [ebp - v->id*4]
                } else if (v->loctype == V_GLOBAL) {
                    // mov eax GLOBAL_ADDR
                    | mov eax, [v->id]
                }
            }
        }
    } else if (skip("(")) {
        if (isassign()) assignment(); else relExpr();
        if (!skip(")")) error("%d: expected expression ')'",
                              tok.tok[tok.pos].nline);
    }

    while (isIndex()) {
        | mov ecx, eax
        skip("[");
        relExpr();
        skip("]");
        | mov eax, [ecx + eax*4]
    }
}

static void mulDivExpr()
{
    int32_t mul = 0, div = 0;
    primExpr();
    while ((mul = skip("*")) || (div = skip("/")) || skip("%")) {
        | push eax
        primExpr();
        | mov ebx, eax
        | pop eax
        if (mul) {
            | imul ebx
        } else if (div) {
            | xor edx, edx
            | idiv ebx
        } else { /* mod */
            | xor edx, edx
            | idiv ebx
            | mov eax, edx
        }
    }
}

static void addSubExpr()
{
    int32_t add;
    mulDivExpr();
    while ((add = skip("+")) || skip("-")) {
        | push eax
        mulDivExpr();
        | mov ebx, eax
        | pop eax
        if (add) {
            | add eax, ebx
        } else {
            | sub eax, ebx
        }
    }
}

static void condExpr()
{
    int32_t lt = 0, gt = 0, ne = 0, eql = 0, fle = 0;
    addSubExpr();
    if ((lt = skip("<")) || (gt = skip(">")) || (ne = skip("!=")) ||
        (eql = skip("==")) || (fle = skip("<=")) || skip(">=")) {
        | push eax
        addSubExpr();
        | mov ebx, eax
        | pop eax
        | cmp eax, ebx
        if (lt)
            | setl al
        else if (gt)
            | setg al
        else if (ne)
            | setne al
        else if (eql)
            | sete al
        else if (fle)
            | setle al
        else
            | setge al

        | movzx eax, al
    }
}

void relExpr()
{
    int and = 0, or = 0;
    condExpr();
    while ((and = skip("and") || skip("&")) ||
           (or = skip("or") || skip("|")) || (skip("xor") || skip("^"))) {
        | push eax
        condExpr();
        | mov ebx, eax
        | pop eax
        if (and)
            | and eax, ebx
        else if (or)
            | or eax, ebx
        else
            | xor eax, ebx
    }
}
