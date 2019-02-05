YALE_CORE_SRC_LIB := parser.c regex.c
YALE_CORE_SRC := $(YALE_CORE_SRC_LIB) unit.c

YALE_CORE_SRC_LIB := $(patsubst %,$(DIRYALE_CORE)/%,$(YALE_CORE_SRC_LIB))
YALE_CORE_SRC := $(patsubst %,$(DIRYALE_CORE)/%,$(YALE_CORE_SRC))

YALE_CORE_OBJ_LIB := $(patsubst %.c,%.o,$(YALE_CORE_SRC_LIB))
YALE_CORE_OBJ := $(patsubst %.c,%.o,$(YALE_CORE_SRC))

YALE_CORE_DEP_LIB := $(patsubst %.c,%.d,$(YALE_CORE_SRC_LIB))
YALE_CORE_DEP := $(patsubst %.c,%.d,$(YALE_CORE_SRC))

YALE_CORE_ASM_LIB := $(patsubst %.c,%.s,$(YALE_CORE_SRC_LIB))
YALE_CORE_ASM := $(patsubst %.c,%.s,$(YALE_CORE_SRC))

CFLAGS_YALE_CORE := 

MAKEFILES_YALE_CORE := $(DIRYALE_CORE)/module.mk
LIBS_YALE_CORE :=

.PHONY: YALE_CORE clean_YALE_CORE distclean_YALE_CORE unit_YALE_CORE $(LCYALE_CORE) clean_$(LCYALE_CORE) distclean_$(LCYALE_CORE) unit_$(LCYALE_CORE)

$(LCYALE_CORE): YALE_CORE
clean_$(LCYALE_CORE): clean_YALE_CORE
distclean_$(LCYALE_CORE): distclean_YALE_CORE
unit_$(LCYALE_CORE): unit_YALE_CORE

YALE_CORE: $(DIRYALE_CORE)/libcore.a

unit_YALE_CORE: $(MAKEFILES_COMMON) $(MAKEFILES_YALE_CORE)
	@true

$(DIRYALE_CORE)/libcore.a: $(YALE_CORE_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_CORE)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRYALE_CORE)/unit: $(DIRYALE_CORE)/unit.o $(DIRYALE_CORE)/libcore.a $(MAKEFILES_COMMON) $(MAKEFILES_YALE_CORE)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_YALE_CORE)

$(YALE_CORE_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_YALE_CORE)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_YALE_CORE)
	$(CC) $(CFLAGS) -c -S -o $*.s $*.c $(CFLAGS_YALE_CORE)

$(YALE_CORE_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_YALE_CORE)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_YALE_CORE)

clean_YALE_CORE:
	rm -f $(YALE_CORE_OBJ) $(YALE_CORE_DEP) $(YALE_CORE_ASM)

distclean_YALE_CORE: clean_YALE_CORE
	rm -f $(DIRYALE_CORE)/libcore.a

-include $(DIRYALE_CORE)/*.d
