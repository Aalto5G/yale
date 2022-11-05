CC := clang

.SUFFIXES:

DIRYALE_YY := yy
LCYALE_YY := yale_yy
MODULES += YALE_YY

DIRYALE_CORE := core
LCYALE_CORE := yale_core
MODULES += YALE_CORE

DIRYALE_MAIN := main
LCYALE_MAIN := yale_main
MODULES += YALE_MAIN

DIRYALE_RUNTIME := runtime
LCYALE_RUNTIME := yale_runtime
MODULES += YALE_RUNTIME

DIRYALE_TEST := test
LCYALE_TEST := yale_test
MODULES += YALE_TEST

DIRYALE_PYBRIDGE := pybridge
LCYALE_PYBRIDGE := yale_pybridge
MODULES += YALE_PYBRIDGE

DIRYALE_CBRIDGE := cbridge
LCYALE_CBRIDGE := yale_cbridge
MODULES += YALE_CBRIDGE

.PHONY: all clean distclean unit

all: $(MODULES)
clean: $(patsubst %,clean_%,$(MODULES))
distclean: $(patsubst %,distclean_%,$(MODULES))
unit: $(patsubst %,unit_%,$(MODULES))

MAKEFILES_COMMON := Makefile opts.mk

include opts.mk

WITH_PYTHON ?= no
WITH_WERROR ?= no
WITH_NEW_CPU ?= yes
PYTHON_DIR ?= /usr/include/python3.6

CFLAGS ?= -Ofast -g -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -Wno-tautological-compare -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith -fomit-frame-pointer -std=gnu11

ifeq ($(WITH_WERROR),yes)
CFLAGS := $(CFLAGS) -Werror
endif

ifeq ($(WITH_NEW_CPU),yes)
  CFLAGS += -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -msse4 -mavx -mavx2 -msse4a -mbmi -mbmi2 -march=skylake
endif

ifeq ($(WITH_PYTHON),yes)
  CFLAGS += -I$(PYTHON_DIR) -fPIC
endif

$(foreach module,$(MODULES),$(eval \
    include $(DIR$(module))/module.mk))
$(foreach module,$(INCLUDES),$(eval \
    include $(DIR$(module))/module.mk))

opts.mk:
	touch opts.mk
