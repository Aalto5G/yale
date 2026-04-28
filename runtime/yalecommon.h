#ifndef _YALECOMMON_H_
#define _YALECOMMON_H_

#include "yalecommoncommon.h"

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint8_t lexer_uint8_t;
typedef uint8_t parser_uint8_t;
#define LEXER_UINT8_MAX UINT8_MAX
#define PARSER_UINT8_MAX UINT8_MAX
typedef uint16_t lexer_uint16_t;
typedef uint16_t parser_uint16_t;
#define LEXER_UINT16_MAX UINT16_MAX
#define PARSER_UINT16_MAX UINT16_MAX
typedef uint8_t lexer_uint_t;
typedef uint8_t parser_uint_t;
#define LEXER_UINT_MAX UINT8_MAX
#define PARSER_UINT_MAX UINT8_MAX

#ifndef CONTAINER_OF
#define CONTAINER_OF(ptr, type, member) \
  ((type*)(((char*)ptr) - offsetof(type, member)))
#endif

#undef yale_likely
#undef yale_unlikely

#ifdef __has_builtin
  #if __has_builtin (__builtin_expect)
    #define yale_likely(x)       __builtin_expect(!!(x),1)
    #define yale_unlikely(x)     __builtin_expect(!!(x),0)
  #else
    #define yale_likely(x)       (!!(x))
    #define yale_unlikely(x)     (!!(x))
  #endif
#else
  #ifdef __clang__ // LLVM 3.0 is the first to use this, no-op on LLVM 1.0
    #define yale_likely(x)       __builtin_expect(!!(x),1)
    #define yale_unlikely(x)     __builtin_expect(!!(x),0)
  #else
    #ifdef __GNUC__
      #if __GNUC__ >= 3
        #define yale_likely(x)       __builtin_expect(!!(x),1)
        #define yale_unlikely(x)     __builtin_expect(!!(x),0)
      #else
        #define yale_likely(x)       (!!(x))
        #define yale_unlikely(x)     (!!(x))
      #endif
    #else
      #define yale_likely(x)       (!!(x))
      #define yale_unlikely(x)     (!!(x))
    #endif
  #endif
#endif

#define SMALL_CODE

struct state {
  uint8_t accepting;
  uint8_t finalflag;
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
struct state_p8l8 {
  uint8_t accepting;
  uint8_t finalflag;
  parser_uint8_t acceptid;
  const parser_uint8_t *taintids;
  parser_uint8_t taintidsz;
  uint64_t fastpathbitmask[4];
#ifdef SMALL_CODE
  const lexer_uint8_t *transitions;
#else
  lexer_uint8_t transitions[256];
#endif
};
struct state_p8l16 {
  uint8_t accepting;
  uint8_t finalflag;
  parser_uint8_t acceptid;
  const parser_uint8_t *taintids;
  parser_uint8_t taintidsz;
  uint64_t fastpathbitmask[4];
#ifdef SMALL_CODE
  const lexer_uint16_t *transitions;
#else
  lexer_uint16_t transitions[256];
#endif
};
struct state_p16l8 {
  uint8_t accepting;
  uint8_t finalflag;
  parser_uint16_t acceptid;
  const parser_uint16_t *taintids;
  parser_uint16_t taintidsz;
  uint64_t fastpathbitmask[4];
#ifdef SMALL_CODE
  const lexer_uint8_t *transitions;
#else
  lexer_uint8_t transitions[256];
#endif
};
struct state_p16l16 {
  uint8_t accepting;
  uint8_t finalflag;
  parser_uint16_t acceptid;
  const parser_uint16_t *taintids;
  parser_uint16_t taintidsz;
  uint64_t fastpathbitmask[4];
#ifdef SMALL_CODE
  const lexer_uint16_t *transitions;
#else
  lexer_uint16_t transitions[256];
#endif
};

struct callbacks {
  uint64_t cbsmask;
#if 0
  const parser_uint_t *cbs;
  parser_uint_t cbsz;
#endif
};

struct ruleentry {
  parser_uint_t rhs;
  parser_uint_t cb;
};
struct ruleentry8 {
  parser_uint8_t rhs;
  parser_uint8_t cb;
};
struct ruleentry16 {
  parser_uint16_t rhs;
  parser_uint16_t cb;
};

struct rule {
  parser_uint_t lhs;
  parser_uint_t rhssz;
  const struct ruleentry *rhs;
};
struct rule8 {
  parser_uint8_t lhs;
  parser_uint8_t rhssz;
  const struct ruleentry8 *rhs;
};
struct rule16 {
  parser_uint16_t lhs;
  parser_uint16_t rhssz;
  const struct ruleentry16 *rhs;
};

struct reentry {
  const struct state *re;
};
struct reentry_p8l8 {
  const struct state_p8l8 *re;
};
struct reentry_p8l16 {
  const struct state_p8l16 *re;
};
struct reentry_p16l8 {
  const struct state_p16l8 *re;
};
struct reentry_p16l16 {
  const struct state_p16l16 *re;
};

#endif
