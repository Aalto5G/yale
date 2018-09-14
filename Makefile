.PHONY: all clean distclean

SRC := yaletest.c yyutils.c
LEXSRC := yale.l
YACCSRC := yale.y

LEXGEN := $(patsubst %.l,%.lex.c,$(LEXSRC))
YACCGEN := $(patsubst %.y,%.tab.c,$(YACCSRC))

GEN := $(LEXGEN) $(YACCGEN)

OBJ := $(patsubst %.c,%.o,$(SRC))
OBJGEN := $(patsubst %.c,%.o,$(GEN))

DEP := $(patsubst %.c,%.d,$(SRC))
DEPGEN := $(patsubst %.c,%.d,$(GEN))

all: yaletest

$(DEP): %.d: %.c Makefile
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c

$(DEPGEN): %.d: %.c %.h Makefile
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c

$(OBJ): %.o: %.c %.d Makefile
	$(CC) $(CFLAGS) -c -o $*.o $*.c
$(OBJGEN): %.o: %.c %.h %.d Makefile
	$(CC) $(CFLAGS) -c -o $*.o $*.c -Wno-sign-compare -Wno-missing-prototypes -Wno-sign-conversion

-include *.d

yaletest: yaletest.o yale.lex.o yale.tab.o yyutils.o
	cc -o yaletest yaletest.o yale.lex.o yale.tab.o yyutils.o
YALE.LEX.INTERMEDIATE: yale.l
	mkdir -p intermediatestore
	flex --outfile=intermediatestore/yale.lex.c --header-file=intermediatestore/yale.lex.h yale.l
	touch YALE.LEX.INTERMEDIATE
YALE.TAB.INTERMEDIATE: yale.y
	mkdir -p intermediatestore
	bison --defines=intermediatestore/yale.tab.h --output=intermediatestore/yale.tab.c yale.y
	touch YALE.TAB.INTERMEDIATE
yale.lex.c: YALE.LEX.INTERMEDIATE
	cp intermediatestore/yale.lex.c .
yale.lex.h: YALE.LEX.INTERMEDIATE
	cp intermediatestore/yale.lex.h .
yale.tab.c: YALE.TAB.INTERMEDIATE
	cp intermediatestore/yale.tab.c .
yale.tab.h: YALE.TAB.INTERMEDIATE
	cp intermediatestore/yale.tab.h .

yale.lex.d: yale.tab.h yale.lex.h
yale.lex.o: yale.tab.h yale.lex.h
yale.tab.d: yale.lex.h yale.tab.h
yale.tab.o: yale.lex.h yale.tab.h

clean:
	rm -f $(OBJ) $(OBJGEN) $(DEP) $(DEPGEN)
	rm -rf intermediatestore
	rm -f YALE.TAB.INTERMEDIATE
	rm -f YALE.LEX.INTERMEDIATE
	rm -f yale.lex.c
	rm -f yale.lex.h
	rm -f yale.tab.c
	rm -f yale.tab.h

distclean: clean
	rm -f yaletest
