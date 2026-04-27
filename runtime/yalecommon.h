#ifndef _YALECOMMON_H_
#define _YALECOMMON_H_

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>

enum yale_special_flags {
  YALE_SPECIAL_FLAG_BYTES = (1<<0),
  YALE_SPECIAL_FLAG_SHORTCUT = (1<<1),
};

enum yale_flags {
  YALE_FLAG_START = (1<<0),
  YALE_FLAG_END = (1<<1),
  YALE_FLAG_ACTION = (1<<2),
  YALE_FLAG_MAJOR_MISTAKE = (1<<3),
};

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

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

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

#if _POSIX_VERSION >= 202405L
  #undef YALE_HAS_FFSLL
  #define YALE_HAS_FFSLL 1
#endif

#ifdef __GLIBC__
  #if __GLIBC__  > 2 || (__GLIBC__  == 2 && __GLIBC_MINOR__  >= 27)
    #undef YALE_HAS_FFSLL
    #define YALE_HAS_FFSLL 1
  #endif
  #ifdef _GNU_SOURCE // this is ancient but required _GNU_SOURCE before 2.27
    #undef YALE_HAS_FFSLL
    #define YALE_HAS_FFSLL 1
  #endif
#endif

#ifdef __FreeBSD__
  #if __FreeBSD_version >= 800000
    #undef YALE_HAS_FFSLL
    #define YALE_HAS_FFSLL 1
  #endif
#endif

#ifdef __has_builtin
  #if __has_builtin (__builtin_ffsll)
    #undef YALE_HAS_BFFSLL
    #define YALE_HAS_BFFSLL 1
  #endif
#endif

static inline int yale_ffsu64(uint64_t x)
{
#ifdef YALE_HAS_BFFSLL
  return __builtin_ffsll((long long)x);
#else
#ifdef YALE_HAS_FFSLL
  return ffsll((long long)x);
#else
  int res = ffs((int)(x&0xFFFFFFFFU));
  if (res)
  {
    return res;
  }
  res = ffs((int)(x>>32));
  if (res)
  {
    return res+32;
  }
  return 0;
#endif
#endif
}

#endif
