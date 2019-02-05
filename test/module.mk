YALE_TEST_SRC_LIB :=
YALE_TEST_SRC := $(YALE_TEST_SRC_LIB) parserunit.c condtest.c httpcmain.c httpcmainprint.c httpcpytest.c httppyperf.c lenperfixcmain.c reprefixcmain.c httptrespcmain.c recursivecbmain.c backtracktestmain.c backtracktestcbmain.c tokentheft1main.c tokentheft1smain.c lenprefixcmain.c httprespcmain.c sslcmain.c sslcmainprint.c

YALE_TEST_SRCGEN_LIB := lenprefixcparser.c reprefixcparser.c httpcparser.c httppycparser.c httprespcparser.c condparsercparser.c recursivecbcparser.c backtracktestcparser.c backtracktestcbcparser.c tokentheft1cparser.c tokentheft1scparser.c ssl1cparser.c ssl2cparser.c ssl3cparser.c ssl4cparser.c ssl5cparser.c ssl6cparser.c
YALE_TEST_SRCGEN := $(YALE_TEST_SRCGEN_LIB)

YALE_TEST_HDRGEN := lenprefixcparser.h reprefixcparser.h httpcparser.h httppycparser.h httprespcparser.h condparsercparser.h recursivecbcparser.h backtracktestcparser.h backtracktestcbcparser.h tokentheft1cparser.h tokentheft1scparser.h ssl1cparser.h ssl2cparser.h ssl3cparser.h ssl4cparser.h ssl5cparser.h ssl6cparser.h

YALE_TEST_HDRGEN := $(patsubst %,$(DIRYALE_TEST)/%,$(YALE_TEST_HDRGEN))

YALE_TEST_SRC_LIB := $(patsubst %,$(DIRYALE_TEST)/%,$(YALE_TEST_SRC_LIB))
YALE_TEST_SRC := $(patsubst %,$(DIRYALE_TEST)/%,$(YALE_TEST_SRC))

YALE_TEST_SRCGEN_LIB := $(patsubst %,$(DIRYALE_TEST)/%,$(YALE_TEST_SRCGEN_LIB))
YALE_TEST_SRCGEN := $(patsubst %,$(DIRYALE_TEST)/%,$(YALE_TEST_SRCGEN))

YALE_TEST_OBJ_LIB := $(patsubst %.c,%.o,$(YALE_TEST_SRC_LIB))
YALE_TEST_OBJ := $(patsubst %.c,%.o,$(YALE_TEST_SRC))

YALE_TEST_OBJGEN_LIB := $(patsubst %.c,%.o,$(YALE_TEST_SRCGEN_LIB))
YALE_TEST_OBJGEN := $(patsubst %.c,%.o,$(YALE_TEST_SRCGEN))

YALE_TEST_DEP_LIB := $(patsubst %.c,%.d,$(YALE_TEST_SRC_LIB))
YALE_TEST_DEP := $(patsubst %.c,%.d,$(YALE_TEST_SRC))

YALE_TEST_DEPGEN_LIB := $(patsubst %.c,%.d,$(YALE_TEST_SRCGEN_LIB))
YALE_TEST_DEPGEN := $(patsubst %.c,%.d,$(YALE_TEST_SRCGEN))

YALE_TEST_ASM_LIB := $(patsubst %.c,%.s,$(YALE_TEST_SRC_LIB))
YALE_TEST_ASM := $(patsubst %.c,%.s,$(YALE_TEST_SRC))

YALE_TEST_ASMGEN_LIB := $(patsubst %.c,%.s,$(YALE_TEST_SRCGEN_LIB))
YALE_TEST_ASMGEN := $(patsubst %.c,%.s,$(YALE_TEST_SRCGEN))

CFLAGS_YALE_TEST := -I$(DIRYALE_RUNTIME) -I$(DIRYALE_CORE)

MAKEFILES_YALE_TEST := $(DIRYALE_TEST)/module.mk
LIBS_YALE_TEST := $(DIRYALE_CORE)/libcore.a

.PHONY: YALE_TEST clean_YALE_TEST distclean_YALE_TEST unit_YALE_TEST $(LCYALE_TEST) clean_$(LCYALE_TEST) distclean_$(LCYALE_TEST) unit_$(LCYALE_TEST)

$(LCYALE_TEST): YALE_TEST
clean_$(LCYALE_TEST): clean_YALE_TEST
distclean_$(LCYALE_TEST): distclean_YALE_TEST
unit_$(LCYALE_TEST): unit_YALE_TEST

YALE_TEST_PROGS:=$(DIRYALE_TEST)/parserunit $(DIRYALE_TEST)/condtest $(DIRYALE_TEST)/httpcmain $(DIRYALE_TEST)/httpcmainprint $(DIRYALE_TEST)/httpcpytest $(DIRYALE_TEST)/httppyperf $(DIRYALE_TEST)/lenprefixcmain $(DIRYALE_TEST)/reprefixcmain $(DIRYALE_TEST)/httprespcmain $(DIRYALE_TEST)/recursivecbmain $(DIRYALE_TEST)/backtracktestmain $(DIRYALE_TEST)/backtracktestcbmain $(DIRYALE_TEST)/tokentheft1main $(DIRYALE_TEST)/tokentheft1smain $(DIRYALE_TEST)/sslcmain $(DIRYALE_TEST)/sslcmainprint

YALE_TEST: $(DIRYALE_TEST)/libtest.a $(YALE_TEST_PROGS)

unit_YALE_TEST: $(YALE_TEST_PROGS) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(DIRYALE_TEST)/parserunit

$(DIRYALE_TEST)/libtest.a: $(YALE_TEST_OBJ_LIB) $(YALE_TEST_OBJGEN_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

# Main programs

$(DIRYALE_TEST)/parserunit: $(DIRYALE_TEST)/parserunit.o $(DIRYALE_TEST)/libtest.a $(LIBS_YALE_TEST) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_YALE_TEST)

$(DIRYALE_TEST)/condtest: $(DIRYALE_TEST)/condtest.o $(DIRYALE_TEST)/libtest.a $(LIBS_YALE_TEST) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_YALE_TEST)

$(DIRYALE_TEST)/httpcmain: $(DIRYALE_TEST)/httpcmain.o $(DIRYALE_TEST)/libtest.a $(LIBS_YALE_TEST) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_YALE_TEST)

$(DIRYALE_TEST)/httpcmainprint: $(DIRYALE_TEST)/httpcmainprint.o $(DIRYALE_TEST)/libtest.a $(LIBS_YALE_TEST) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_YALE_TEST)

$(DIRYALE_TEST)/httpcpytest: $(DIRYALE_TEST)/httpcpytest.o $(DIRYALE_TEST)/libtest.a $(LIBS_YALE_TEST) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_YALE_TEST)

$(DIRYALE_TEST)/httppyperf: $(DIRYALE_TEST)/httppyperf.o $(DIRYALE_TEST)/libtest.a $(LIBS_YALE_TEST) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_YALE_TEST)

$(DIRYALE_TEST)/lenprefixcmain: $(DIRYALE_TEST)/lenprefixcmain.o $(DIRYALE_TEST)/libtest.a $(LIBS_YALE_TEST) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_YALE_TEST)

$(DIRYALE_TEST)/reprefixcmain: $(DIRYALE_TEST)/reprefixcmain.o $(DIRYALE_TEST)/libtest.a $(LIBS_YALE_TEST) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_YALE_TEST)

$(DIRYALE_TEST)/httprespcmain: $(DIRYALE_TEST)/httprespcmain.o $(DIRYALE_TEST)/libtest.a $(LIBS_YALE_TEST) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_YALE_TEST)

$(DIRYALE_TEST)/recursivecbmain: $(DIRYALE_TEST)/recursivecbmain.o $(DIRYALE_TEST)/libtest.a $(LIBS_YALE_TEST) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_YALE_TEST)

$(DIRYALE_TEST)/backtracktestmain: $(DIRYALE_TEST)/backtracktestmain.o $(DIRYALE_TEST)/libtest.a $(LIBS_YALE_TEST) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_YALE_TEST)

$(DIRYALE_TEST)/backtracktestcbmain: $(DIRYALE_TEST)/backtracktestcbmain.o $(DIRYALE_TEST)/libtest.a $(LIBS_YALE_TEST) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_YALE_TEST)

$(DIRYALE_TEST)/tokentheft1main: $(DIRYALE_TEST)/tokentheft1main.o $(DIRYALE_TEST)/libtest.a $(LIBS_YALE_TEST) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_YALE_TEST)

$(DIRYALE_TEST)/tokentheft1smain: $(DIRYALE_TEST)/tokentheft1smain.o $(DIRYALE_TEST)/libtest.a $(LIBS_YALE_TEST) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_YALE_TEST)

$(DIRYALE_TEST)/sslcmain: $(DIRYALE_TEST)/sslcmain.o $(DIRYALE_TEST)/libtest.a $(LIBS_YALE_TEST) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_YALE_TEST)

$(DIRYALE_TEST)/sslcmainprint: $(DIRYALE_TEST)/sslcmainprint.o $(DIRYALE_TEST)/libtest.a $(LIBS_YALE_TEST) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_YALE_TEST)

# Dependencies

#$(DIRYALE_TEST)/httpmain.d: $(DIRYALE_TEST)/httpparser.h
#$(DIRYALE_TEST)/httpmain.o: $(DIRYALE_TEST)/httpparser.h
#$(DIRYALE_TEST)/httpmainprint.d: $(DIRYALE_TEST)/httpparser.h
#$(DIRYALE_TEST)/httpmainprint.o: $(DIRYALE_TEST)/httpparser.h
#$(DIRYALE_TEST)/httpparser.d: $(DIRYALE_TEST)/httpparser.h
#$(DIRYALE_TEST)/httpparser.o: $(DIRYALE_TEST)/httpparser.h

$(DIRYALE_TEST)/httpcmain.d: $(DIRYALE_TEST)/httpcparser.h
$(DIRYALE_TEST)/httpcmain.o: $(DIRYALE_TEST)/httpcparser.h
$(DIRYALE_TEST)/httpcmainprint.d: $(DIRYALE_TEST)/httpcparser.h
$(DIRYALE_TEST)/httpcmainprint.o: $(DIRYALE_TEST)/httpcparser.h
$(DIRYALE_TEST)/httpcparser.d: $(DIRYALE_TEST)/httpcparser.h
$(DIRYALE_TEST)/httpcparser.o: $(DIRYALE_TEST)/httpcparser.h

$(DIRYALE_TEST)/httpcpytest.d: $(DIRYALE_TEST)/httppycparser.h
$(DIRYALE_TEST)/httpcpytest.o: $(DIRYALE_TEST)/httppycparser.h
$(DIRYALE_TEST)/httppyperf.d: $(DIRYALE_TEST)/httppycparser.h
$(DIRYALE_TEST)/httppyperf.o: $(DIRYALE_TEST)/httppycparser.h
$(DIRYALE_TEST)/httppycparser.d: $(DIRYALE_TEST)/httppycparser.h
$(DIRYALE_TEST)/httppycparser.o: $(DIRYALE_TEST)/httppycparser.h

$(DIRYALE_TEST)/recursivecbmain.d: $(DIRYALE_TEST)/recursivecbcparser.h
$(DIRYALE_TEST)/recursivecbmain.o: $(DIRYALE_TEST)/recursivecbcparser.h
$(DIRYALE_TEST)/recursivecbcparser.d: $(DIRYALE_TEST)/recursivecbcparser.h
$(DIRYALE_TEST)/recursivecbcparser.o: $(DIRYALE_TEST)/recursivecbcparser.h

$(DIRYALE_TEST)/httprespcmain.d: $(DIRYALE_TEST)/httprespcparser.h
$(DIRYALE_TEST)/httprespcmain.o: $(DIRYALE_TEST)/httprespcparser.h
$(DIRYALE_TEST)/httprespcparser.d: $(DIRYALE_TEST)/httprespcparser.h
$(DIRYALE_TEST)/httprespcparser.o: $(DIRYALE_TEST)/httprespcparser.h

$(DIRYALE_TEST)/backtracktestmain.d: $(DIRYALE_TEST)/backtracktestcparser.h
$(DIRYALE_TEST)/backtracktestmain.o: $(DIRYALE_TEST)/backtracktestcparser.h
$(DIRYALE_TEST)/backtracktestcparser.d: $(DIRYALE_TEST)/backtracktestcparser.h
$(DIRYALE_TEST)/backtracktestcparser.o: $(DIRYALE_TEST)/backtracktestcparser.h

$(DIRYALE_TEST)/backtracktestcbmain.d: $(DIRYALE_TEST)/backtracktestcbcparser.h
$(DIRYALE_TEST)/backtracktestcbmain.o: $(DIRYALE_TEST)/backtracktestcbcparser.h
$(DIRYALE_TEST)/backtracktestcbcparser.d: $(DIRYALE_TEST)/backtracktestcbcparser.h
$(DIRYALE_TEST)/backtracktestcbcparser.o: $(DIRYALE_TEST)/backtracktestcbcparser.h

$(DIRYALE_TEST)/lenprefixcmain.d: $(DIRYALE_TEST)/lenprefixcparser.h
$(DIRYALE_TEST)/lenprefixcmain.o: $(DIRYALE_TEST)/lenprefixcparser.h
$(DIRYALE_TEST)/lenprefixcparser.d: $(DIRYALE_TEST)/lenprefixcparser.h
$(DIRYALE_TEST)/lenprefixcparser.o: $(DIRYALE_TEST)/lenprefixcparser.h

$(DIRYALE_TEST)/reprefixcmain.d: $(DIRYALE_TEST)/reprefixcparser.h
$(DIRYALE_TEST)/reprefixcmain.o: $(DIRYALE_TEST)/reprefixcparser.h
$(DIRYALE_TEST)/reprefixcparser.d: $(DIRYALE_TEST)/reprefixcparser.h
$(DIRYALE_TEST)/reprefixcparser.o: $(DIRYALE_TEST)/reprefixcparser.h

$(DIRYALE_TEST)/tokentheft1main.d: $(DIRYALE_TEST)/tokentheft1cparser.h
$(DIRYALE_TEST)/tokentheft1main.o: $(DIRYALE_TEST)/tokentheft1cparser.h
$(DIRYALE_TEST)/tokentheft1cparser.d: $(DIRYALE_TEST)/tokentheft1cparser.h
$(DIRYALE_TEST)/tokentheft1cparser.o: $(DIRYALE_TEST)/tokentheft1cparser.h

$(DIRYALE_TEST)/tokentheft1smain.d: $(DIRYALE_TEST)/tokentheft1scparser.h
$(DIRYALE_TEST)/tokentheft1smain.o: $(DIRYALE_TEST)/tokentheft1scparser.h
$(DIRYALE_TEST)/tokentheft1scparser.d: $(DIRYALE_TEST)/tokentheft1scparser.h
$(DIRYALE_TEST)/tokentheft1scparser.o: $(DIRYALE_TEST)/tokentheft1scparser.h

$(DIRYALE_TEST)/condtest.d: $(DIRYALE_TEST)/condparsercparser.h
$(DIRYALE_TEST)/condtest.o: $(DIRYALE_TEST)/condparsercparser.h
$(DIRYALE_TEST)/condparsercparser.d: $(DIRYALE_TEST)/condparsercparser.h
$(DIRYALE_TEST)/condparsercparser.o: $(DIRYALE_TEST)/condparsercparser.h

# Generated parsers

YALE_TEST_PARSER_DEPS := $(DIRYALE_MAIN)/yaleparser $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)

$(DIRYALE_TEST)/lenprefixcparser.h: $(DIRYALE_TEST)/lenprefix.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< h

$(DIRYALE_TEST)/lenprefixcparser.c: $(DIRYALE_TEST)/lenprefix.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< c

$(DIRYALE_TEST)/reprefixcparser.h: $(DIRYALE_TEST)/reprefix.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< h

$(DIRYALE_TEST)/reprefixcparser.c: $(DIRYALE_TEST)/reprefix.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< c

$(DIRYALE_TEST)/httpcparser.h: $(DIRYALE_TEST)/httppaper.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< h

$(DIRYALE_TEST)/httpcparser.c: $(DIRYALE_TEST)/httppaper.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< c

$(DIRYALE_TEST)/httppycparser.h: $(DIRYALE_TEST)/httppy.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< h

$(DIRYALE_TEST)/httppycparser.c: $(DIRYALE_TEST)/httppy.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< c

$(DIRYALE_TEST)/httprespcparser.h: $(DIRYALE_TEST)/httpresp.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< h

$(DIRYALE_TEST)/httprespcparser.c: $(DIRYALE_TEST)/httpresp.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< c

$(DIRYALE_TEST)/condparsercparser.h: $(DIRYALE_TEST)/condparser.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< h

$(DIRYALE_TEST)/condparsercparser.c: $(DIRYALE_TEST)/condparser.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< c

$(DIRYALE_TEST)/recursivecbcparser.h: $(DIRYALE_TEST)/recursivecb.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< h

$(DIRYALE_TEST)/recursivecbcparser.c: $(DIRYALE_TEST)/recursivecb.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< c

$(DIRYALE_TEST)/backtracktestcparser.h: $(DIRYALE_TEST)/backtracktest.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< h

$(DIRYALE_TEST)/backtracktestcparser.c: $(DIRYALE_TEST)/backtracktest.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< c

$(DIRYALE_TEST)/backtracktestcbcparser.h: $(DIRYALE_TEST)/backtracktestcb.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< h

$(DIRYALE_TEST)/backtracktestcbcparser.c: $(DIRYALE_TEST)/backtracktestcb.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< c

$(DIRYALE_TEST)/tokentheft1cparser.h: $(DIRYALE_TEST)/tokentheft1.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< h

$(DIRYALE_TEST)/tokentheft1cparser.c: $(DIRYALE_TEST)/tokentheft1.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< c

$(DIRYALE_TEST)/tokentheft1scparser.h: $(DIRYALE_TEST)/tokentheft1s.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< h

$(DIRYALE_TEST)/tokentheft1scparser.c: $(DIRYALE_TEST)/tokentheft1s.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< c

# ------ Begin SSL dependencies --------

$(DIRYALE_TEST)/sslcmain.d: $(DIRYALE_TEST)/ssl1cparser.h $(DIRYALE_TEST)/ssl2cparser.h $(DIRYALE_TEST)/ssl3cparser.h $(DIRYALE_TEST)/ssl4cparser.h $(DIRYALE_TEST)/ssl5cparser.h $(DIRYALE_TEST)/ssl6cparser.h
$(DIRYALE_TEST)/sslcmain.o: $(DIRYALE_TEST)/ssl1cparser.h $(DIRYALE_TEST)/ssl2cparser.h $(DIRYALE_TEST)/ssl3cparser.h $(DIRYALE_TEST)/ssl4cparser.h $(DIRYALE_TEST)/ssl5cparser.h $(DIRYALE_TEST)/ssl6cparser.h
$(DIRYALE_TEST)/sslcmainprint.d: $(DIRYALE_TEST)/ssl1cparser.h $(DIRYALE_TEST)/ssl2cparser.h $(DIRYALE_TEST)/ssl3cparser.h $(DIRYALE_TEST)/ssl4cparser.h $(DIRYALE_TEST)/ssl5cparser.h $(DIRYALE_TEST)/ssl6cparser.h
$(DIRYALE_TEST)/sslcmainprint.o: $(DIRYALE_TEST)/ssl1cparser.h $(DIRYALE_TEST)/ssl2cparser.h $(DIRYALE_TEST)/ssl3cparser.h $(DIRYALE_TEST)/ssl4cparser.h $(DIRYALE_TEST)/ssl5cparser.h $(DIRYALE_TEST)/ssl6cparser.h
$(DIRYALE_TEST)/ssl1cparser.d: $(DIRYALE_TEST)/ssl1cparser.h $(DIRYALE_TEST)/ssl2cparser.h $(DIRYALE_TEST)/ssl3cparser.h $(DIRYALE_TEST)/ssl4cparser.h $(DIRYALE_TEST)/ssl5cparser.h $(DIRYALE_TEST)/ssl6cparser.h
$(DIRYALE_TEST)/ssl1cparser.o: $(DIRYALE_TEST)/ssl1cparser.h $(DIRYALE_TEST)/ssl2cparser.h $(DIRYALE_TEST)/ssl3cparser.h $(DIRYALE_TEST)/ssl4cparser.h $(DIRYALE_TEST)/ssl5cparser.h $(DIRYALE_TEST)/ssl6cparser.h
$(DIRYALE_TEST)/ssl2cparser.d: $(DIRYALE_TEST)/ssl2cparser.h $(DIRYALE_TEST)/ssl3cparser.h $(DIRYALE_TEST)/ssl4cparser.h $(DIRYALE_TEST)/ssl5cparser.h $(DIRYALE_TEST)/ssl6cparser.h
$(DIRYALE_TEST)/ssl2cparser.o: $(DIRYALE_TEST)/ssl2cparser.h $(DIRYALE_TEST)/ssl3cparser.h $(DIRYALE_TEST)/ssl4cparser.h $(DIRYALE_TEST)/ssl5cparser.h $(DIRYALE_TEST)/ssl6cparser.h
$(DIRYALE_TEST)/ssl3cparser.d: $(DIRYALE_TEST)/ssl3cparser.h $(DIRYALE_TEST)/ssl4cparser.h $(DIRYALE_TEST)/ssl5cparser.h $(DIRYALE_TEST)/ssl6cparser.h
$(DIRYALE_TEST)/ssl3cparser.o: $(DIRYALE_TEST)/ssl3cparser.h $(DIRYALE_TEST)/ssl4cparser.h $(DIRYALE_TEST)/ssl5cparser.h $(DIRYALE_TEST)/ssl6cparser.h
$(DIRYALE_TEST)/ssl4cparser.d: $(DIRYALE_TEST)/ssl4cparser.h $(DIRYALE_TEST)/ssl5cparser.h $(DIRYALE_TEST)/ssl6cparser.h
$(DIRYALE_TEST)/ssl4cparser.o: $(DIRYALE_TEST)/ssl4cparser.h $(DIRYALE_TEST)/ssl5cparser.h $(DIRYALE_TEST)/ssl6cparser.h
$(DIRYALE_TEST)/ssl5cparser.d: $(DIRYALE_TEST)/ssl5cparser.h $(DIRYALE_TEST)/ssl6cparser.h
$(DIRYALE_TEST)/ssl5cparser.o: $(DIRYALE_TEST)/ssl5cparser.h $(DIRYALE_TEST)/ssl6cparser.h
$(DIRYALE_TEST)/ssl6cparser.d: $(DIRYALE_TEST)/ssl6cparser.h
$(DIRYALE_TEST)/ssl6cparser.o: $(DIRYALE_TEST)/ssl6cparser.h

# ------ Begin SSL generated parsers --------

$(DIRYALE_TEST)/ssl1cparser.h: $(DIRYALE_TEST)/ssl1.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< h

$(DIRYALE_TEST)/ssl1cparser.c: $(DIRYALE_TEST)/ssl1.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< c

$(DIRYALE_TEST)/ssl2cparser.h: $(DIRYALE_TEST)/ssl2.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< h

$(DIRYALE_TEST)/ssl2cparser.c: $(DIRYALE_TEST)/ssl2.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< c

$(DIRYALE_TEST)/ssl3cparser.h: $(DIRYALE_TEST)/ssl3.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< h

$(DIRYALE_TEST)/ssl3cparser.c: $(DIRYALE_TEST)/ssl3.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< c

$(DIRYALE_TEST)/ssl4cparser.h: $(DIRYALE_TEST)/ssl4.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< h

$(DIRYALE_TEST)/ssl4cparser.c: $(DIRYALE_TEST)/ssl4.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< c

$(DIRYALE_TEST)/ssl5cparser.h: $(DIRYALE_TEST)/ssl5.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< h

$(DIRYALE_TEST)/ssl5cparser.c: $(DIRYALE_TEST)/ssl5.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< c

$(DIRYALE_TEST)/ssl6cparser.h: $(DIRYALE_TEST)/ssl6.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< h

$(DIRYALE_TEST)/ssl6cparser.c: $(DIRYALE_TEST)/ssl6.txt $(YALE_TEST_PARSER_DEPS)
	./$(DIRYALE_MAIN)/yaleparser $< c

# Automatic rules

$(YALE_TEST_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_YALE_TEST)
	$(CC) $(CFLAGS) -c -S -o $*.s $*.c $(CFLAGS_YALE_TEST)

$(YALE_TEST_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_YALE_TEST)

$(YALE_TEST_OBJGEN): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_YALE_TEST)
	$(CC) $(CFLAGS) -c -S -o $*.s $*.c $(CFLAGS_YALE_TEST)

$(YALE_TEST_DEPGEN): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_YALE_TEST)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_YALE_TEST)

clean_YALE_TEST:
	rm -f $(YALE_TEST_OBJ) $(YALE_TEST_DEP) $(YALE_TEST_OBJGEN) $(YALE_TEST_DEPGEN)
	rm -f $(YALE_TEST_SRCGEN) $(YALE_TEST_HDRGEN)
	rm -f $(YALE_TEST_ASM) $(YALE_TEST_ASMGEN)

distclean_YALE_TEST: clean_YALE_TEST
	rm -f $(DIRYALE_TEST)/libtest.a $(YALE_TEST_PROGS)

-include $(DIRYALE_TEST)/*.d
