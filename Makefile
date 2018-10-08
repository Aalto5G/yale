.PHONY: all clean distclean

CFLAGS = -O3 -g -Wall -Wextra -Werror -Wno-unused-parameter

SRC := yaletest.c yaletopy.c yyutils.c httpmain.c httpmainprint.c yaleparser.c parser.c regex.c regexmain.c httpcmain.c httpcmainprint.c sslcmain.c lenprefixcmain.c sslcmainprint.c condtest.c httprespcmain.c
LEXSRC := yale.l
YACCSRC := yale.y

LEXGEN := $(patsubst %.l,%.lex.c,$(LEXSRC))
YACCGEN := $(patsubst %.y,%.tab.c,$(YACCSRC))

GEN := $(LEXGEN) $(YACCGEN) httpparser.c httpcparser.c lenprefixcparser.c ssl1cparser.c ssl2cparser.c ssl3cparser.c ssl4cparser.c ssl5cparser.c ssl6cparser.c condparsercparser.c httprespcparser.c

OBJ := $(patsubst %.c,%.o,$(SRC))
OBJGEN := $(patsubst %.c,%.o,$(GEN))

DEP := $(patsubst %.c,%.d,$(SRC))
DEPGEN := $(patsubst %.c,%.d,$(GEN))

all: yaletest yaletopy httpmain httpmainprint httpcmain httpcmainprint yaleparser regexmain lenprefixcmain sslcmain sslcmainprint condtest httprespcmain

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

condtest: condtest.o condparsercparser.o Makefile
	$(CC) $(CFLAGS) -o condtest condtest.o condparsercparser.o

httpmain: httpmain.o httpparser.o Makefile
	$(CC) $(CFLAGS) -o httpmain httpmain.o httpparser.o

httpmainprint: httpmainprint.o httpparser.o Makefile
	$(CC) $(CFLAGS) -o httpmainprint httpmainprint.o httpparser.o

httpcmain: httpcmain.o httpcparser.o Makefile
	$(CC) $(CFLAGS) -o httpcmain httpcmain.o httpcparser.o

httpcmainprint: httpcmainprint.o httpcparser.o Makefile
	$(CC) $(CFLAGS) -o httpcmainprint httpcmainprint.o httpcparser.o

lenprefixcmain: lenprefixcmain.o lenprefixcparser.o Makefile
	$(CC) $(CFLAGS) -o lenprefixcmain lenprefixcmain.o lenprefixcparser.o

regexmain: regexmain.o regex.o Makefile
	$(CC) $(CFLAGS) -o regexmain regexmain.o regex.o

httprespcmain: httprespcmain.o httprespcparser.o Makefile
	$(CC) $(CFLAGS) -o httprespcmain httprespcmain.o httprespcparser.o

httpmain.d: httpparser.h Makefile
httpmain.o: httpparser.h Makefile
httpmainprint.d: httpparser.h Makefile
httpmainprint.o: httpparser.h Makefile
httpparser.d: httpparser.h Makefile
httpparser.o: httpparser.h Makefile

httpcmain.d: httpcparser.h Makefile
httpcmain.o: httpcparser.h Makefile
httpcmainprint.d: httpcparser.h Makefile
httpcmainprint.o: httpcparser.h Makefile
httpcparser.d: httpcparser.h Makefile
httpcparser.o: httpcparser.h Makefile

httprespcmain.d: httprespcparser.h Makefile
httprespcmain.o: httprespcparser.h Makefile
httprespcparser.d: httprespcparser.h Makefile
httprespcparser.o: httprespcparser.h Makefile

lenprefixcmain.d: lenprefixcparser.h Makefile
lenprefixcmain.o: lenprefixcparser.h Makefile
lenprefixcparser.d: lenprefixcparser.h Makefile
lenprefixcparser.o: lenprefixcparser.h Makefile

condtest.d: condparsercparser.h Makefile
condtest.o: condparsercparser.h Makefile
condparsercparser.d: condparsercparser.h Makefile
condparsercparser.o: condparsercparser.h Makefile

http.py: yaletopy httppaper.txt
	./yaletopy httppaper.txt http.py

httpparser.h: http.py parser.py regex.py Makefile
	python http.py h

httpparser.c: http.py parser.py regex.py Makefile
	python http.py c

lenprefixcparser.h: yaleparser lenprefix.txt Makefile
	./yaleparser lenprefix.txt h

lenprefixcparser.c: yaleparser lenprefix.txt Makefile
	./yaleparser lenprefix.txt c

httpcparser.h: yaleparser httppaper.txt Makefile
	./yaleparser httppaper.txt h

httpcparser.c: yaleparser httppaper.txt Makefile
	./yaleparser httppaper.txt c

httprespcparser.h: yaleparser httpresp.txt Makefile
	./yaleparser httpresp.txt h

httprespcparser.c: yaleparser httpresp.txt Makefile
	./yaleparser httpresp.txt c

condparsercparser.h: yaleparser condparser.txt Makefile
	./yaleparser condparser.txt h

condparsercparser.c: yaleparser condparser.txt Makefile
	./yaleparser condparser.txt c

# ------ Begin SSL --------

sslcmain.d: ssl1cparser.h ssl2cparser.h ssl3cparser.h ssl4cparser.h ssl5cparser.h ssl6cparser.h Makefile
sslcmain.o: ssl1cparser.h ssl2cparser.h ssl3cparser.h ssl4cparser.h ssl5cparser.h ssl6cparser.h Makefile
sslcmainprint.d: ssl1cparser.h ssl2cparser.h ssl3cparser.h ssl4cparser.h ssl5cparser.h ssl6cparser.h Makefile
sslcmainprint.o: ssl1cparser.h ssl2cparser.h ssl3cparser.h ssl4cparser.h ssl5cparser.h ssl6cparser.h Makefile
ssl1cparser.d: ssl1cparser.h ssl2cparser.h ssl3cparser.h ssl4cparser.h ssl5cparser.h ssl6cparser.h Makefile
ssl1cparser.o: ssl1cparser.h ssl2cparser.h ssl3cparser.h ssl4cparser.h ssl5cparser.h ssl6cparser.h Makefile
ssl2cparser.d: ssl2cparser.h ssl3cparser.h ssl4cparser.h ssl5cparser.h ssl6cparser.h Makefile
ssl2cparser.o: ssl2cparser.h ssl3cparser.h ssl4cparser.h ssl5cparser.h ssl6cparser.h Makefile
ssl3cparser.d: ssl3cparser.h ssl4cparser.h ssl5cparser.h ssl6cparser.h Makefile
ssl3cparser.o: ssl3cparser.h ssl4cparser.h ssl5cparser.h ssl6cparser.h Makefile
ssl4cparser.d: ssl4cparser.h ssl5cparser.h ssl6cparser.h Makefile
ssl4cparser.o: ssl4cparser.h ssl5cparser.h ssl6cparser.h Makefile
ssl5cparser.d: ssl5cparser.h ssl6cparser.h Makefile
ssl5cparser.o: ssl5cparser.h ssl6cparser.h Makefile
ssl6cparser.d: ssl6cparser.h Makefile
ssl6cparser.o: ssl6cparser.h Makefile

ssl1cparser.h: yaleparser ssl1.txt Makefile
	./yaleparser ssl1.txt h

ssl1cparser.c: yaleparser ssl1.txt Makefile
	./yaleparser ssl1.txt c

ssl2cparser.h: yaleparser ssl2.txt Makefile
	./yaleparser ssl2.txt h

ssl2cparser.c: yaleparser ssl2.txt Makefile
	./yaleparser ssl2.txt c

ssl3cparser.h: yaleparser ssl3.txt Makefile
	./yaleparser ssl3.txt h

ssl3cparser.c: yaleparser ssl3.txt Makefile
	./yaleparser ssl3.txt c

ssl4cparser.h: yaleparser ssl4.txt Makefile
	./yaleparser ssl4.txt h

ssl4cparser.c: yaleparser ssl4.txt Makefile
	./yaleparser ssl4.txt c

ssl5cparser.h: yaleparser ssl5.txt Makefile
	./yaleparser ssl5.txt h

ssl5cparser.c: yaleparser ssl5.txt Makefile
	./yaleparser ssl5.txt c

ssl6cparser.h: yaleparser ssl6.txt Makefile
	./yaleparser ssl6.txt h

ssl6cparser.c: yaleparser ssl6.txt Makefile
	./yaleparser ssl6.txt c

sslcmain: sslcmain.o ssl1cparser.o ssl2cparser.o ssl3cparser.o ssl4cparser.o ssl5cparser.o ssl6cparser.o
	$(CC) $(CFLAGS) -o sslcmain sslcmain.o ssl1cparser.o ssl2cparser.o ssl3cparser.o ssl4cparser.o ssl5cparser.o ssl6cparser.o

sslcmainprint: sslcmainprint.o ssl1cparser.o ssl2cparser.o ssl3cparser.o ssl4cparser.o ssl5cparser.o ssl6cparser.o
	$(CC) $(CFLAGS) -o sslcmainprint sslcmainprint.o ssl1cparser.o ssl2cparser.o ssl3cparser.o ssl4cparser.o ssl5cparser.o ssl6cparser.o

# ------ End SSL --------

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
	rm -f httpcparser.c
	rm -f httpcparser.h
	rm -f lenprefixcparser.c
	rm -f lenprefixcparser.h
	rm -f httprespcparser.c
	rm -f httprespcparser.h
	rm -f condparsercparser.c
	rm -f condparsercparser.h
	rm -f httpparser.c
	rm -f httpparser.h
	rm -f ssl1cparser.c
	rm -f ssl1cparser.h
	rm -f ssl2cparser.c
	rm -f ssl2cparser.h
	rm -f ssl3cparser.c
	rm -f ssl3cparser.h
	rm -f ssl4cparser.c
	rm -f ssl4cparser.h
	rm -f ssl5cparser.c
	rm -f ssl5cparser.h
	rm -f ssl6cparser.c
	rm -f ssl6cparser.h
	rm -f http.py

distclean: clean
	rm -f yaletest
	rm -f yaletopy
	rm -f httpmain
	rm -f httpmainprint
	rm -f httpcmain
	rm -f httpcmainprint
	rm -f sslcmain
	rm -f lenprefixcmain
	rm -f yaleparser
	rm -f regexmain
	rm -f condtest
