#ifndef _PARSER_H_
#define _PARSER_H_

#include "yale.h"
#include "regex.h"
#include <sys/uio.h>

struct dict {
  struct bitset bitset[YALE_UINT_MAX_LEGAL + 1];
  struct bitset has;
};

struct REGenEntry {
  struct bitset key;
  struct dfa_node *dfas;
  yale_uint_t dfacnt;
};

struct REGen {
  struct REGenEntry entries[YALE_UINT_MAX_LEGAL];
};

struct LookupTblEntry {
  yale_uint_t val;
  yale_uint_t cb;
};

struct firstset_entry {
  struct yale_hash_list_node node;
  yale_uint_t rhs[YALE_UINT_MAX_LEGAL]; // 0.25 KB
  yale_uint_t rhssz;
  struct dict dict; // 8 KB
};

struct stackconfig {
  struct yale_hash_list_node node;
  yale_uint_t *stack;
  yale_uint_t sz;
  size_t i;
};

struct ParserGen {
  yale_uint_t tokencnt;
  yale_uint_t nonterminalcnt;
  char *parsername;
  yale_uint_t start_state;
  yale_uint_t epsilon;
  char *state_include_str;
  size_t Ficnt;
  yale_uint_t pick_thoses_cnt;
  yale_uint_t max_stack_size;
  yale_uint_t max_bt;
  size_t stackconfigcnt;
  char *userareaptr;
  int tokens_finalized;
  yale_uint_t rulecnt;
  yale_uint_t cbcnt;
  struct yale_hash_table Fi_hash;
  struct yale_hash_table stackconfigs_hash;
  struct dict *Fo[YALE_UINT_MAX_LEGAL + 1]; // 2 kB
  struct REGen re_gen;
  yale_uint_t pick_thoses_id_by_nonterminal[YALE_UINT_MAX_LEGAL];
  struct pick_those_struct pick_thoses[YALE_UINT_MAX_LEGAL];
  struct iovec re_by_idx[YALE_UINT_MAX_LEGAL];
  int priorities[YALE_UINT_MAX_LEGAL];
  struct rule rules[YALE_UINT_MAX_LEGAL]; // 382 kB
  struct cb cbs[YALE_UINT_MAX_LEGAL];
  struct nfa_node ns[YALE_UINT_MAX_LEGAL]; // 2 MB
  struct dfa_node ds[YALE_UINT_MAX_LEGAL];
  struct LookupTblEntry T[YALE_UINT_MAX_LEGAL][YALE_UINT_MAX_LEGAL];
    // val==YALE_UINT_MAX_LEGAL: invalid
    // cb==YALE_UINT_MAX_LEGAL: no callback
  yale_uint_t pick_those[YALE_UINT_MAX_LEGAL][YALE_UINT_MAX_LEGAL]; // 64 kB
  struct firstset_entry *Fi[8192]; // 64 kB
  //struct stackconfig stackconfigs[32768]; // 1.25 MB
  struct stackconfig *stackconfigs[32768]; // 0.25 MB
  struct transitionbufs bufs; // 16 MB, this could be made to use dynamic alloc
  char userarea[64*1024*1024];
};

void parsergen_init(struct ParserGen *gen, char *parsername);

void parsergen_free(struct ParserGen *gen);

void gen_parser(struct ParserGen *gen);

void parsergen_state_include(struct ParserGen *gen, char *stateinclude);

void parsergen_set_start_state(struct ParserGen *gen, yale_uint_t start_state);

yale_uint_t parsergen_add_token(struct ParserGen *gen, char *re, size_t resz, int prio);

void parsergen_finalize_tokens(struct ParserGen *gen);

yale_uint_t parsergen_add_nonterminal(struct ParserGen *gen);

void parsergen_set_rules(struct ParserGen *gen, const struct rule *rules, yale_uint_t rulecnt, const struct namespaceitem *ns);

void parsergen_set_cb(struct ParserGen *gen, const struct cb *cbs, yale_uint_t cbcnt);

ssize_t max_stack_sz(struct ParserGen *gen);

void parsergen_dump_headers(struct ParserGen *gen, FILE *f);

void parsergen_dump_parser(struct ParserGen *gen, FILE *f);

#endif
