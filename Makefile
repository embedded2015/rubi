CFLAGS = -Wall -m32 -std=gnu99 -O2
C = $(CC) $(CFLAGS)

rubi: engine.o expr.o parser.o stdlib.o
	$(C) -o $@ $^

engine.o: rubi.h engine.c
	$(C) -c engine.c

expr.o: expr.h expr.c asm.h
	$(C) -c expr.c

parser.o: parser.h parser.c asm.h
	$(C) -c parser.c

stdlib.o: stdlib.c asm.h expr.h
	$(C) -c stdlib.c

clean:
	$(RM) a.out rubi *.o *~ text
