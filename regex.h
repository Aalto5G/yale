#ifndef _REGEX_H_
#define _REGEX_H_

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <ctype.h>
#include "bitset.h"
#include "yalehashtable.h"

struct re;

#if 0
struct wildcard {
};

struct emptystr {
};
#endif

struct literals {
  struct charbitset bitmask;
};

struct concat {
  struct re *re1;
  struct re *re2;
};

struct altern {
  struct re *re1;
  struct re *re2;
};

struct alternmulti {
  struct re **res;
  yale_uint_t *pick_those;
  size_t resz;
};

struct star {
  struct re *re;
};

struct re_parse_result {
  size_t branchsz;
};

enum re_type {
  WILDCARD,
  EMPTYSTR,
  LITERALS,
  CONCAT,
  ALTERN,
  ALTERNMULTI,
  STAR
};

struct re {
  enum re_type type;
  union {
#if 0
    struct wildcard wc;
    struct emptystr e;
#endif
    struct literals lit;
    struct concat cat;
    struct altern alt;
    struct alternmulti altmulti;
    struct star star;
  } u;
};

struct re *dup_re(struct re *re);

struct dfa_node {
  yale_uint_t d[256];
  yale_uint_t default_tr;
  yale_uint_t acceptid;
  uint8_t tainted:1;
  uint8_t accepting:1;
  uint8_t final:1;
  struct bitset acceptidset;
  struct bitset taintidset;
  uint64_t algo_tmp;
  size_t transitions_id;
};

struct nfa_node {
  struct bitset d[256];
  struct bitset defaults;
  struct bitset epsilon;
  uint8_t accepting:1;
  yale_uint_t taintid;
};

struct numbers_set {
  struct yale_hash_list_node node;
  struct bitset numbers;
};

struct numbers_sets {
  struct yale_hash_table hash;
};

void numbers_sets_init(struct numbers_sets *hash, void *(*alloc)(void*,size_t), void *allocud);

int numbers_sets_put(struct numbers_sets *hash, const struct bitset *numbers, void *(*alloc)(void*,size_t), void *allocud);

void numbers_sets_emit(FILE *f, struct numbers_sets *hash, const struct bitset *numbers, void *(*alloc)(void*,size_t), void *allocud);

void nfa_init(struct nfa_node *n, int accepting, int taintid);

void nfa_connect(struct nfa_node *n, char ch, yale_uint_t node2);

void nfa_connect_epsilon(struct nfa_node *n, yale_uint_t node2);

void nfa_connect_default(struct nfa_node *n, yale_uint_t node2);

void epsilonclosure(struct nfa_node *ns, struct bitset nodes,
                    struct bitset *closurep, int *tainted,
                    struct bitset *acceptidsetp,
                    struct bitset *taintidsetp);

void dfa_init(struct dfa_node *n, int accepting, int tainted, struct bitset *acceptidset, struct bitset *taintidset);

void dfa_init_empty(struct dfa_node *n);

void dfa_connect(struct dfa_node *n, char ch, yale_uint_t node2);

void dfa_connect_default(struct dfa_node *n, yale_uint_t node2);

void
check_recurse_acceptid_is(struct dfa_node *ds, yale_uint_t state, yale_uint_t acceptid);

void
check_recurse_acceptid_is_not(struct dfa_node *ds, yale_uint_t state, yale_uint_t acceptid);

void check_cb_first(struct dfa_node *ds, yale_uint_t acceptid, yale_uint_t state);

void check_cb(struct dfa_node *ds, yale_uint_t state, yale_uint_t acceptid);

struct bitset_hash_item {
  struct bitset key;
  yale_uint_t dfanodeid;
};

struct bitset_hash {
  struct bitset_hash_item tbl[YALE_UINT_MAX_LEGAL];
  yale_uint_t tblsz;
};

// FIXME this algorithm requires thorough review
ssize_t state_backtrack(struct dfa_node *ds, yale_uint_t state, size_t bound);

void __attribute__((noinline)) set_accepting(struct dfa_node *ds, yale_uint_t state, int *priorities);

ssize_t maximal_backtrack(struct dfa_node *ds, yale_uint_t state, size_t bound);

void dfaviz(struct dfa_node *ds, yale_uint_t cnt);

void nfaviz(struct nfa_node *ns, yale_uint_t cnt);

yale_uint_t nfa2dfa(struct nfa_node *ns, struct dfa_node *ds, yale_uint_t begin);

struct re *parse_re(const char *re, size_t resz, size_t *remainderstart);

struct re *
parse_bracketexpr(const char *re, size_t resz, size_t *remainderstart);

struct re *parse_atom(const char *re, size_t resz, size_t *remainderstart);

struct re *parse_piece(const char *re, size_t resz, size_t *remainderstart);

// branch: piece branch
struct re *parse_branch(const char *re, size_t resz, size_t *remainderstart);

// RE: branch | RE
struct re *parse_re(const char *re, size_t resz, size_t *remainderstart);

struct re *parse_res(struct iovec *regexps, yale_uint_t *pick_those, size_t resz);

void gennfa(struct re *regexp,
            struct nfa_node *ns, yale_uint_t *ncnt,
            yale_uint_t begin, yale_uint_t end,
            yale_uint_t taintid);

void gennfa_main(struct re *regexp,
                 struct nfa_node *ns, yale_uint_t *ncnt,
                 yale_uint_t taintid);

void gennfa_alternmulti(struct re *regexp,
                        struct nfa_node *ns, yale_uint_t *ncnt);

struct pick_those_struct {
  yale_uint_t *pick_those;
  size_t len;
  struct dfa_node *ds;
  size_t dscnt;
};

struct transitionbuf {
  struct yale_hash_list_node node;
  yale_uint_t transitions[256];
  size_t id;
};

#define MAX_TRANS 65536 // 256 automatons, 256 states per automaton

struct transitionbufs {
  struct yale_hash_table hash;
  struct transitionbuf *all[MAX_TRANS];
  size_t cnt;
};

uint32_t transition_hash_fn(struct yale_hash_list_node *node, void *ud);

static inline void transitionbufs_init(struct transitionbufs *bufs,
                                       void *(*alloc_fn)(void*, size_t), void *alloc_ud)
{
  bufs->cnt = 0;
  yale_hash_table_init(&bufs->hash, 65536, transition_hash_fn, NULL, alloc_fn, alloc_ud);
}

static inline void transitionbufs_fini(struct transitionbufs *bufs)
{
  struct yale_hash_list_node *n, *x;
  unsigned bucket;
  YALE_HASH_TABLE_FOR_EACH_SAFE(&bufs->hash, bucket, n, x)
  {
    yale_hash_table_delete(&bufs->hash, n);
  }
  yale_hash_table_free(&bufs->hash);
}

size_t
get_transid(const yale_uint_t *transitions, struct transitionbufs *bufs,
            void *(*alloc)(void*, size_t), void *alloc_ud);

void
perf_trans(yale_uint_t *transitions, struct transitionbufs *bufs,
           void *(*alloc)(void*, size_t), void *alloc_ud);

void
pick(struct nfa_node *nsglobal, struct dfa_node *dsglobal,
     struct iovec *res, struct pick_those_struct *pick_those, int *priorities);

void
collect(struct pick_those_struct *pick_thoses, size_t cnt,
        struct transitionbufs *bufs,
        void *(*alloc)(void*, size_t), void *alloc_ud);

void dump_headers(FILE *f, const char *parsername, size_t max_bt);

void
dump_collected(FILE *f, const char *parsername, struct transitionbufs *bufs);

void
dump_one(FILE *f, const char *parsername, struct pick_those_struct *pick_those,
         struct numbers_sets *numbershash,
         void *(*alloc)(void*,size_t), void *allocud);

void
dump_chead(FILE *f, const char *parsername, int nofastpath);

#endif
