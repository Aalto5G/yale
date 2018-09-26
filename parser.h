#ifndef _PARSER_H_
#define _PARSER_H_

#include "yale.h"
#include "regex.h"
#include <sys/uio.h>

struct dict {
  struct bitset bitset[256];
  struct bitset has;
};

struct REGenEntry {
  struct bitset key;
  struct dfa_node *dfas;
  uint8_t dfacnt;
};

struct REGen {
  struct REGenEntry entries[255];
};

struct LookupTblEntry {
  uint8_t val;
  uint8_t cb;
};

struct firstset_entry {
  uint8_t rhs[256]; // 0.25 KB
  uint8_t rhssz;
  struct dict dict; // 8 KB
};

struct stackconfig {
  uint8_t stack[255];
  uint8_t sz;
};

struct ParserGen {
  struct iovec re_by_idx[255];
  int priorities[255];
  uint8_t tokencnt;
  uint8_t nonterminalcnt;
  char *parsername;
  uint8_t start_state;
  uint8_t epsilon;
  char *state_include_str;
  int tokens_finalized;
  struct rule rules[255];
  uint8_t rulecnt;
  struct cb cbs[255];
  uint8_t cbcnt;
  struct REGen re_gen;
  struct LookupTblEntry T[255][255]; // val==255: invalid, cb==255: no callback
  struct dict Fo[256]; // 2 MB
  struct firstset_entry Fi[8192]; // 66 MB
  size_t Ficnt;
  uint8_t pick_those[255][255];
  struct pick_those_struct pick_thoses[255];
  uint8_t pick_thoses_cnt;
  uint8_t max_stack_size;
  uint8_t max_bt;
  struct nfa_node ns[255];
  struct dfa_node ds[255];
  struct transitionbufs bufs;
  struct stackconfig stackconfigs[32768]; // 8 MB
  size_t stackconfigcnt;
};

void parsergen_init(struct ParserGen *gen, char *parsername);

void gen_parser(struct ParserGen *gen);

void parsergen_state_include(struct ParserGen *gen, char *stateinclude);

void parsergen_set_start_state(struct ParserGen *gen, uint8_t start_state);

uint8_t parsergen_add_token(struct ParserGen *gen, char *re, size_t resz, int prio);

void parsergen_finalize_tokens(struct ParserGen *gen);

uint8_t parsergen_add_nonterminal(struct ParserGen *gen);

void parsergen_set_rules(struct ParserGen *gen, const struct rule *rules, uint8_t rulecnt, const struct namespaceitem *ns);

void parsergen_set_cb(struct ParserGen *gen, const struct cb *cbs, uint8_t cbcnt);

ssize_t max_stack_sz(struct ParserGen *gen);

void parsergen_dump_headers(struct ParserGen *gen, FILE *f);

#endif
