.PHONY: all clean distclean

CFLAGS = -O3 -g -Wall -Wextra -Werror -Wno-unused-parameter

SRC := yaletest.c yaletopy.c yyutils.c httpmain.c httpmainprint.c yaleparser.c parser.c regex.c regexmain.c
LEXSRC := yale.l
YACCSRC := yale.y

LEXGEN := $(patsubst %.l,%.lex.c,$(LEXSRC))
YACCGEN := $(patsubst %.y,%.tab.c,$(YACCSRC))

GEN := $(LEXGEN) $(YACCGEN) httpparser.c

OBJ := $(patsubst %.c,%.o,$(SRC))
OBJGEN := $(patsubst %.c,%.o,$(GEN))

DEP := $(patsubst %.c,%.d,$(SRC))
DEPGEN := $(patsubst %.c,%.d,$(GEN))

all: yaletest yaletopy httpmain httpmainprint yaleparser regexmain

$(DEP): %.d: %.c Makefile
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c

$(DEPGEN): %.d: %.c %.h Makefile
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c

$(OBJ): %.o: %.c %.d Makefile
	$(CC) $(CFLAGS) -c -o $*.o $*.c
$(OBJGEN): %.o: %.c %.h %.d Makefile
	$(CC) $(CFLAGS) -c -o $*.o $*.c -Wno-sign-compare -Wno-missing-prototypes -Wno-sign-conversion

-include *.d

yaleparser: yaleparser.o yale.lex.o yale.tab.o yyutils.o parser.o regex.o Makefile
	$(CC) $(CFLAGS) -o yaleparser yaleparser.o yale.lex.o yale.tab.o yyutils.o parser.o regex.o

httpmain: httpmain.o httpparser.o Makefile
	$(CC) $(CFLAGS) -o httpmain httpmain.o httpparser.o

regexmain: regexmain.o regex.o Makefile
	$(CC) $(CFLAGS) -o regexmain regexmain.o regex.o

httpmainprint: httpmainprint.o httpparser.o Makefile
	$(CC) $(CFLAGS) -o httpmainprint httpmainprint.o httpparser.o

httpmain.d: httpparser.h Makefile
httpmain.o: httpparser.h Makefile
httpparser.d: httpparser.h Makefile
httpparser.o: httpparser.h Makefile

http.py: yaletopy httppaper.txt
	./yaletopy httppaper.txt http.py

httpparser.h: http.py parser.py regex.py Makefile
	python http.py h

httpparser.c: http.py parser.py regex.py Makefile
	python http.py c

yaletest: yaletest.o yale.lex.o yale.tab.o yyutils.o Makefile
	$(CC) $(CFLAGS) -o yaletest yaletest.o yale.lex.o yale.tab.o yyutils.o

yaletopy: yaletopy.o yale.lex.o yale.tab.o yyutils.o Makefile
	$(CC) $(CFLAGS) -o yaletopy yaletopy.o yale.lex.o yale.tab.o yyutils.o

YALE.LEX.INTERMEDIATE: yale.l Makefile
	mkdir -p intermediatestore
	flex --outfile=intermediatestore/yale.lex.c --header-file=intermediatestore/yale.lex.h yale.l
	touch YALE.LEX.INTERMEDIATE
YALE.TAB.INTERMEDIATE: yale.y Makefile
	mkdir -p intermediatestore
	bison --defines=intermediatestore/yale.tab.h --output=intermediatestore/yale.tab.c yale.y
	touch YALE.TAB.INTERMEDIATE
yale.lex.c: YALE.LEX.INTERMEDIATE Makefile
	cp intermediatestore/yale.lex.c .
yale.lex.h: YALE.LEX.INTERMEDIATE Makefile
	cp intermediatestore/yale.lex.h .
yale.tab.c: YALE.TAB.INTERMEDIATE Makefile
	cp intermediatestore/yale.tab.c .
yale.tab.h: YALE.TAB.INTERMEDIATE Makefile
	cp intermediatestore/yale.tab.h .

yale.lex.d: yale.tab.h yale.lex.h Makefile
yale.lex.o: yale.tab.h yale.lex.h Makefile
yale.tab.d: yale.lex.h yale.tab.h Makefile
yale.tab.o: yale.lex.h yale.tab.h Makefile

clean:
	rm -f $(OBJ) $(OBJGEN) $(DEP) $(DEPGEN)
	rm -rf intermediatestore
	rm -f YALE.TAB.INTERMEDIATE
	rm -f YALE.LEX.INTERMEDIATE
	rm -f yale.lex.c
	rm -f yale.lex.h
	rm -f yale.tab.c
	rm -f yale.tab.h
	rm -f httpparser.c
	rm -f httpparser.h
	rm -f http.py

distclean: clean
	rm -f yaletest
	rm -f yaletopy
	rm -f httpmain
	rm -f httpmainprint
