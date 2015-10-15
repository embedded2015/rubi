#include "rubi.h"
#include "parser.h"
#include "asm.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/wait.h>

struct {
    uint32_t addr[0xff];
    int count;
} mem;

static unsigned int w;
static void set_xor128() { w = 1234 + (getpid() ^ 0xFFBA9285); }

void init()
{
    long memsz = 0xFFFF + 1;
    if (posix_memalign((void **) &ntvCode, memsz, memsz))
        perror("posix_memalign");
    if (mprotect(ntvCode, memsz, PROT_READ | PROT_WRITE | PROT_EXEC))
        perror("mprotect");
    tok.pos = ntvCount = 0;
    tok.size = 0xfff;
    set_xor128();
    tok.tok = calloc(sizeof(Token), tok.size);
    brks.addr = calloc(sizeof(uint32_t), 1);
    rets.addr = calloc(sizeof(uint32_t), 1);
}

static void freeAddr()
{
    if (mem.count > 0) {
        for (--mem.count; mem.count >= 0; --mem.count)
            free((void *)mem.addr[mem.count]);
        mem.count = 0;
    }
}

void dispose()
{
    free(ntvCode);
    free(brks.addr);
    free(rets.addr);
    free(tok.tok);
    freeAddr();
}

int32_t lex(char *code)
{
    int32_t codeSize = strlen(code), line = 1;
    int32_t is_crlf = 0;

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

static void put_i32(int32_t n) { printf("%d", n); }
static void put_str(int32_t *n) { printf("%s", (char *) n); }
static void put_ln() { printf("\n"); }

static void ssleep(uint32_t t) { usleep(t * CLOCKS_PER_SEC / 1000); }

static void add_mem(int32_t addr) { mem.addr[mem.count++] = addr; }

static int xor128()
{
    static uint32_t x = 123456789, y = 362436069, z = 521288629;
    uint32_t t;
    t = x ^ (x << 11);
    x = y; y = z; z = w;
    w = (w ^ (w >> 19)) ^ (t ^ (t >> 8));
    return ((int32_t) w < 0) ? -(int32_t) w : (int32_t) w;
}

static void *funcTable[] = {
    put_i32, /*  0 */ put_str, /*  4 */ put_ln, /*   8 */ malloc, /* 12 */
    xor128,  /* 16 */ printf,  /* 20 */ add_mem, /* 24 */ ssleep, /* 28 */
    fopen,   /* 32 */ fprintf, /* 36 */ fclose,  /* 40 */ fgets,   /* 44 */
    free,    /* 48 */ freeAddr  /* 52 */
};

static int execute(char *source)
{
    init();
    lex(source);
    parser();
    ((int (*)(int *, void **)) ntvCode)(0, funcTable);
    dispose();
    return 0;
}

int main(int argc, char **argv)
{
    char *src;

    if (argc < 2) error("no given filename");

    FILE *fp = fopen(argv[1], "rb");
    size_t ssz = 0;

    if (!fp) {
        perror("file not found");
        exit(0);
    }
    fseek(fp, 0, SEEK_END);
    ssz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    src = calloc(sizeof(char), ssz + 2);
    fread(src, sizeof(char), ssz, fp);
    fclose(fp);

    return execute(src);
}
