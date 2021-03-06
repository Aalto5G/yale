YALE_CBRIDGE_SRC_LIB := sslc.c httpc.c
YALE_CBRIDGE_SRC := $(YALE_CBRIDGE_SRC_LIB) test.c

YALE_CBRIDGE_SRC_LIB := $(patsubst %,$(DIRYALE_CBRIDGE)/%,$(YALE_CBRIDGE_SRC_LIB))
YALE_CBRIDGE_SRC := $(patsubst %,$(DIRYALE_CBRIDGE)/%,$(YALE_CBRIDGE_SRC))

YALE_CBRIDGE_OBJ_LIB := $(patsubst %.c,%.o,$(YALE_CBRIDGE_SRC_LIB))
YALE_CBRIDGE_OBJ := $(patsubst %.c,%.o,$(YALE_CBRIDGE_SRC))

YALE_CBRIDGE_DEP_LIB := $(patsubst %.c,%.d,$(YALE_CBRIDGE_SRC_LIB))
YALE_CBRIDGE_DEP := $(patsubst %.c,%.d,$(YALE_CBRIDGE_SRC))

YALE_CBRIDGE_ASM_LIB := $(patsubst %.c,%.s,$(YALE_CBRIDGE_SRC_LIB))
YALE_CBRIDGE_ASM := $(patsubst %.c,%.s,$(YALE_CBRIDGE_SRC))

CFLAGS_YALE_CBRIDGE := -I$(DIRYALE_TEST) -I$(DIRYALE_RUNTIME)

MAKEFILES_YALE_CBRIDGE := $(DIRYALE_CBRIDGE)/module.mk
LIBS_YALE_CBRIDGE := $(DIRYALE_TEST)/libtest.a

.PHONY: YALE_CBRIDGE clean_YALE_CBRIDGE distclean_YALE_CBRIDGE unit_YALE_CBRIDGE $(LCYALE_CBRIDGE) clean_$(LCYALE_CBRIDGE) distclean_$(LCYALE_CBRIDGE) unit_$(LCYALE_CBRIDGE)

$(LCYALE_CBRIDGE): YALE_CBRIDGE
clean_$(LCYALE_CBRIDGE): clean_YALE_CBRIDGE
distclean_$(LCYALE_CBRIDGE): distclean_YALE_CBRIDGE
unit_$(LCYALE_CBRIDGE): unit_YALE_CBRIDGE

YALE_CBRIDGE: $(DIRYALE_CBRIDGE)/libcbridge.a $(DIRYALE_CBRIDGE)/test

unit_YALE_CBRIDGE: $(MAKEFILES_COMMON) $(MAKEFILES_YALE_CBRIDGE)
	@true

$(DIRYALE_CBRIDGE)/libcbridge.a: $(YALE_CBRIDGE_OBJ_LIB) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_CBRIDGE)
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(DIRYALE_CBRIDGE)/test: $(DIRYALE_CBRIDGE)/test.o $(DIRYALE_CBRIDGE)/libcbridge.a $(LIBS_YALE_CBRIDGE) $(MAKEFILES_COMMON) $(MAKEFILES_YALE_CBRIDGE)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(CFLAGS_YALE_CBRIDGE)

$(DIRYALE_CBRIDGE)/httpc.d: $(DIRYALE_TEST)/httppycparser.h
$(DIRYALE_CBRIDGE)/httpc.o: $(DIRYALE_TEST)/httppycparser.h

$(DIRYALE_CBRIDGE)/sslc.d: $(DIRYALE_TEST)/ssl1cparser.h $(DIRYALE_TEST)/ssl2cparser.h $(DIRYALE_TEST)/ssl3cparser.h $(DIRYALE_TEST)/ssl4cparser.h $(DIRYALE_TEST)/ssl5cparser.h $(DIRYALE_TEST)/ssl6cparser.h
$(DIRYALE_CBRIDGE)/sslc.o: $(DIRYALE_TEST)/ssl1cparser.h $(DIRYALE_TEST)/ssl2cparser.h $(DIRYALE_TEST)/ssl3cparser.h $(DIRYALE_TEST)/ssl4cparser.h $(DIRYALE_TEST)/ssl5cparser.h $(DIRYALE_TEST)/ssl6cparser.h

$(YALE_CBRIDGE_OBJ): %.o: %.c %.d $(MAKEFILES_COMMON) $(MAKEFILES_YALE_CBRIDGE)
	$(CC) $(CFLAGS) -c -o $*.o $*.c $(CFLAGS_YALE_CBRIDGE)
	$(CC) $(CFLAGS) -c -S -o $*.s $*.c $(CFLAGS_YALE_CBRIDGE)

$(YALE_CBRIDGE_DEP): %.d: %.c $(MAKEFILES_COMMON) $(MAKEFILES_YALE_CBRIDGE)
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c $(CFLAGS_YALE_CBRIDGE)

clean_YALE_CBRIDGE:
	rm -f $(YALE_CBRIDGE_OBJ) $(YALE_CBRIDGE_DEP) $(YALE_CBRIDGE_ASM)

distclean_YALE_CBRIDGE: clean_YALE_CBRIDGE
	rm -f $(DIRYALE_CBRIDGE)/libcbridge.a $(DIRYALE_CBRIDGE)/test

-include $(DIRYALE_CBRIDGE)/*.d
