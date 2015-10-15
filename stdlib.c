#include <string.h>
#include "expr.h"
#include "asm.h"

typedef struct {
    char *name;
    int args, addr;
} std_function;

static std_function stdfunc[] = {
    {"Array", 1, 12},
    {"rand", 0, 16}, {"printf", -1, 20}, {"sleep", 1, 28},
    {"fopen", 2, 32}, {"fprintf", -1, 36}, {"fclose", 1, 40}, {"fgets", 3, 44},
    {"free", 1, 48}, {"freeLocal", 0, 52}
};

int make_stdfunc(char *name)
{
    for (size_t i = 0; i < sizeof(stdfunc) / sizeof(stdfunc[0]); i++) {
        if (!strcmp(stdfunc[i].name, name)) {
            if(!strcmp(name, "Array")) {
                relExpr(); // get array size
                emit(0xc1); emit(0xe0 + EAX); emit(2); // shl eax 2
                emit(0x89); emit(0x04); emit(0x24); // mov [esp], eax
                emit(0xff); emit(0x56); emit(12); // call malloc
                emit(0x50 + EAX); // push eax
                emit(0x89); emit(0x04); emit(0x24); // mov [esp], eax
                emit(0xff); emit(0x56); emit(24); // call add_mem
                emit(0x58 + EAX); // pop eax
            } else {
                if (stdfunc[i].args == -1) { // vector
                    uint32_t a = 0;
                    do {
                        relExpr();
                        emit(0x89); emit(0x44); emit(0x24); emit(a);
                            // mov [esp+a], eax
                        a += 4;
                    } while(skip(","));
                } else {
                    for(int arg = 0; arg < stdfunc[i].args; arg++) {
                        relExpr();
                        emit(0x89); emit(0x44); emit(0x24); emit(arg * 4);
                            // mov [esp+arg*4], eax
                        skip(",");
                    }
                }
                emit(0xff); emit(0x56); emit(stdfunc[i].addr);
                    // call $function
            }
            return 1;
        }
    }
    return 0;
}
