YALE_YY_SRC_LIB := yyutils.c
YALE_YY_SRC := $(YALE_YY_SRC_LIB)

YALE_YY_LEX_LIB := yale.l
YALE_YY_LEX := $(YALE_YY_LEX_LIB)

YALE_YY_YACC_LIB := yale.y
YALE_YY_YACC := $(YALE_YY_YACC_LIB)

YALE_YY_LEX_LIB := $(patsubst %,$(DIRYALE_YY)/%,$(YALE_YY_LEX_LIB))
YALE_YY_LEX := $(patsubst %,$(DIRYALE_YY)/%,$(YALE_YY_LEX))

YALE_YY_YACC_LIB := $(patsubst %,$(DIRYALE_YY)/%,$(YALE_YY_YACC_LIB))
YALE_YY_YACC := $(patsubst %,$(DIRYALE_YY)/%,$(YALE_YY_YACC))

YALE_YY_LEXGEN_LIB := $(patsubst %.l,%.lex.c,$(YALE_YY_LEX_LIB))
YALE_YY_LEXGEN := $(patsubst %.l,%.lex.c,$(YALE_YY_LEX))

YALE_YY_YACCGEN_LIB := $(patsubst %.y,%.tab.c,$(YALE_YY_YACC_LIB))
YALE_YY_YACCGEN := $(patsubst %.y,%.tab.c,$(YALE_YY_YACC))

YALE_YY_GEN_LIB := $(patsubst %.l,%.lex.c,$(YALE_YY_LEX_LIB)) $(patsubst %.y,%.tab.c,$(YALE_YY_YACC_LIB))
YALE_YY_GEN := $(patsubst %.l,%.lex.c,$(YALE_YY_LEX)) $(patsubst %.y,%.tab.c,$(YALE_YY_YACC))

YALE_YY_SRC_LIB := $(patsubst %,$(DIRYALE_YY)/%,$(YALE_YY_SRC_LIB))
YALE_YY_SRC := $(patsubst %,$(DIRYALE_YY)/%,$(YALE_YY_SRC))

YALE_YY_OBJ_LIB := $(patsubst %.c,%.o,$(YALE_YY_SRC_LIB))
YALE_YY_OBJ := $(patsubst %.c,%.o,$(YALE_YY_SRC))

YALE_YY_OBJGEN_LIB := $(patsubst %.c,%.o,$(YALE_YY_GEN_LIB))
YALE_YY_OBJGEN := $(patsubst %.c,%.o,$(YALE_YY_GEN))

YALE_YY_ASM_LIB := $(patsubst %.c,%.s,$(YALE_YY_SRC_LIB))
YALE_YY_ASM := $(patsubst %.c,%.s,$(YALE_YY_SRC))

YALE_YY_ASMGEN_LIB := $(patsubst %.c,%.s,$(YALE_YY_GEN_LIB))
YALE_YY_ASMGEN := $(patsubst %.c,%.s,$(YALE_YY_GEN))

YALE_YY_DEP_LIB := $(patsubst %.c,%.d,$(YALE_YY_SRC_LIB))
YALE_YY_DEP := $(patsubst %.c,%.d,$(YALE_YY_SRC))

YALE_YY_DEPGEN_LIB := $(patsubst %.c,%.d,$(YALE_YY_GEN_LIB))
YALE_YY_DEPGEN := $(patsubst %.c,%.d,$(YALE_YY_GEN))

CFLAGS_YALE_YY := -I$(DIRYALE_CORE)

MAKEFILES_YALE_YY := $(DIRYALE_YY)/module.mk
LIBS_YALE_YY := $(DIRCORE)/libcore.a

.PHONY: YALE_YY clean_YALE_YY distclean_YALE_YY unit_YALE_YY $(LCYALE_YY) clean_$(LCYALE_YY) distclean_$(LCYALE_YY) unit_$(LCYALE_YY)

$(LCYALE_YY): YALE_YY
clean_$(LCYALE_YY): clean_YALE_YY
distclean_$(LCYALE_YY): distclean_YALE_YY
unit_$(LCYALE_YY): unit_YALE_YY

YALE_YY: $(DIRYALE_YY)/libyy.a

unit_YALE_YY: $(MAKEFILES_COMMON) $(MAKEFILES_YALE_YY)
	@true

$(DIRYALE_YY)/libyy.a: $(YALE_YY_OBJ_LIB) $(YALE_YY_OBJGEN_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_YY)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(YALE_YY_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_YALE_YY)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_YALE_YY)
	$(CC) $(CFLAGS) -c -S -o $*.s $*.c $(CFLAGS_YALE_YY)
$(YALE_YY_OBJGEN): %.o: %.c %.h %.d $(MAKEFILES_COMMON) $(MAKEFILES_YALE_YY)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_YALE_YY) -Wno-sign-compare -Wno-missing-prototypes
	$(CC) $(CFLAGS) -c -S -o $*.s $*.c $(CFLAGS_YALE_YY) -Wno-sign-compare -Wno-missing-prototypes

$(YALE_YY_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_YALE_YY)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_YALE_YY)
$(YALE_YY_DEPGEN): %.d: %.c %.h $(MAKEFILES_COMMON) $(MAKEFILES_YALE_YY)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_YALE_YY)

$(DIRYALE_YY)/yale.lex.d: $(DIRYALE_YY)/yale.tab.h $(DIRYALE_YY)/yale.lex.h
$(DIRYALE_YY)/yale.lex.o: $(DIRYALE_YY)/yale.tab.h $(DIRYALE_YY)/yale.lex.h
$(DIRYALE_YY)/yale.tab.d: $(DIRYALE_YY)/yale.tab.h $(DIRYALE_YY)/yale.lex.h
$(DIRYALE_YY)/yale.tab.o: $(DIRYALE_YY)/yale.tab.h $(DIRYALE_YY)/yale.lex.h

$(DIRYALE_YY)/YALE.LEX.INTERMEDIATE: $(DIRYALE_YY)/yale.l
	mkdir -p $(DIRYALE_YY)/intermediatestore
	flex --outfile=$(DIRYALE_YY)/intermediatestore/yale.lex.c --header-file=$(DIRYALE_YY)/intermediatestore/yale.lex.h $(DIRYALE_YY)/yale.l
	touch $(DIRYALE_YY)/YALE.LEX.INTERMEDIATE
$(DIRYALE_YY)/YALE.TAB.INTERMEDIATE: $(DIRYALE_YY)/yale.y
	mkdir -p $(DIRYALE_YY)/intermediatestore
	bison --defines=$(DIRYALE_YY)/intermediatestore/yale.tab.h --output=$(DIRYALE_YY)/intermediatestore/yale.tab.c $(DIRYALE_YY)/yale.y
	touch $(DIRYALE_YY)/YALE.TAB.INTERMEDIATE
$(DIRYALE_YY)/yale.lex.c: $(DIRYALE_YY)/YALE.LEX.INTERMEDIATE
	cp $(DIRYALE_YY)/intermediatestore/yale.lex.c $(DIRYALE_YY)
$(DIRYALE_YY)/yale.lex.h: $(DIRYALE_YY)/YALE.LEX.INTERMEDIATE
	cp $(DIRYALE_YY)/intermediatestore/yale.lex.h $(DIRYALE_YY)
$(DIRYALE_YY)/yale.tab.c: $(DIRYALE_YY)/YALE.TAB.INTERMEDIATE
	cp $(DIRYALE_YY)/intermediatestore/yale.tab.c $(DIRYALE_YY)
$(DIRYALE_YY)/yale.tab.h: $(DIRYALE_YY)/YALE.TAB.INTERMEDIATE
	cp $(DIRYALE_YY)/intermediatestore/yale.tab.h $(DIRYALE_YY)

clean_YALE_YY:
	rm -f $(YALE_YY_OBJ) $(YALE_YY_OBJGEN) $(YALE_YY_DEP) $(YALE_YY_DEPGEN)
	rm -f $(YALE_YY_ASM) $(YALE_YY_ASMGEN)
	rm -rf $(DIRYALE_YY)/intermediatestore
	rm -f $(DIRYALE_YY)/YALE.TAB.INTERMEDIATE
	rm -f $(DIRYALE_YY)/YALE.LEX.INTERMEDIATE
	rm -f $(DIRYALE_YY)/yale.lex.c
	rm -f $(DIRYALE_YY)/yale.lex.h
	rm -f $(DIRYALE_YY)/yale.tab.c
	rm -f $(DIRYALE_YY)/yale.tab.h

distclean_YALE_YY: clean_YALE_YY
	rm -f $(DIRYALE_YY)/libyy.a

-include $(DIRYALE_YY)/*.d
