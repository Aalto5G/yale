YALE_MAIN_SRC_LIB :=
YALE_MAIN_SRC := $(YALE_MAIN_SRC_LIB) yaleparser.c yaletopy.c

YALE_MAIN_SRC_LIB := $(patsubst %,$(DIRYALE_MAIN)/%,$(YALE_MAIN_SRC_LIB))
YALE_MAIN_SRC := $(patsubst %,$(DIRYALE_MAIN)/%,$(YALE_MAIN_SRC))

YALE_MAIN_OBJ_LIB := $(patsubst %.c,%.o,$(YALE_MAIN_SRC_LIB))
YALE_MAIN_OBJ := $(patsubst %.c,%.o,$(YALE_MAIN_SRC))

YALE_MAIN_DEP_LIB := $(patsubst %.c,%.d,$(YALE_MAIN_SRC_LIB))
YALE_MAIN_DEP := $(patsubst %.c,%.d,$(YALE_MAIN_SRC))

YALE_MAIN_ASM_LIB := $(patsubst %.c,%.s,$(YALE_MAIN_SRC_LIB))
YALE_MAIN_ASM := $(patsubst %.c,%.s,$(YALE_MAIN_SRC))

CFLAGS_YALE_MAIN := -I$(DIRYALE_CORE) -I$(DIRYALE_YY)

MAKEFILES_YALE_MAIN := $(DIRYALE_MAIN)/module.mk
LIBS_YALE_MAIN := $(DIRYALE_YY)/libyy.a $(DIRYALE_CORE)/libcore.a

.PHONY: YALE_MAIN clean_YALE_MAIN distclean_YALE_MAIN unit_YALE_MAIN $(LCYALE_MAIN) clean_$(LCYALE_MAIN) distclean_$(LCYALE_MAIN) unit_$(LCYALE_MAIN)

$(LCYALE_MAIN): YALE_MAIN
clean_$(LCYALE_MAIN): clean_YALE_MAIN
distclean_$(LCYALE_MAIN): distclean_YALE_MAIN
unit_$(LCYALE_MAIN): unit_YALE_MAIN

YALE_MAIN: $(DIRYALE_MAIN)/libmain.a $(DIRYALE_MAIN)/yaleparser $(DIRYALE_MAIN)/yaletopy

unit_YALE_MAIN: $(MAKEFILES_COMMON) $(MAKEFILES_YALE_MAIN)
	@true

$(DIRYALE_MAIN)/libmain.a: $(YALE_MAIN_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_MAIN)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRYALE_MAIN)/yaleparser: $(DIRYALE_MAIN)/yaleparser.o $(DIRYALE_MAIN)/libmain.a $(LIBS_YALE_MAIN) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_MAIN)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_YALE_MAIN)

$(DIRYALE_MAIN)/yaletopy: $(DIRYALE_MAIN)/yaletopy.o $(DIRYALE_MAIN)/libmain.a $(LIBS_YALE_MAIN) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_MAIN)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_YALE_MAIN)

$(YALE_MAIN_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_YALE_MAIN)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_YALE_MAIN)
	$(CC) $(CFLAGS) -c -S -o $*.s $*.c $(CFLAGS_YALE_MAIN)

$(YALE_MAIN_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_YALE_MAIN)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_YALE_MAIN)

clean_YALE_MAIN:
	rm -f $(YALE_MAIN_OBJ) $(YALE_MAIN_DEP) $(YALE_MAIN_ASM)

distclean_YALE_MAIN: clean_YALE_MAIN
	rm -f $(DIRYALE_MAIN)/libmain.a $(DIRYALE_MAIN)/yaletopy $(DIRYALE_MAIN)/yaleparser

-include $(DIRYALE_MAIN)/*.d
