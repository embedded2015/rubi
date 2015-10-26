CC = gcc
CFLAGS = -Wall -m32 -g -std=gnu99
C = $(CC) $(CFLAGS)

rubi: engine.o codegen.o
	$(C) -o $@ $^

minilua: dynasm/minilua.c
	$(CC) -Wall -std=gnu99 -O2 -o $@ $< -lm

engine.o: engine.c rubi.h
	$(C) -o $@ -c engine.c

codegen.o: parser.h parser.dasc expr.dasc stdlib.dasc minilua
	cat parser.dasc expr.dasc stdlib.dasc | ./minilua dynasm/dynasm.lua -o codegen.c -
	$(C) -o $@ -c codegen.c

clean:
	$(RM) a.out rubi minilua *.o *~ text codegen.c
