#ifndef _YALECOMMON2_H_
#define _YALECOMMON2_H_

#include "yalecommoncommon.h"

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint8_t yale_lexer_uint8_t;
typedef uint8_t yale_parser_uint8_t;
#define YALE_LEXER_UINT8_MAX UINT8_MAX
#define YALE_PARSER_UINT8_MAX UINT8_MAX
typedef uint16_t yale_lexer_uint16_t;
typedef uint16_t yale_parser_uint16_t;
#define YALE_LEXER_UINT16_MAX UINT16_MAX
#define YALE_PARSER_UINT16_MAX UINT16_MAX

#ifndef YALE_CONTAINER_OF
#define YALE_CONTAINER_OF(ptr, type, member) \
  ((type*)(((char*)ptr) - offsetof(type, member)))
#endif

#undef yale_likely
#define yale_likely(x)       __builtin_expect((x),1)
#undef yale_unlikely
#define yale_unlikely(x)     __builtin_expect((x),0)

#ifndef YALE_SMALL_CODE
#define YALE_SMALL_CODE 1
#endif

struct yale_state_p8l8 {
  uint8_t accepting;
  uint8_t finalflag;
  yale_parser_uint8_t acceptid;
  const yale_parser_uint8_t *taintids;
  yale_parser_uint8_t taintidsz;
  uint64_t fastpathbitmask[4];
#if YALE_SMALL_CODE
  const yale_lexer_uint8_t *transitions;
#else
  yale_lexer_uint8_t transitions[256];
#endif
};
struct yale_state_p8l16 {
  uint8_t accepting;
  uint8_t finalflag;
  yale_parser_uint8_t acceptid;
  const yale_parser_uint8_t *taintids;
  yale_parser_uint8_t taintidsz;
  uint64_t fastpathbitmask[4];
#if YALE_SMALL_CODE
  const yale_lexer_uint16_t *transitions;
#else
  yale_lexer_uint16_t transitions[256];
#endif
};
struct yale_state_p16l8 {
  uint8_t accepting;
  uint8_t finalflag;
  yale_parser_uint16_t acceptid;
  const yale_parser_uint16_t *taintids;
  yale_parser_uint16_t taintidsz;
  uint64_t fastpathbitmask[4];
#if YALE_SMALL_CODE
  const yale_lexer_uint8_t *transitions;
#else
  yale_lexer_uint8_t transitions[256];
#endif
};
struct yale_state_p16l16 {
  uint8_t accepting;
  uint8_t finalflag;
  yale_parser_uint16_t acceptid;
  const yale_parser_uint16_t *taintids;
  yale_parser_uint16_t taintidsz;
  uint64_t fastpathbitmask[4];
#if YALE_SMALL_CODE
  const yale_lexer_uint16_t *transitions;
#else
  yale_lexer_uint16_t transitions[256];
#endif
};

struct yale_ruleentry8 {
  yale_parser_uint8_t rhs;
  yale_parser_uint8_t cb;
};
struct yale_ruleentry16 {
  yale_parser_uint16_t rhs;
  yale_parser_uint16_t cb;
};

struct yale_rule8 {
  yale_parser_uint8_t lhs;
  yale_parser_uint8_t rhssz;
  const struct yale_ruleentry8 *rhs;
};
struct yale_rule16 {
  yale_parser_uint16_t lhs;
  yale_parser_uint16_t rhssz;
  const struct yale_ruleentry16 *rhs;
};

struct yale_reentry_p8l8 {
  const struct yale_state_p8l8 *re;
};
struct yale_reentry_p8l16 {
  const struct yale_state_p8l16 *re;
};
struct yale_reentry_p16l8 {
  const struct yale_state_p16l8 *re;
};
struct yale_reentry_p16l16 {
  const struct yale_state_p16l16 *re;
};

#endif
