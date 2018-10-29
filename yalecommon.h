#ifndef _YALECOMMON_H_
#define _YALECOMMON_H_

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

enum yale_flags {
  YALE_FLAG_START = (1<<0),
  YALE_FLAG_END = (1<<1),
  YALE_FLAG_ACTION = (1<<2),
  YALE_FLAG_MAJOR_MISTAKE = (1<<3),
};

typedef uint8_t lexer_uint_t;
typedef uint8_t parser_uint_t;
#define LEXER_UINT_MAX UINT8_MAX
#define PARSER_UINT_MAX UINT8_MAX

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
  parser_uint_t acceptid;
  const parser_uint_t *taintids;
  parser_uint_t taintidsz;
  uint64_t fastpathbitmask[4];
#ifdef SMALL_CODE
  const lexer_uint_t *transitions;
#else
  lexer_uint_t transitions[256];
#endif
};

struct callbacks {
  const parser_uint_t *cbs;
  parser_uint_t cbsz;
};

struct ruleentry {
  parser_uint_t rhs;
  parser_uint_t cb;
};

struct rule {
  parser_uint_t lhs;
  parser_uint_t rhssz;
  const struct ruleentry *rhs;
};

struct reentry {
  const struct state *re;
};

#endif
