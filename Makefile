ARCH = WIN
CC = gcc
CFLAGS = -Wall -m32 -g -std=gnu99
C = $(CC) $(CFLAGS)

rubi: engine.o expr.o parser.o stdlib.o
	$(C) -o $@ $^

%.$(ARCH).c: %.c
	minilua dynasm/dynasm.lua -o $@ -D $(ARCH) $<

engine.o: rubi.h engine.$(ARCH).c
	$(C) -o $@ -c engine.$(ARCH).c

expr.o: expr.h expr.$(ARCH).c
	$(C) -o $@ -c expr.$(ARCH).c

parser.o: parser.h parser.$(ARCH).c
	$(C) -o $@ -c parser.$(ARCH).c

stdlib.o: stdlib.c expr.h
	$(C) -c stdlib.c

clean:
	$(RM) a.out rubi *.o *~ text *.$(ARCH).c
