#include "rubi.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

int32_t lex(char *code)
{
    int32_t codeSize = strlen(code), line = 1;
    int is_crlf = 0;

    for (int32_t i = 0; i < codeSize; i++) {
        if (tok.size <= i)
            tok.tok = realloc(tok.tok, (tok.size += 512 * sizeof(Token)));
        if (isdigit(code[i])) { // number?
            for (; isdigit(code[i]); i++)
                strncat(tok.tok[tok.pos].val, &(code[i]), 1);
            tok.tok[tok.pos].nline = line;
            i--;
            tok.pos++;
        } else if (isalpha(code[i])) { // ident?
            char *str = tok.tok[tok.pos].val;
            for (; isalpha(code[i]) || isdigit(code[i]) || code[i] == '_'; i++)
                *str++ = code[i];
            tok.tok[tok.pos].nline = line;
            i--;
            tok.pos++;
        } else if (code[i] == ' ' || code[i] == '\t') { // space or tab?
        } else if (code[i] == '#') { // comment?
            for (i++; code[i] != '\n'; i++) { } line++;
        } else if (code[i] == '"') { // string?
            strcpy(tok.tok[tok.pos].val, "\"");
            tok.tok[tok.pos++].nline = line;
            for (i++; code[i] != '"' && code[i] != '\0'; i++)
                strncat(tok.tok[tok.pos].val, &(code[i]), 1);
            tok.tok[tok.pos].nline = line;
            if (code[i] == '\0')
                error("%d: expected expression '\"'",
                      tok.tok[tok.pos].nline);
            tok.pos++;
        } else if (code[i] == '\n' ||
                   (is_crlf = (code[i] == '\r' && code[i + 1] == '\n'))) {
            i += is_crlf;
            strcpy(tok.tok[tok.pos].val, ";");
            tok.tok[tok.pos].nline = line++;
            tok.pos++;
        } else {
            strncat(tok.tok[tok.pos].val, &(code[i]), 1);
            if (code[i + 1] == '=' || (code[i] == '+' && code[i + 1] == '+') ||
                (code[i] == '-' && code[i + 1] == '-'))
                strncat(tok.tok[tok.pos].val, &(code[++i]), 1);
            tok.tok[tok.pos].nline = line;
            tok.pos++;
        }
    }
    tok.tok[tok.pos].nline = line;
    tok.size = tok.pos - 1;

    return 0;
}
