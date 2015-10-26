CC = gcc
CFLAGS = -Wall -m32 -g -std=gnu99
C = $(CC) $(CFLAGS)

rubi: engine.o codegen.o
	$(C) -o $@ $^

engine.o: engine.c rubi.h
	$(C) -o $@ -c engine.c

codegen.o: parser.h parser.dasc expr.dasc stdlib.dasc
	type parser.dasc expr.dasc stdlib.dasc | minilua dynasm/dynasm.lua -o codegen.c -D WIN -
	$(C) -o $@ -c codegen.c

clean:
	$(RM) a.out rubi *.o *~ text codegen.c
