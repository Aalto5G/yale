.PHONY: all clean distclean

CC = clang
CFLAGS = -Ofast -g -Wall -Wextra -Werror -Wno-unused-parameter -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -msse4 -mavx -mavx2 -msse4a -mbmi -mbmi2 -march=skylake -fomit-frame-pointer

SRC := yaletest.c yaletopy.c yyutils.c httpmain.c httpmainprint.c yaleparser.c parser.c regex.c regexmain.c httpcmain.c httpcmainprint.c sslcmain.c lenprefixcmain.c sslcmainprint.c condtest.c httprespcmain.c unit.c recursivecbmain.c backtracktestmain.c backtracktestcbmain.c reprefixcmain.c tokentheft1main.c
LEXSRC := yale.l
YACCSRC := yale.y

LEXGEN := $(patsubst %.l,%.lex.c,$(LEXSRC))
YACCGEN := $(patsubst %.y,%.tab.c,$(YACCSRC))

GEN := $(LEXGEN) $(YACCGEN) httpparser.c httpcparser.c lenprefixcparser.c ssl1cparser.c ssl2cparser.c ssl3cparser.c ssl4cparser.c ssl5cparser.c ssl6cparser.c condparsercparser.c httprespcparser.c recursivecbcparser.c backtracktestcparser.c backtracktestcbcparser.c reprefixcparser.c tokentheft1cparser.c

OBJ := $(patsubst %.c,%.o,$(SRC))
OBJGEN := $(patsubst %.c,%.o,$(GEN))

DEP := $(patsubst %.c,%.d,$(SRC))
DEPGEN := $(patsubst %.c,%.d,$(GEN))

all: yaletest yaletopy httpmain httpmainprint httpcmain httpcmainprint yaleparser regexmain lenprefixcmain sslcmain sslcmainprint condtest httprespcmain unit recursivecbmain backtracktestmain backtracktestcbmain reprefixcmain tokentheft1main parserunit

$(DEP): %.d: %.c Makefile
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c

$(DEPGEN): %.d: %.c %.h Makefile
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c

$(OBJ): %.o: %.c %.d Makefile
	$(CC) $(CFLAGS) -c -o $*.o $*.c
$(OBJGEN): %.o: %.c %.h %.d Makefile
	$(CC) $(CFLAGS) -c -o $*.o $*.c

-include *.d

yaleparser: yaleparser.o yale.lex.o yale.tab.o yyutils.o parser.o regex.o Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^)

.PHONY: check
check: all
	./parserunit

unit: unit.o parser.o regex.o Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^)

parserunit: parserunit.o Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^)

condtest: condtest.o condparsercparser.o Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^)

httpmain: httpmain.o httpparser.o Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^)

httpmainprint: httpmainprint.o httpparser.o Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^)

httpcmain: httpcmain.o httpcparser.o Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^)

httpcmainprint: httpcmainprint.o httpcparser.o Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^)

lenprefixcmain: lenprefixcmain.o lenprefixcparser.o Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^)

reprefixcmain: reprefixcmain.o reprefixcparser.o Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^)

regexmain: regexmain.o regex.o Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^)

httprespcmain: httprespcmain.o httprespcparser.o Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^)

recursivecbmain: recursivecbmain.o recursivecbcparser.o Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^)

backtracktestmain: backtracktestmain.o backtracktestcparser.o Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^)

backtracktestcbmain: backtracktestcbmain.o backtracktestcbcparser.o Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^)

tokentheft1main: tokentheft1main.o tokentheft1cparser.o Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^)

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

recursivecbmain.d: recursivecbcparser.h Makefile
recursivecbmain.o: recursivecbcparser.h Makefile
recursivecbcparser.d: recursivecbcparser.h Makefile
recursivecbcparser.o: recursivecbcparser.h Makefile

httprespcmain.d: httprespcparser.h Makefile
httprespcmain.o: httprespcparser.h Makefile
httprespcparser.d: httprespcparser.h Makefile
httprespcparser.o: httprespcparser.h Makefile

backtracktestmain.d: backtracktestcparser.h Makefile
backtracktestmain.o: backtracktestcparser.h Makefile
backtracktestcparser.d: backtracktestcparser.h Makefile
backtracktestcparser.o: backtracktestcparser.h Makefile

backtracktestcbmain.d: backtracktestcbcparser.h Makefile
backtracktestcbmain.o: backtracktestcbcparser.h Makefile
backtracktestcbcparser.d: backtracktestcbcparser.h Makefile
backtracktestcbcparser.o: backtracktestcbcparser.h Makefile

lenprefixcmain.d: lenprefixcparser.h Makefile
lenprefixcmain.o: lenprefixcparser.h Makefile
lenprefixcparser.d: lenprefixcparser.h Makefile
lenprefixcparser.o: lenprefixcparser.h Makefile

reprefixcmain.d: reprefixcparser.h Makefile
reprefixcmain.o: reprefixcparser.h Makefile
reprefixcparser.d: reprefixcparser.h Makefile
reprefixcparser.o: reprefixcparser.h Makefile

tokentheft1main.d: tokentheft1cparser.h Makefile
tokentheft1main.o: tokentheft1cparser.h Makefile
tokentheft1cparser.d: tokentheft1cparser.h Makefile
tokentheft1cparser.o: tokentheft1cparser.h Makefile

condtest.d: condparsercparser.h Makefile
condtest.o: condparsercparser.h Makefile
condparsercparser.d: condparsercparser.h Makefile
condparsercparser.o: condparsercparser.h Makefile

http.py: httppaper.txt yaletopy
	./yaletopy $< $@

httpparser.h: http.py parser.py regex.py Makefile
	python $< h

httpparser.c: http.py parser.py regex.py Makefile
	python $< c

lenprefixcparser.h: lenprefix.txt yaleparser Makefile
	./yaleparser lenprefix.txt h

lenprefixcparser.c: lenprefix.txt yaleparser Makefile
	./yaleparser lenprefix.txt c

reprefixcparser.h: reprefix.txt yaleparser Makefile
	./yaleparser reprefix.txt h

reprefixcparser.c: reprefix.txt yaleparser Makefile
	./yaleparser reprefix.txt c

httpcparser.h: httppaper.txt yaleparser Makefile
	./yaleparser httppaper.txt h

httpcparser.c: httppaper.txt yaleparser Makefile
	./yaleparser httppaper.txt c

httprespcparser.h: httpresp.txt yaleparser Makefile
	./yaleparser httpresp.txt h

httprespcparser.c: httpresp.txt yaleparser Makefile
	./yaleparser httpresp.txt c

condparsercparser.h: condparser.txt yaleparser Makefile
	./yaleparser condparser.txt h

condparsercparser.c: yaleparser condparser.txt Makefile
	./yaleparser condparser.txt c

recursivecbcparser.h: recursivecb.txt yaleparser Makefile
	./yaleparser recursivecb.txt h

recursivecbcparser.c: recursivecb.txt yaleparser Makefile
	./yaleparser recursivecb.txt c

backtracktestcparser.h: backtracktest.txt yaleparser Makefile
	./yaleparser backtracktest.txt h

backtracktestcparser.c: backtracktest.txt yaleparser Makefile
	./yaleparser backtracktest.txt c

backtracktestcbcparser.h: backtracktestcb.txt yaleparser Makefile
	./yaleparser backtracktestcb.txt h

backtracktestcbcparser.c: backtracktestcb.txt yaleparser Makefile
	./yaleparser backtracktestcb.txt c

tokentheft1cparser.h: tokentheft1.txt yaleparser Makefile
	./yaleparser tokentheft1.txt h

tokentheft1cparser.c: tokentheft1.txt yaleparser Makefile
	./yaleparser tokentheft1.txt c

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

ssl1cparser.h: ssl1.txt yaleparser Makefile
	./yaleparser $< h

ssl1cparser.c: ssl1.txt yaleparser Makefile
	./yaleparser $< c

ssl2cparser.h: ssl2.txt yaleparser Makefile
	./yaleparser $< h

ssl2cparser.c: ssl2.txt yaleparser Makefile
	./yaleparser $< c

ssl3cparser.h: ssl3.txt yaleparser Makefile
	./yaleparser $< h

ssl3cparser.c: ssl3.txt yaleparser Makefile
	./yaleparser $< c

ssl4cparser.h: ssl4.txt yaleparser Makefile
	./yaleparser $< h

ssl4cparser.c: ssl4.txt yaleparser Makefile
	./yaleparser $< c

ssl5cparser.h: ssl5.txt yaleparser Makefile
	./yaleparser $< h

ssl5cparser.c: ssl5.txt yaleparser Makefile
	./yaleparser $< c

ssl6cparser.h: ssl6.txt yaleparser Makefile
	./yaleparser $< h

ssl6cparser.c: ssl6.txt yaleparser Makefile
	./yaleparser $< c

sslcmain: sslcmain.o ssl1cparser.o ssl2cparser.o ssl3cparser.o ssl4cparser.o ssl5cparser.o ssl6cparser.o
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^)

sslcmainprint: sslcmainprint.o ssl1cparser.o ssl2cparser.o ssl3cparser.o ssl4cparser.o ssl5cparser.o ssl6cparser.o
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^)

# ------ End SSL --------

yaletest: yaletest.o yale.lex.o yale.tab.o yyutils.o Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^)

yaletopy: yaletopy.o yale.lex.o yale.tab.o yyutils.o Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^)

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
	rm -f parser.pyc
	rm -f regex.pyc
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
	rm -f reprefixcparser.c
	rm -f reprefixcparser.h
	rm -f httprespcparser.c
	rm -f httprespcparser.h
	rm -f condparsercparser.c
	rm -f condparsercparser.h
	rm -f backtracktestcbcparser.c
	rm -f backtracktestcbcparser.h
	rm -f backtracktestcparser.c
	rm -f backtracktestcparser.h
	rm -f recursivecbcparser.c
	rm -f recursivecbcparser.h
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
	rm -f tokentheft1cparser.c
	rm -f tokentheft1cparser.h

distclean: clean
	rm -f yaletest
	rm -f yaletopy
	rm -f httpmain
	rm -f httpmainprint
	rm -f httpcmain
	rm -f httpcmainprint
	rm -f httprespcmain
	rm -f sslcmain
	rm -f recursivecbmain
	rm -f backtracktestmain
	rm -f backtracktestcbmain
	rm -f sslcmainprint
	rm -f lenprefixcmain
	rm -f reprefixcmain
	rm -f yaleparser
	rm -f regexmain
	rm -f condtest
	rm -f unit
	rm -f tokentheft1main
	rm -f parserunit
