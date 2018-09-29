#ifndef _YALECOMMON_H_
#define _YALECOMMON_H_

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef CONTAINER_OF
#define CONTAINER_OF(ptr, type, member) \
  ((type*)(((char*)ptr) - (((char*)&(((type*)0)->member)) - ((char*)0))))
#endif

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#undef SMALL_CODE

struct state {
  uint8_t accepting;
  uint8_t final;
  uint8_t acceptid;
  uint64_t fastpathbitmask[4];
#ifdef SMALL_CODE
  const uint8_t *transitions;
#else
  uint8_t transitions[256];
#endif
};

struct ruleentry {
  uint8_t rhs;
  uint8_t cb;
};

struct rule {
  uint8_t lhs;
  uint8_t rhssz;
  const struct ruleentry *rhs;
};

struct reentry {
  const struct state *re;
};

#endif
