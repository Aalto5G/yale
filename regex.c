#define _GNU_SOURCE
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <ctype.h>
#include "regex.h"
#include "yalemurmur.h"
#include "yalecontainerof.h"

static inline struct re *alloc_re(void)
{
  struct re *result = malloc(sizeof(*result));
  return result;
}

struct re *dup_re(struct re *re)
{
  struct re *result = alloc_re();
  *result = *re;
  switch (re->type)
  {
    case WILDCARD:
    case EMPTYSTR:
      break;
    case LITERALS:
      break;
    case STAR:
      result->u.star.re = dup_re(re->u.star.re);
      break;
    case ALTERNMULTI:
      abort();
      break;
    case ALTERN:
      result->u.alt.re1 = dup_re(re->u.alt.re1);
      result->u.alt.re2 = dup_re(re->u.alt.re2);
      break;
    case CONCAT:
      result->u.cat.re1 = dup_re(re->u.cat.re1);
      result->u.cat.re2 = dup_re(re->u.cat.re2);
      break;
    default:
      abort();
  }
  return result;
}

static inline void free_re(struct re *re)
{
  size_t i;
  //printf("re->type %d\n", re->type);
  switch (re->type)
  {
    case WILDCARD:
    case EMPTYSTR:
      break;
    case LITERALS:
#if 0
  printf("[");
  for (i = 0; i < 256; i++)
  {
    yale_uint_t wordoff = i/64;
    yale_uint_t bitoff = i%64;
    if (re->u.lit.bitmask.bitset[wordoff] & 1ULL<<bitoff)
    {
      printf("%c", (char)(unsigned char)i);
    }
  }
  printf("]\n");
#endif
      break;
    case STAR:
      free_re(re->u.star.re);
      re->u.star.re = NULL;
      break;
    case ALTERNMULTI:
      for (i = 0; i < re->u.altmulti.resz; i++)
      {
        free_re(re->u.altmulti.res[i]);
        re->u.altmulti.res[i] = NULL;
      }
      free(re->u.altmulti.res);
      re->u.altmulti.res = NULL;
      free(re->u.altmulti.pick_those);
      re->u.altmulti.pick_those = NULL;
      break;
    case ALTERN:
      free_re(re->u.alt.re1);
      re->u.alt.re1 = NULL;
      free_re(re->u.alt.re2);
      re->u.alt.re2 = NULL;
      break;
    case CONCAT:
      free_re(re->u.cat.re1);
      re->u.cat.re1 = NULL;
      free_re(re->u.cat.re2);
      re->u.cat.re2 = NULL;
      break;
    default:
      abort();
  }
  free(re);
}

void nfa_init(struct nfa_node *n, int accepting, int taintid)
{
  memset(n, 0, sizeof(*n));
  n->accepting = !!accepting;
  n->taintid = taintid;
}

void nfa_connect(struct nfa_node *n, char ch, yale_uint_t node2)
{
  yale_uint_t wordoff = node2/64;
  yale_uint_t bitoff = node2%64;
  n->d[(unsigned char)ch].bitset[wordoff] |= (1ULL<<bitoff);
}

void nfa_connect_epsilon(struct nfa_node *n, yale_uint_t node2)
{
  yale_uint_t wordoff = node2/64;
  yale_uint_t bitoff = node2%64;
  n->epsilon.bitset[wordoff] |= (1ULL<<bitoff);
}

void nfa_connect_default(struct nfa_node *n, yale_uint_t node2)
{
  yale_uint_t wordoff = node2/64;
  yale_uint_t bitoff = node2%64;
  n->defaults.bitset[wordoff] |= (1ULL<<bitoff);
}

void epsilonclosure(struct nfa_node *ns, struct bitset nodes,
                    struct bitset *closurep, int *tainted,
                    struct bitset *acceptidsetp,
                    struct bitset *taintidsetp)
{
  struct bitset closure = nodes;
  struct bitset taintidset = {};
  struct bitset acceptidset = {};
  yale_uint_t stack[YALE_UINT_MAX_LEGAL + 1];
  yale_uint_t nidx;
  size_t stacksz = 0;
  size_t i;
  size_t taintcnt = 0;
  for (i = 0; i < YALE_UINT_MAX_LEGAL + 1; /*i++*/)
  {
    yale_uint_t wordoff = i/64;
    yale_uint_t bitoff = i%64;
    if (nodes.bitset[wordoff] & (1ULL<<bitoff))
    {
      if (stacksz >= sizeof(stack)/sizeof(*stack))
      {
        abort();
      }
      stack[stacksz++] = i;
    }
    if (bitoff != 63)
    {
      i = (wordoff*64) + myffsll(nodes.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
    }
    else
    {
      i++;
    }
  }
  if (stacksz > 0)
  {
    //printf("Epsilon closure stack size: %d\n", (int)stacksz);
  }
  while (stacksz > 0)
  {
    struct nfa_node *n;
    nidx = stack[--stacksz];
    n = &ns[nidx];
    for (i = 0; i < YALE_UINT_MAX_LEGAL + 1; /*i++*/)
    {
      yale_uint_t wordoff = i/64;
      yale_uint_t bitoff = i%64;
      if (n->epsilon.bitset[wordoff] & (1ULL<<bitoff))
      {
        if (!(closure.bitset[wordoff] & (1ULL<<bitoff)))
        {
          closure.bitset[wordoff] |= (1ULL<<bitoff);
          if (stacksz >= sizeof(stack)/sizeof(*stack))
          {
            abort();
          }
          stack[stacksz++] = i;
        }
      }
      if (bitoff != 63)
      {
        i = (wordoff*64) + myffsll(n->epsilon.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
      }
      else
      {
        i++;
      }
    }
  }
  for (i = 0; i < YALE_UINT_MAX_LEGAL + 1; /*i++*/)
  {
    yale_uint_t wordoff = i/64;
    yale_uint_t bitoff = i%64;
    if (closure.bitset[wordoff] & (1ULL<<bitoff))
    {
      struct nfa_node *n = &ns[i];
      if (n->taintid != YALE_UINT_MAX_LEGAL)
      {
        yale_uint_t wordoff2 = n->taintid/64;
        yale_uint_t bitoff2 = n->taintid%64;
        if (!(taintidset.bitset[wordoff2] & (1ULL<<bitoff2)))
        {
          taintidset.bitset[wordoff2] |= (1ULL<<bitoff2);
          taintcnt++;
        }
        if (n->accepting)
        {
          acceptidset.bitset[wordoff2] |= (1ULL<<bitoff2);
        }
      }
    }
    if (bitoff != 63)
    {
      i = (wordoff*64) + myffsll(closure.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
    }
    else
    {
      i++;
    }
  }
  *closurep = closure;
  *acceptidsetp = acceptidset;
  *taintidsetp = taintidset;
  *tainted = (taintcnt > 1);
}

void dfa_init(struct dfa_node *n, int accepting, int tainted, struct bitset *acceptidset, struct bitset *taintidset)
{
#if 0
#else
  size_t i;
#endif
  memset(n, 0, sizeof(*n));
#if 0
  memset(n->d, 0xff, 256*sizeof(*n->d)); // Doesn't work if YALE_UINT_MAX_LEGAL not all ones
#else
  for (i = 0; i < 256; i++)
  {
    n->d[i] = YALE_UINT_MAX_LEGAL;
  }
#endif
  n->default_tr = YALE_UINT_MAX_LEGAL;
  n->acceptid = YALE_UINT_MAX_LEGAL;
  if (accepting && bitset_empty(acceptidset))
  {
    printf("Accepting yet acceptidset empty\n");
    abort();
  }
  if (accepting && bitset_empty(taintidset))
  {
    printf("Accepting yet taintidset empty\n");
    abort();
  }
  n->accepting = !!accepting;
  n->tainted = !!tainted;
  n->acceptidset = *acceptidset;
  n->taintidset = *taintidset;
  n->transitions_id = SIZE_MAX;
}

void dfa_init_empty(struct dfa_node *n)
{
  struct bitset acceptidset = {};
  struct bitset taintidset = {};
  dfa_init(n, 0, 0, &acceptidset, &taintidset);
}

void dfa_connect(struct dfa_node *n, char ch, yale_uint_t node2)
{
  if (n->d[(unsigned char)ch] != YALE_UINT_MAX_LEGAL || node2 == YALE_UINT_MAX_LEGAL)
  {
    printf("node2 %u\n", (unsigned)node2);
    printf("already connected %u\n", (unsigned)n->d[(unsigned char)ch]);
    abort();
  }
  n->d[(unsigned char)ch] = node2;
}

void dfa_connect_default(struct dfa_node *n, yale_uint_t node2)
{
  printf("connecting default\n");
  if (n->default_tr != YALE_UINT_MAX_LEGAL || node2 == YALE_UINT_MAX_LEGAL)
  {
    printf("default conflict\n");
    printf("node2 %u\n", (unsigned)node2);
    printf("already connected %u\n", (unsigned)n->default_tr);
    abort();
  }
  n->default_tr = node2;
}

void
check_recurse_acceptid_is(struct dfa_node *ds, yale_uint_t state, yale_uint_t acceptid)
{
  struct bitset tovisit = {};
  struct bitset visited = {};
  size_t i;
  set_bitset(&tovisit, state);
  while (!bitset_empty(&tovisit))
  {
    yale_uint_t queued = pick_rm_first(&tovisit);
    struct dfa_node *node = &ds[queued];
    if (has_bitset(&visited, queued))
    {
      continue;
    }
    set_bitset(&visited, queued);
    if (!ds[queued].accepting || ds[queued].acceptid != acceptid)
    {
      abort(); // FIXME error handling
    }
    for (i = 0; i < 256; i++)
    {
      if (node->d[i] != YALE_UINT_MAX_LEGAL)
      {
        if (!has_bitset(&visited, node->d[i]))
        {
          set_bitset(&tovisit, node->d[i]);
        }
      }
    }
    if (node->default_tr != YALE_UINT_MAX_LEGAL)
    {
      if (!has_bitset(&visited, node->default_tr))
      {
        set_bitset(&tovisit, node->default_tr);
      }
    }
  }
}

void
check_recurse_acceptid_is_not(struct dfa_node *ds, yale_uint_t state, yale_uint_t acceptid)
{
  struct bitset tovisit = {};
  struct bitset visited = {};
  size_t i;
  set_bitset(&tovisit, state);
  while (!bitset_empty(&tovisit))
  {
    yale_uint_t queued = pick_rm_first(&tovisit);
    struct dfa_node *node = &ds[queued];
    if (has_bitset(&visited, queued))
    {
      continue;
    }
    set_bitset(&visited, queued);
    if (ds[queued].accepting && ds[queued].acceptid == acceptid)
    {
      printf("Recurse acceptid is not fail, acceptid=%d\n", acceptid);
      abort(); // FIXME error handling
    }
    for (i = 0; i < 256; i++)
    {
      if (node->d[i] != YALE_UINT_MAX_LEGAL)
      {
        if (!has_bitset(&visited, node->d[i]))
        {
          set_bitset(&tovisit, node->d[i]);
        }
      }
    }
    if (node->default_tr != YALE_UINT_MAX_LEGAL)
    {
      if (!has_bitset(&visited, node->default_tr))
      {
        set_bitset(&tovisit, node->default_tr);
      }
    }
  }
}

void check_cb_first(struct dfa_node *ds, yale_uint_t acceptid, yale_uint_t state)
{
  if (ds[state].accepting && ds[state].acceptid == acceptid)
  {
    check_recurse_acceptid_is(ds, state, acceptid);
  }
  else
  {
    check_recurse_acceptid_is_not(ds, state, acceptid);
  }
}

void check_cb(struct dfa_node *ds, yale_uint_t state, yale_uint_t acceptid)
{
  size_t i;
  for (i = 0; i < 256; i++)
  {
    if (ds[state].d[i] != YALE_UINT_MAX_LEGAL)
    {
      check_cb_first(ds, acceptid, ds[state].d[i]);
    }
  }
  if (ds[state].default_tr != YALE_UINT_MAX_LEGAL)
  {
    check_cb_first(ds, acceptid, ds[state].default_tr);
  }
}

// FIXME this algorithm requires thorough review
ssize_t state_backtrack(struct dfa_node *ds, yale_uint_t state, size_t bound)
{
  struct bitset tovisit = {};
  struct bitset visited = {};
  size_t max_backtrack = 0;
  size_t i;
  ds[state].algo_tmp = 0;
  set_bitset(&tovisit, state);
  while (!bitset_empty(&tovisit))
  {
    yale_uint_t queued = pick_rm_first(&tovisit);
    struct dfa_node *node = &ds[queued];
    //printf("queued %d\n", (int)queued);
    if (node->algo_tmp > bound)
    {
      return -1;
    }
    if (node->algo_tmp > max_backtrack)
    {
      max_backtrack = node->algo_tmp;
    }
    set_bitset(&visited, queued);
    for (i = 0; i < 256; i++)
    {
      if (node->d[i] != YALE_UINT_MAX_LEGAL)
      {
        if (ds[node->d[i]].accepting)
        {
          continue;
        }
        if (!has_bitset(&visited, node->d[i]) ||
            ds[node->d[i]].algo_tmp < node->algo_tmp + 1)
        {
          ds[node->d[i]].algo_tmp = node->algo_tmp + 1;
          set_bitset(&tovisit, node->d[i]);
        }
      }
    }
    if (node->default_tr != YALE_UINT_MAX_LEGAL)
    {
      if (!has_bitset(&visited, node->default_tr) ||
          ds[node->default_tr].algo_tmp < node->algo_tmp + 1)
      {
        ds[node->default_tr].algo_tmp = node->algo_tmp + 1;
        set_bitset(&tovisit, node->default_tr);
      }
    }
  }
  return max_backtrack;
}

void __attribute__((noinline)) set_accepting(struct dfa_node *ds, yale_uint_t state, int *priorities)
{
  struct bitset tovisit = {};
  struct bitset visited = {};
  size_t i;
  int seen = 0;
  int priority;
  size_t count_thisprio = 0;
  yale_uint_t acceptid = 0;
  yale_uint_t wordoff, bitoff;
  set_bitset(&tovisit, state);
  while (!bitset_empty(&tovisit))
  {
    yale_uint_t queued = pick_rm_first(&tovisit);
    //printf("QUEUED %d\n", (int)queued);
    if (has_bitset(&visited, queued))
    {
      continue;
    }
    set_bitset(&visited, queued);
    if (ds[queued].accepting)
    {
      seen = 0;
      //printf("Setting seen to 0\n");
      for (i = 0; i < YALE_UINT_MAX_LEGAL + 1; /*i++*/)
      {
        wordoff = i/64;
        bitoff = i%64;
        //printf("? %d\n", (int)i);
        if (ds[queued].acceptidset.bitset[wordoff] & (1ULL<<bitoff))
        {
          //printf("! %d\n", (int)i);
          if (!seen)
          {
            seen = 1;
            priority = priorities[i];
            count_thisprio = 1;
            acceptid = i;
          }
          else if (priorities[i] == priority)
          {
            count_thisprio++;
          }
          else if (priorities[i] > priority)
          {
            priority = priorities[i];
            count_thisprio = 1;
            acceptid = i;
          }
        }
        if (bitoff != 63)
        {
          i = (wordoff*64) + myffsll(ds[queued].acceptidset.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
        }
        else
        {
          i++;
        }
      }
      if (!seen)
      {
        abort(); // Shouldn't happen
      }
      if (count_thisprio > 1)
      {
        abort(); // FIXME better error handling
      }
      if (count_thisprio == 0)
      {
        abort(); // Shouldn't happen
      }
      ds[queued].acceptid = acceptid;
    }
    for (i = 0; i < 256; i++)
    {
      if (ds[queued].d[i] != YALE_UINT_MAX_LEGAL)
      {
        if (!has_bitset(&visited, ds[queued].d[i]))
        {
          set_bitset(&tovisit, ds[queued].d[i]);
        }
      }
    }
    if (ds[queued].default_tr != YALE_UINT_MAX_LEGAL)
    {
      if (!has_bitset(&visited, ds[queued].default_tr))
      {
        set_bitset(&tovisit, ds[queued].default_tr);
      }
    }
  }
}

ssize_t maximal_backtrack(struct dfa_node *ds, yale_uint_t state, size_t bound)
{
  struct bitset tovisit = {};
  struct bitset visited = {};
  ssize_t state_bt;
  ssize_t max_backtrack = 0;
  size_t i;
  set_bitset(&tovisit, state);
  while (!bitset_empty(&tovisit))
  {
    yale_uint_t queued = pick_rm_first(&tovisit);
    //printf("QUEUED %d\n", (int)queued);
    if (has_bitset(&visited, queued))
    {
      continue;
    }
    set_bitset(&visited, queued);
    if (ds[queued].accepting)
    {
      state_bt = state_backtrack(ds, queued, bound);
      if (state_bt < 0)
      {
        return -1;
      }
      if (state_bt > max_backtrack)
      {
        max_backtrack = state_bt;
      }
    }
    for (i = 0; i < 256; i++)
    {
      if (ds[queued].d[i] != YALE_UINT_MAX_LEGAL)
      {
        if (!has_bitset(&visited, ds[queued].d[i]))
        {
          set_bitset(&tovisit, ds[queued].d[i]);
        }
      }
    }
    if (ds[queued].default_tr != YALE_UINT_MAX_LEGAL)
    {
      if (!has_bitset(&visited, ds[queued].default_tr))
      {
        set_bitset(&tovisit, ds[queued].default_tr);
      }
    }
  }
  return max_backtrack;
}


void dfaviz(struct dfa_node *ds, yale_uint_t cnt)
{
  yale_uint_t i;
  size_t j;
  printf("digraph fsm {\n");
  for (i = 0; i < cnt; i++)
  {
    printf("n%d [label=\"%d%s%s%s\"];\n",
           (int)i, (int)i,
           ds[i].accepting ? "+" : "",
           ds[i].tainted ? "*" : "",
           ds[i].final ? "F" : "");
  }
  for (i = 0; i < cnt; i++)
  {
    for (j = 0; j < 256; j++)
    {
      if (ds[i].d[j] != YALE_UINT_MAX_LEGAL)
      {
        printf("n%d -> n%d [label=\"%d\"];\n", i, ds[i].d[j], (int)(unsigned char)j);
      }
    }
  }
  printf("}\n");
}

void nfaviz(struct nfa_node *ns, yale_uint_t cnt)
{
  yale_uint_t i;
  size_t j;
  size_t k;
  printf("digraph fsm {\n");
  for (i = 0; i < cnt; i++)
  {
    printf("n%d [label=\"%d%s\"];\n",
           (int)i, (int)i,
           ns[i].accepting ? "+" : "");
  }
  for (i = 0; i < cnt; i++)
  {
    for (j = 0; j < 256; j++)
    {
      struct bitset *bs = &ns[i].d[j];
      for (k = 0; k < YALE_UINT_MAX_LEGAL + 1; k++)
      {
        yale_uint_t wordoff = k/64;
        yale_uint_t bitoff = k%64;
        if (bs->bitset[wordoff] & (1ULL<<bitoff))
        {
          printf("n%zu -> n%zu [label=\"%c\"];\n", (size_t)i, k, (unsigned char)j);
        }
      }
    }
  }
  printf("}\n");
}

yale_uint_t nfa2dfa(struct nfa_node *ns, struct dfa_node *ds, yale_uint_t begin)
{
  struct bitset initial = {};
  struct bitset dfabegin = {};
  int tainted;
  struct bitset acceptidset = {};
  struct bitset taintidset = {};
  struct bitset defaults = {};
  const struct bitset defaults_empty = {};
  yale_uint_t wordoff = begin/64;
  yale_uint_t bitoff = begin%64;
  yale_uint_t curdfanode = 0;
  yale_uint_t dfanodeid, dfanodeid2;
  int accepting = 0;
  struct bitset queue[YALE_UINT_MAX_LEGAL + 1];
  size_t queuesz;
  struct bitset_hash d = {};
  size_t i, j;

  initial.bitset[wordoff] |= (1ULL<<bitoff);

  epsilonclosure(ns, initial, &dfabegin, &tainted, &acceptidset, &taintidset);
  for (i = 0; i < YALE_UINT_MAX_LEGAL + 1; /*i++*/)
  {
    wordoff = i/64;
    bitoff = i%64;
    if (dfabegin.bitset[wordoff] & (1ULL<<bitoff))
    {
      if (ns[i].accepting)
      {
        accepting = 1;
        break;
      }
    }
    if (bitoff != 63)
    {
      i = (wordoff*64) + myffsll(dfabegin.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
    }
    else
    {
      i++;
    }
  }

  d.tbl[d.tblsz].dfanodeid = curdfanode;
  memcpy(&d.tbl[d.tblsz++].key, &dfabegin, sizeof(dfabegin));
  dfa_init(&ds[curdfanode], accepting, tainted, &acceptidset, &taintidset);
  curdfanode++;

  queue[0] = dfabegin;
  queuesz = 1;

  while (queuesz)
  {
    struct bitset nns = queue[--queuesz];
    struct bitset d2[256] = {};
    struct bitset d2epsilon = {};
    struct bitset defaultsec;
    //printf("Iter\n");
    defaults = defaults_empty;
    for (i = 0; i < YALE_UINT_MAX_LEGAL + 1; /*i++*/) // for nn in nns
    {
      wordoff = i/64;
      bitoff = i%64;
      if (nns.bitset[wordoff] & (1ULL<<bitoff))
      {
        bitset_update(&defaults, &ns[i].defaults);
        //defaults.bitset[wordoff] |= (1ULL<<bitoff);
        for (j = 0; j < 256; j++) // for ch,nns2 in nn.d.items()
        {
          bitset_update(&d2[j], &ns[i].d[j]);
        }
        bitset_update(&d2epsilon, &ns[i].epsilon);
      }
      if (bitoff != 63)
      {
        i = (wordoff*64) + myffsll(nns.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
      }
      else
      {
        i++;
      }
    }
    epsilonclosure(ns, defaults, &defaultsec, &tainted, &acceptidset, &taintidset);
    if (!bitset_empty(&defaultsec))
    {
      dfanodeid = YALE_UINT_MAX_LEGAL;
      for (i = 0; i < d.tblsz; i++)
      {
        if (bitset_equal(&d.tbl[i].key, &defaultsec))
        {
          dfanodeid = d.tbl[i].dfanodeid;
          break;
        }
      }
      if (dfanodeid == YALE_UINT_MAX_LEGAL)
      {
        accepting = 0;
        for (i = 0; i < YALE_UINT_MAX_LEGAL + 1; /*i++*/)
        {
          wordoff = i/64;
          bitoff = i%64;
          if (defaultsec.bitset[wordoff] & (1ULL<<bitoff))
          {
            if (ns[i].accepting)
            {
              accepting = 1;
              break;
            }
          }
          if (bitoff != 63)
          {
            i = (wordoff*64) + myffsll(defaultsec.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
          }
          else
          {
            i++;
          }
        }
        if (curdfanode >= YALE_UINT_MAX_LEGAL)
        {
          abort();
        }
        d.tbl[d.tblsz].dfanodeid = curdfanode;
        memcpy(&d.tbl[d.tblsz++].key, &defaultsec, sizeof(defaultsec));
        dfa_init(&ds[curdfanode], accepting, tainted, &acceptidset, &taintidset);
        dfanodeid = curdfanode++;
        if (queuesz >= sizeof(queue)/sizeof(*queue))
        {
          abort();
        }
        queue[queuesz++] = defaultsec;
      }

      dfanodeid2 = YALE_UINT_MAX_LEGAL;
      for (i = 0; i < d.tblsz; i++)
      {
        if (bitset_equal(&d.tbl[i].key, &nns))
        {
          dfanodeid2 = d.tbl[i].dfanodeid;
          break;
        }
      }
      if (dfanodeid2 == YALE_UINT_MAX_LEGAL)
      {
        abort(); // FIXME fails
      }
      dfa_connect_default(&ds[dfanodeid2], dfanodeid);
      printf("Connected default\n");
    }
    for (i = 0; i < 256; i++)
    {
      struct bitset ec;
      if (bitset_empty(&d2[i]))
      {
        continue;
      }
      bitset_update(&d2[i], &defaults);
      epsilonclosure(ns, d2[i], &ec, &tainted, &acceptidset, &taintidset);

      dfanodeid = YALE_UINT_MAX_LEGAL;
      for (j = 0; j < d.tblsz; j++)
      {
        if (bitset_equal(&d.tbl[j].key, &ec))
        {
          dfanodeid = d.tbl[j].dfanodeid;
          break;
        }
      }
      if (dfanodeid == YALE_UINT_MAX_LEGAL)
      {
        accepting = 0;
        for (j = 0; j < YALE_UINT_MAX_LEGAL + 1; /*j++*/)
        {
          wordoff = j/64;
          bitoff = j%64;
          if (ec.bitset[wordoff] & (1ULL<<bitoff))
          {
            if (ns[j].accepting)
            {
              accepting = 1;
              break;
            }
          }
          if (bitoff != 63)
          {
            j = (wordoff*64) + myffsll(ec.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
          }
          else
          {
            j++;
          }
        }
        if (curdfanode >= YALE_UINT_MAX_LEGAL)
        {
          abort();
        }
        d.tbl[d.tblsz].dfanodeid = curdfanode;
        memcpy(&d.tbl[d.tblsz++].key, &ec, sizeof(ec));
        dfa_init(&ds[curdfanode], accepting, tainted, &acceptidset, &taintidset);
        dfanodeid = curdfanode++;
        if (queuesz >= sizeof(queue)/sizeof(*queue))
        {
          abort();
        }
        queue[queuesz++] = ec;
      }

      dfanodeid2 = YALE_UINT_MAX_LEGAL;
      for (j = 0; j < d.tblsz; j++)
      {
        if (bitset_equal(&d.tbl[j].key, &nns))
        {
          dfanodeid2 = d.tbl[j].dfanodeid;
          break;
        }
      }
      if (dfanodeid2 == YALE_UINT_MAX_LEGAL)
      {
        abort();
      }
      dfa_connect(&ds[dfanodeid2], i, dfanodeid);
    }
  }
  for (i = 0; i < curdfanode; i++)
  {
    for (j = 0; j < 256; j++)
    {
      if (ds[i].d[j] != YALE_UINT_MAX_LEGAL)
      {
        break;
      }
    }
    if (j == 256 && ds[i].default_tr == YALE_UINT_MAX_LEGAL)
    {
      ds[i].final = 1;
    }
  }
  return curdfanode;
}

struct re *parse_re(const char *re, size_t resz, size_t *remainderstart);

static inline void set_char(uint64_t bitmask[4], unsigned char ch)
{
  yale_uint_t wordoff = ch/64;
  yale_uint_t bitoff = ch%64;
  bitmask[wordoff] |= 1ULL<<bitoff;
}

struct re *
parse_bracketexpr(const char *re, size_t resz, size_t *remainderstart)
{
  uint64_t bitmask[4] = {};
  const char *start;
  const char *term;
  size_t i, len;
  int inverse = 0;
  int has_last = 0;
  char last = 0;
  if (resz == 0)
  {
    abort();
  }
  if (re[0] == '^')
  {
    if (resz < 2)
    {
      abort();
    }
    inverse = 1;
    start = re+1;
    term = memchr(re+2, ']', resz-2);
  }
  else
  {
    start = re;
    term = memchr(re+1, ']', resz-1);
  }
  len = term - start; // FIXME is return value correct for inverse?
  i = 0;
  while (i < len)
  {
    char first = start[i];
    i++;
    if (first == '\\')
    {
      char newlast;
      if (i == len)
      {
        abort(); // FIXME error handling
      }
      first = start[i];
      i++;
      if (first == 'n')
      {
        newlast = '\n';
      }
      else if (first == 'r')
      {
        newlast = '\r';
      }
      else if (first == 't')
      {
        newlast = '\t';
      }
      else if (first == 'x')
      {
        char hexbuf[3] = {0};
        if (i+2 >= len)
        {
          abort();
        }
        hexbuf[0] = start[i++];
        hexbuf[1] = start[i++];
        if (!isxdigit(hexbuf[0]) || !isxdigit(hexbuf[1]))
        {
          abort(); // FIXME error handling
        }
        newlast = strtol(hexbuf, NULL, 16);
      }
      else
      {
        abort(); // FIXME error handling
      }
      if (has_last)
      {
        set_char(bitmask, last);
      }
      last = newlast;
      has_last = 1;
    }
    else if (first == '-' && has_last && i < len)
    {
      char lastlast;
      has_last = 0;
      first = start[i];
      i++;
      if (first == '\\')
      {
        if (i == len)
        {
          abort(); // FIXME error handling
        }
        first = start[i];
        i++;
        if (first == 'n')
        {
          lastlast = '\n';
        }
        else if (first == 'r')
        {
          lastlast = '\r';
        }
        else if (first == 't')
        {
          lastlast = '\t';
        }
        else if (first == 'x')
        {
          char hexbuf[3] = {0};
          if (i+2 >= len)
          {
            abort();
          }
          hexbuf[0] = start[i++];
          hexbuf[1] = start[i++];
          if (!isxdigit(hexbuf[0]) || !isxdigit(hexbuf[1]))
          {
            abort(); // FIXME error handling
          }
          lastlast = strtol(hexbuf, NULL, 16);
        }
        else
        {
          abort(); // FIXME error handling
        }
      }
      else if (first == '-')
      {
        abort(); // FIXME error handling
      }
      else
      {
        lastlast = first;
      }
      size_t j;
      for (j = (unsigned char)last; j <= (unsigned char)lastlast; j++)
      {
        set_char(bitmask, j);
      }
    }
    else
    {
      if (has_last)
      {
        set_char(bitmask, last);
      }
      last = first;
      has_last = 1;
    }
  }
  if (has_last)
  {
    set_char(bitmask, last);
  }
  if (inverse)
  {
    bitmask[0] ^= UINT64_MAX;
    bitmask[1] ^= UINT64_MAX;
    bitmask[2] ^= UINT64_MAX;
    bitmask[3] ^= UINT64_MAX;
  }
  struct re *result = alloc_re();
  result->type = LITERALS;
  memcpy(result->u.lit.bitmask.bitset, bitmask, sizeof(bitmask));
#if 0
  printf("[");
  for (i = 0; i < 256; i++)
  {
    yale_uint_t wordoff = i/64;
    yale_uint_t bitoff = i%64;
    if (bitmask[wordoff] & 1ULL<<bitoff)
    {
      printf("%c", (char)(unsigned char)i);
    }
  }
  printf("]\n");
#endif
  *remainderstart = len + 1;
  return result;
}

struct re *parse_atom(const char *re, size_t resz, size_t *remainderstart)
{
  struct re *result;
  if (resz == 0 || re[0] == ')' || re[0] == '|')
  {
    result = alloc_re();
    result->type = EMPTYSTR;
    *remainderstart = 0;
    return result;
  }
  else if (re[0] == '[')
  {
    size_t bracketexprsz;
    result = parse_bracketexpr(re+1, resz-1, &bracketexprsz);
    *remainderstart = bracketexprsz + 1;
    return result;
  }
  else if (re[0] == '.')
  {
    result = alloc_re();
    result->type = LITERALS; // WAS: WILDCARD, FIXME!
    memset(&result->u.lit, 0xFF, sizeof(result->u.lit));
    *remainderstart = 1;
    return result;
  }
  else if (re[0] == '(')
  {
    size_t resz = 0;
    result = parse_re(re+1, resz-1, &resz);
    if (re[1+resz] != ')')
    {
      abort(); // FIXME error handling
    }
    *remainderstart = 2+resz;
    return result;
  }
  else
  {
    unsigned char uch = re[0];
    yale_uint_t wordoff = uch/64;
    yale_uint_t bitoff = uch%64;
    result = alloc_re();
    result->type = LITERALS;
    memset(&result->u.lit, 0, sizeof(result->u.lit));
    result->u.lit.bitmask.bitset[wordoff] |= (1ULL<<bitoff);
    *remainderstart = 1;
    return result;
  }
}

struct re *parse_piece(const char *re, size_t resz, size_t *remainderstart)
{
  size_t atomsz;
  struct re *re1;
  struct re *result;
  struct re *intermediate;
  re1 = parse_atom(re, resz, &atomsz);
  if (atomsz < resz)
  {
    if (re[atomsz] == '*')
    {
      result = alloc_re();
      result->type = STAR;
      result->u.star.re = re1;
      *remainderstart = atomsz+1;
      return result;
    }
    else if (re[atomsz] == '+')
    {
      intermediate = alloc_re();
      intermediate->type = STAR;
      intermediate->u.star.re = re1;
      result = alloc_re();
      result->type = CONCAT;
      result->u.cat.re1 = dup_re(re1);
      result->u.cat.re2 = intermediate;
      *remainderstart = atomsz+1;
      return result;
    }
    else if (re[atomsz] == '?')
    {
      intermediate = alloc_re();
      intermediate->type = EMPTYSTR;
      result = alloc_re();
      result->type = ALTERN;
      result->u.alt.re1 = re1;
      result->u.alt.re2 = intermediate;
      *remainderstart = atomsz+1;
      return result;
    }
  }
  *remainderstart = atomsz;
  return re1;
}

// branch: piece branch
struct re *parse_branch(const char *re, size_t resz, size_t *remainderstart)
{
  size_t piecesz, branchsz2;
  struct re *re1, *re2, *result;
  re1 = parse_piece(re, resz, &piecesz);
  if (piecesz < resz && re[piecesz] != '|' && re[piecesz] != ')')
  {
    re2 = parse_branch(re+piecesz, resz-piecesz, &branchsz2);
    result = alloc_re();
    result->type = CONCAT;
    result->u.cat.re1 = re1;
    result->u.cat.re2 = re2;
    *remainderstart = piecesz+branchsz2;
    return result;
  }
  else
  {
    *remainderstart = piecesz;
    return re1;
  }
}

// RE: branch | RE

struct re *parse_re(const char *re, size_t resz, size_t *remainderstart)
{
  struct re *result;
  size_t branchsz, branchsz2;
  struct re *re1, *re2;
  re1 = parse_branch(re, resz, &branchsz);
  if (branchsz < resz && re[branchsz] == '|')
  {
    re2 = parse_re(re+branchsz+1, resz-branchsz-1, &branchsz2);
    result = alloc_re();
    result->type = ALTERN;
    result->u.alt.re1 = re1;
    result->u.alt.re2 = re2;
    *remainderstart = branchsz+1+branchsz2;
    return result;
  }
  else
  {
    *remainderstart = branchsz;
    return re1;
  }
}

struct re *parse_res(struct iovec *regexps, yale_uint_t *pick_those, size_t resz)
{
  struct re **res;
  struct re *result;
  size_t i;
  yale_uint_t *pick_those2 = malloc(sizeof(*pick_those2)*resz);
  memcpy(pick_those2, pick_those, sizeof(*pick_those2)*resz);
  res = malloc(sizeof(*res)*resz);
  result = alloc_re();
  result->type = ALTERNMULTI;
  result->u.altmulti.res = res;
  result->u.altmulti.pick_those = pick_those2;
  result->u.altmulti.resz = resz;
  for (i = 0; i < resz; i++)
  {
    size_t regexplen = regexps[pick_those[i]].iov_len;
    size_t remainderstart;
    res[i] = parse_re((const char*)regexps[pick_those[i]].iov_base, regexplen, &remainderstart);
    if (remainderstart != regexplen)
    {
      abort(); // FIXME error handling
    }
  }
  return result;
}

void gennfa(struct re *regexp,
            struct nfa_node *ns, yale_uint_t *ncnt,
            yale_uint_t begin, yale_uint_t end,
            yale_uint_t taintid)
{
  switch (regexp->type)
  {
    case STAR:
    {
      yale_uint_t begin1 = (*ncnt)++;
      yale_uint_t end1 = (*ncnt)++;
      nfa_init(&ns[begin1], 0, taintid);
      nfa_init(&ns[end1], 0, taintid);
      gennfa(regexp->u.star.re, ns, ncnt, begin1, end1, taintid);
      nfa_connect_epsilon(&ns[begin], begin1);
      nfa_connect_epsilon(&ns[begin], end);
      nfa_connect_epsilon(&ns[end1], begin1);
      nfa_connect_epsilon(&ns[end1], end);
      return;
    }
    case WILDCARD:
    {
      abort();
    }
    case LITERALS:
    {
      size_t i;
      for (i = 0; i < 256; i++)
      {
        yale_uint_t wordoff = i/64;
        yale_uint_t bitoff = i%64;
        if (regexp->u.lit.bitmask.bitset[wordoff] & (1ULL<<bitoff))
        {
          nfa_connect(&ns[begin], (unsigned char)i, end);
        }
      }
      return;
    }
    case EMPTYSTR:
    {
      nfa_connect_epsilon(&ns[begin], end);
      return;
    }
    case ALTERN:
    {
      gennfa(regexp->u.cat.re1, ns, ncnt, begin, end, taintid);
      gennfa(regexp->u.cat.re2, ns, ncnt, begin, end, taintid);
      return;
    }
    case ALTERNMULTI:
    {
      abort();
    }
    case CONCAT:
    {
      yale_uint_t middle = (*ncnt)++;
      nfa_init(&ns[middle], 0, taintid);
      gennfa(regexp->u.cat.re1, ns, ncnt, begin, middle, taintid);
      gennfa(regexp->u.cat.re2, ns, ncnt, middle, end, taintid);
      return;
    }
  }
}

void gennfa_main(struct re *regexp,
                 struct nfa_node *ns, yale_uint_t *ncnt,
                 yale_uint_t taintid)
{
  yale_uint_t begin = (*ncnt)++;
  yale_uint_t end = (*ncnt)++;
  nfa_init(&ns[begin], 0, YALE_UINT_MAX_LEGAL);
  nfa_init(&ns[end], 1, YALE_UINT_MAX_LEGAL);
  gennfa(regexp, ns, ncnt, begin, end, taintid);
}

void gennfa_alternmulti(struct re *regexp,
                        struct nfa_node *ns, yale_uint_t *ncnt)
{
  yale_uint_t begin = (*ncnt)++;
  size_t i;
  nfa_init(&ns[begin], 0, YALE_UINT_MAX_LEGAL);
  for (i = 0; i < regexp->u.altmulti.resz; i++)
  {
    yale_uint_t end = (*ncnt)++;
    nfa_init(&ns[end], 1, regexp->u.altmulti.pick_those[i]);
    gennfa(regexp->u.altmulti.res[i], ns, ncnt, begin, end, regexp->u.altmulti.pick_those[i]);
  }
}

uint32_t transition_hash(const yale_uint_t *transitions)
{
  return yalemurmur_buf(0x12345678U, transitions, 256*sizeof(*transitions));
}

uint32_t transition_hash_fn(struct yale_hash_list_node *node, void *ud)
{
  return transition_hash(YALE_CONTAINER_OF(node, struct transitionbuf, node)->transitions);
}

size_t
get_transid(const yale_uint_t *transitions, struct transitionbufs *bufs,
            void *(*alloc)(void*, size_t), void *alloc_ud)
{
  uint32_t hashval;
  struct yale_hash_list_node *node;
  struct transitionbuf *bufnew;
  hashval = transition_hash(transitions);
  YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&bufs->hash, node, hashval)
  {
    struct transitionbuf *buf = YALE_CONTAINER_OF(node, struct transitionbuf, node);
    if (memcmp(transitions, buf->transitions, 256*sizeof(*transitions)) == 0)
    {
      return buf->id;
    }
  }
  if (bufs->cnt >= sizeof(bufs->all)/sizeof(*bufs->all))
  {
    abort();
  }
  bufnew = alloc(alloc_ud, sizeof(struct transitionbuf));
  memcpy(bufnew->transitions, transitions, 256*sizeof(*transitions));
  bufnew->id = bufs->cnt;
  yale_hash_table_add_nogrow(&bufs->hash, &bufnew->node, hashval);
  bufs->all[bufs->cnt] = bufnew;
  bufs->cnt++;
  return bufnew->id;
}

void
perf_trans(yale_uint_t *transitions, struct transitionbufs *bufs,
           void *(*alloc)(void*, size_t), void *alloc_ud)
{
  size_t i;
  memset(bufs, 0, sizeof(*bufs));
  for (i = 0; i < MAX_TRANS; i++)
  {
    transitions[0] = i&0xFF;
    transitions[1] = i>>8;
    get_transid(transitions, bufs, alloc, alloc_ud);
  }
}

void
pick(struct nfa_node *nsglobal, struct dfa_node *dsglobal,
     struct iovec *res, struct pick_those_struct *pick_those, int *priorities)
{
  yale_uint_t ncnt;
  struct re *re;
  size_t i;
  for (i = 0; i < YALE_UINT_MAX_LEGAL; i++)
  {
    dfa_init_empty(&dsglobal[i]);
  }
  ncnt = 0;
  re = parse_res(res, pick_those->pick_those, pick_those->len);
  gennfa_alternmulti(re, nsglobal, &ncnt);
  free_re(re);
  pick_those->dscnt = nfa2dfa(nsglobal, dsglobal, 0);
  set_accepting(dsglobal, 0, priorities);
  pick_those->ds = malloc(sizeof(*pick_those->ds)*pick_those->dscnt);
  memcpy(pick_those->ds, dsglobal, sizeof(*pick_those->ds)*pick_those->dscnt);
}

void
collect(struct pick_those_struct *pick_thoses, size_t cnt,
        struct transitionbufs *bufs, void *(*alloc)(void*, size_t), void *alloc_ud)
{
  size_t i, j;
  for (i = 0; i < cnt; i++)
  {
    struct dfa_node *ds = pick_thoses[i].ds;
    yale_uint_t dscnt = pick_thoses[i].dscnt;
    for (j = 0; j < dscnt; j++)
    {
      ds[j].transitions_id = get_transid(ds[j].d, bufs, alloc, alloc_ud);
    }
  }
}

static int fprints(FILE *f, const char *s)
{
  return fputs(s, f);
}

void dump_headers(FILE *f, const char *parsername, size_t max_bt, size_t cbssz)
{
  char *parserupper = strdup(parsername);
  size_t len = strlen(parsername);
  size_t i;
  for (i = 0; i < len; i++)
  {
    parserupper[i] = toupper((unsigned char)parserupper[i]);
  }
  fprintf(f, "#define %s_BACKTRACKLEN (%zu)\n", parserupper, max_bt);
  fprintf(f, "#define %s_BACKTRACKLEN_PLUS_1 ((%s_BACKTRACKLEN) + 1)\n", parserupper, parserupper);
  fprints(f, "\n");
  fprintf(f, "struct %s_parserctx;", parsername);
  fprints(f, "\n");
  fprintf(f, "struct %s_rectx {\n", parsername);
  fprints(f, "  lexer_uint_t state; // 0 is initial state\n");
  fprints(f, "  lexer_uint_t last_accept; // LEXER_UINT_MAX means never accepted\n");
  fprintf(f, "#if %s_BACKTRACKLEN_PLUS_1 > 1\n", parserupper);
  fprints(f, "  uint8_t backtrackstart;\n"); // FIXME uint8_t
  fprints(f, "  uint8_t backtrackmid;\n"); // FIXME uint8_t
  fprints(f, "  uint8_t backtrackend;\n"); // FIXME uint8_t
  fprintf(f, "  unsigned char backtrack[%s_BACKTRACKLEN_PLUS_1];\n", parserupper);
  fprints(f, "#endif\n");
  fprints(f, "};\n");
  fprints(f, "\n");
  fprints(f, "static inline void\n");
  fprintf(f, "%s_init_statemachine(struct %s_rectx *ctx)\n", parsername, parsername);
  fprints(f, "{\n");
  fprints(f, "  ctx->state = 0;\n");
  fprints(f, "  ctx->last_accept = LEXER_UINT_MAX;\n");
  fprintf(f, "#if %s_BACKTRACKLEN_PLUS_1 > 1\n", parserupper);
  fprints(f, "  ctx->backtrackstart = 0;\n");
  fprints(f, "  ctx->backtrackmid = 0;\n");
  fprints(f, "  ctx->backtrackend = 0;\n");
  fprints(f, "#endif\n");
  fprints(f, "}\n");
  fprints(f, "\n");
  if (cbssz)
  {
    fprints(f, "ssize_t\n");
    fprintf(f, "%s_feed_statemachine(struct %s_rectx *ctx, const struct state *stbl, const void *buf, size_t sz, parser_uint_t *state, ssize_t(*cbtbl[])(const char*, size_t, int, struct %s_parserctx*), const struct callbacks *cb2, parser_uint_t cb1, const parser_uint_t *cbstack, parser_uint_t cbstacksz);//, void *baton);\n", parsername, parsername, parsername);
  }
  else
  {
    fprints(f, "ssize_t\n");
    fprintf(f, "%s_feed_statemachine(struct %s_rectx *ctx, const struct state *stbl, const void *buf, size_t sz, parser_uint_t *state, ssize_t(*cbtbl[])(const char*, size_t, int, struct %s_parserctx*), const struct callbacks *cb2, parser_uint_t cb1);//, void *baton);\n", parsername, parsername, parsername);
  }
  fprints(f, "\n");
  free(parserupper);
}

void
dump_collected(FILE *f, const char *parsername, struct transitionbufs *bufs)
{
  size_t i;
  size_t j;
  fprints(f, "#ifdef SMALL_CODE\n");
  fprintf(f, "const lexer_uint_t %s_transitiontbl[][256] = {\n", parsername);
  for (i = 0; i < bufs->cnt; i++)
  {
    fprintf(f, "{");
    for (j = 0; j < 256; j++)
    {
      if (bufs->all[i]->transitions[j] == YALE_UINT_MAX_LEGAL)
      {
        fprints(f, "LEXER_UINT_MAX, ");
      }
      else
      {
        fprintf(f, "%d, ", (int)bufs->all[i]->transitions[j]);
      }
    }
    fprints(f, "},\n");
  }
  fprints(f, "};\n");
  fprints(f, "#endif\n");
}

void
dump_one(FILE *f, const char *parsername, struct pick_those_struct *pick_those,
         struct numbers_sets *numbershash,
         void *(*alloc)(void*,size_t), void *allocud)
{
  size_t i, j;
  for (i = 0; i < pick_those->dscnt; i++)
  {
    struct dfa_node *ds = &pick_those->ds[i];
    numbers_sets_emit(f, numbershash, &ds->taintidset, alloc, allocud);
  }
  fprintf(f, "const struct state %s_states", parsername);
  for (i = 0; i < pick_those->len; i++)
  {
    fprintf(f, "_%d", pick_those->pick_those[i]);
  }
  fprints(f, "[] = {\n");
  for (i = 0; i < pick_those->dscnt; i++)
  {
    struct dfa_node *ds = &pick_those->ds[i];
    if (ds->acceptid == YALE_UINT_MAX_LEGAL)
    {
      fprintf(f, "{ .accepting = %d, .acceptid = PARSER_UINT_MAX, .final = %d,\n",
                 (int)ds->accepting, (int)ds->final);
    }
    else
    {
      fprintf(f, "{ .accepting = %d, .acceptid = %d, .final = %d,\n",
                 (int)ds->accepting, (int)ds->acceptid, (int)ds->final);
    }
    fprints(f, ".taintids = taintidsetarray");
    for (j = 0; j < YALE_UINT_MAX_LEGAL + 1; /*j++*/)
    {
      yale_uint_t wordoff = j/64;
      yale_uint_t bitoff = j%64;
      if (ds->taintidset.bitset[wordoff] & (1ULL<<bitoff))
      {
        fprintf(f, "_%d", (int)j);
      }
      if (bitoff != 63)
      {
        j = (wordoff*64) + myffsll(ds->taintidset.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
      }
      else
      {
        j++;
      }
    }
    fprints(f, ", .taintidsz = sizeof(taintidsetarray");
    for (j = 0; j < YALE_UINT_MAX_LEGAL + 1; /*j++*/)
    {
      yale_uint_t wordoff = j/64;
      yale_uint_t bitoff = j%64;
      if (ds->taintidset.bitset[wordoff] & (1ULL<<bitoff))
      {
        fprintf(f, "_%d", (int)j);
      }
      if (bitoff != 63)
      {
        j = (wordoff*64) + myffsll(ds->taintidset.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
      }
      else
      {
        j++;
      }
    }
    fprints(f, ")/sizeof(parser_uint_t),\n");
    fprints(f, ".fastpathbitmask = {");
    if (ds->accepting && !ds->final)
    {
      size_t iid, jid;
      uint64_t curval;
      for (iid = 0; iid < 4; iid++)
      {
        curval = 0;
        for (jid = 0; jid < 64; jid++)
        {
          unsigned char uch = 64*iid + jid;
          if (ds->d[uch] == i)
          {
            curval |= (1ULL<<jid);
          }
          else if (ds->default_tr == i)
          {
            curval |= (1ULL<<jid);
          }
        }
        fprintf(f, "0x%llx,", (unsigned long long)curval);
      }
    }
    fprints(f, "},\n");
    fprints(f, "#ifdef SMALL_CODE\n");
    fprintf(f, ".transitions = %s_transitiontbl[%zu],\n", parsername, ds->transitions_id);
    fprints(f, "#else\n");
    fprints(f, ".transitions = {");
    for (j = 0; j < 256; j++)
    {
      if (ds->d[j] == YALE_UINT_MAX_LEGAL)
      {
        fprints(f, "LEXER_UINT_MAX, ");
      }
      else
      {
        fprintf(f, "%d, ", (int)ds->d[j]);
      }
    }
    fprints(f, "},\n");
    fprints(f, "#endif\n");
    fprints(f, "},\n");
  }
  fprints(f, "};\n");
}

void
dump_chead(FILE *f, const char *parsername, int nofastpath, size_t cbssz)
{
  char *parserupper = strdup(parsername);
  size_t len = strlen(parsername);
  size_t i;
  for (i = 0; i < len; i++)
  {
    parserupper[i] = toupper((unsigned char)parserupper[i]);
  }
  if (!nofastpath)
  {
    fprints(f, "static inline int\n");
    fprintf(f, "%s_is_fastpath(const struct state *st, unsigned char uch)\n", parsername);
    fprints(f, "{\n");
    fprints(f, "  return !!(st->fastpathbitmask[uch/64] & (1ULL<<(uch%64)));\n");
    fprints(f, "}\n");
    fprints(f, "\n");
  }
  if (cbssz)
  {
    fprints(f, "ssize_t\n");
    fprintf(f, "%s_feed_statemachine(struct %s_rectx *ctx, const struct state *stbl, const void *buf, size_t sz, parser_uint_t *state, ssize_t(*cbtbl[])(const char*, size_t, int, struct %s_parserctx*), const struct callbacks *cb2, parser_uint_t cb1, const parser_uint_t *cbstack, parser_uint_t cbstacksz)//, void *baton)\n", parsername, parsername, parsername);
  }
  else
  {
    fprints(f, "ssize_t\n");
    fprintf(f, "%s_feed_statemachine(struct %s_rectx *ctx, const struct state *stbl, const void *buf, size_t sz, parser_uint_t *state, ssize_t(*cbtbl[])(const char*, size_t, int, struct %s_parserctx*), const struct callbacks *cb2, parser_uint_t cb1)//, void *baton)\n", parsername, parsername, parsername);
  }
  fprints(f, "{\n");
  fprints(f, "  const unsigned char *ubuf = (unsigned char*)buf;\n");
  fprints(f, "  const struct state *st = NULL;\n");
  fprints(f, "  size_t i;\n");
  fprints(f, "  int start = 0;\n");
  fprints(f, "  lexer_uint_t newstate;\n");
  fprintf(f, "  struct %s_parserctx *pctx = CONTAINER_OF(ctx, struct %s_parserctx, rctx);\n", parsername, parsername);
  fprints(f, "  if (ctx->state == LEXER_UINT_MAX)\n");
  fprints(f, "  {\n");
  fprints(f, "    *state = PARSER_UINT_MAX;\n");
  fprints(f, "    return -EINVAL;\n");
  fprints(f, "  }\n");
  fprints(f, "  if (ctx->state == 0)\n");
  fprints(f, "  {\n");
  fprints(f, "    start = 1;\n");
  fprints(f, "  }\n");
  fprints(f, "  //printf(\"Called: %s\\n\", buf);\n");
  fprintf(f, "#if %s_BACKTRACKLEN_PLUS_1 > 1\n", parserupper);
  fprints(f, "  if (unlikely(ctx->backtrackmid != ctx->backtrackend))\n");
  fprints(f, "  {\n");
  fprints(f, "    while (ctx->backtrackmid != ctx->backtrackend)\n");
  fprints(f, "    {\n");
  fprints(f, "      st = &stbl[ctx->state];\n");
  fprints(f, "      ctx->state = st->transitions[ctx->backtrack[ctx->backtrackmid]];\n");
  fprints(f, "      if (unlikely(ctx->state == LEXER_UINT_MAX))\n");
  fprints(f, "      {\n");
  fprints(f, "        if (ctx->last_accept == LEXER_UINT_MAX)\n");
  fprints(f, "        {\n");
  fprints(f, "          *state = PARSER_UINT_MAX;\n");
  fprints(f, "          return -EINVAL;\n");
  fprints(f, "        }\n");
  fprints(f, "        ctx->state = ctx->last_accept;\n");
  fprints(f, "        ctx->last_accept = LEXER_UINT_MAX;\n");
  fprints(f, "        st = &stbl[ctx->state];\n");
  fprints(f, "        *state = st->acceptid;\n");
  fprints(f, "        ctx->state = 0;\n");
  fprints(f, "        return 0;\n");
  fprints(f, "      }\n");
  fprints(f, "      ctx->backtrackmid++;\n");
  fprintf(f, "      if (ctx->backtrackmid >= %s_BACKTRACKLEN_PLUS_1)\n", parserupper);
  fprints(f, "      {\n");
  fprints(f, "        ctx->backtrackmid = 0;\n");
  fprints(f, "      }\n");
  fprints(f, "      st = &stbl[ctx->state];\n");
  fprints(f, "      if (st->accepting)\n");
  fprints(f, "      {\n");
  fprints(f, "        if (st->final)\n");
  fprints(f, "        {\n");
  fprints(f, "          *state = st->acceptid;\n");
  fprints(f, "          ctx->state = 0;\n");
  fprints(f, "          ctx->last_accept = LEXER_UINT_MAX;\n");
  fprints(f, "          ctx->backtrackstart = ctx->backtrackmid;\n");
  fprints(f, "          return 0;\n");
  fprints(f, "        }\n");
  fprints(f, "        else\n");
  fprints(f, "        {\n");
  fprints(f, "          ctx->last_accept = ctx->state; // FIXME correct?\n");
  fprints(f, "          ctx->backtrackstart = ctx->backtrackmid; // FIXME correct?\n");
  fprints(f, "        }\n");
  fprints(f, "      }\n");
  fprints(f, "    }\n");
  fprints(f, "  }\n");
  fprints(f, "#endif\n");
  fprints(f, "  for (i = 0; i < sz; i++)\n");
  fprints(f, "  {\n");
  fprints(f, "    st = &stbl[ctx->state];\n");
  if (!nofastpath)
  {
    fprintf(f, "    if (%s_is_fastpath(st, ubuf[i]))\n", parsername);
    fprints(f, "    {\n");
    fprints(f, "      ctx->last_accept = ctx->state;\n");
    fprints(f, "      while (i + 8 < sz) // FIXME test this thoroughly, all branches!\n");
    fprints(f, "      {\n");
    fprintf(f, "        if (!%s_is_fastpath(st, ubuf[i+1]))\n", parsername);
    fprints(f, "        {\n");
    fprints(f, "          i += 0;\n");
    fprints(f, "          break;\n");
    fprints(f, "        }\n");
    fprintf(f, "        if (!%s_is_fastpath(st, ubuf[i+2]))\n", parsername);
    fprints(f, "        {\n");
    fprints(f, "          i += 1;\n");
    fprints(f, "          break;\n");
    fprints(f, "        }\n");
    fprintf(f, "        if (!%s_is_fastpath(st, ubuf[i+3]))\n", parsername);
    fprints(f, "        {\n");
    fprints(f, "          i += 2;\n");
    fprints(f, "          break;\n");
    fprints(f, "        }\n");
    fprintf(f, "        if (!%s_is_fastpath(st, ubuf[i+4]))\n", parsername);
    fprints(f, "        {\n");
    fprints(f, "          i += 3;\n");
    fprints(f, "          break;\n");
    fprints(f, "        }\n");
    fprintf(f, "        if (!%s_is_fastpath(st, ubuf[i+5]))\n", parsername);
    fprints(f, "        {\n");
    fprints(f, "          i += 4;\n");
    fprints(f, "          break;\n");
    fprints(f, "        }\n");
    fprintf(f, "        if (!%s_is_fastpath(st, ubuf[i+6]))\n", parsername);
    fprints(f, "        {\n");
    fprints(f, "          i += 5;\n");
    fprints(f, "          break;\n");
    fprints(f, "        }\n");
    fprintf(f, "        if (!%s_is_fastpath(st, ubuf[i+7]))\n", parsername);
    fprints(f, "        {\n");
    fprints(f, "          i += 6;\n");
    fprints(f, "          break;\n");
    fprints(f, "        }\n");
    fprintf(f, "        if (!%s_is_fastpath(st, ubuf[i+8]))\n", parsername);
    fprints(f, "        {\n");
    fprints(f, "          i += 7;\n");
    fprints(f, "          break;\n");
    fprints(f, "        }\n");
    fprints(f, "        i += 8;\n");
    fprints(f, "      }\n");
    fprints(f, "      continue;\n");
    fprints(f, "    }\n");
  }
  fprints(f, "    newstate = st->transitions[ubuf[i]];\n");
  fprints(f, "    if (newstate != ctx->state) // Improves perf a lot\n");
  fprints(f, "    {\n");
  fprints(f, "      ctx->state = newstate;\n");
  fprints(f, "    }\n");
  fprints(f, "    //printf(\"New state: %d\\n\", ctx->state);\n");
  fprints(f, "    if (unlikely(newstate == LEXER_UINT_MAX)) // use newstate here, not ctx->state, faster\n");
  fprints(f, "    {\n");
  fprints(f, "      if (ctx->last_accept == LEXER_UINT_MAX)\n");
  fprints(f, "      {\n");
  fprints(f, "        *state = PARSER_UINT_MAX;\n");
  fprints(f, "        //printf(\"Error\\n\");\n");
  fprints(f, "        return -EINVAL;\n");
  fprints(f, "      }\n");
  fprints(f, "      ctx->state = ctx->last_accept;\n");
  fprints(f, "      ctx->last_accept = LEXER_UINT_MAX;\n");
  fprintf(f, "#if %s_BACKTRACKLEN_PLUS_1 > 1\n", parserupper);
  fprints(f, "      ctx->backtrackmid = ctx->backtrackstart;\n");
  fprintf(f, "#endif\n");
  fprints(f, "      st = &stbl[ctx->state];\n");
  fprints(f, "      *state = st->acceptid;\n");
  fprints(f, "      ctx->state = 0;\n");
  fprints(f, "      if (cb2 && st->accepting)\n");
  fprints(f, "      {\n");
  fprints(f, "        size_t cbidx;\n");
  fprints(f, "        const struct callbacks *mycb = &cb2[st->acceptid];\n");
  fprintf(f, "        enum yale_flags flags = 0;\n");
  if (cbssz)
  {
    fprints(f, "        uint64_t cbmask = 0;\n"); // FIXME may need a bigger one
    fprints(f, "        uint16_t bitoff;\n");
  }
  fprintf(f, "        ssize_t cbr;\n");
  fprintf(f, "        if (start)\n");
  fprintf(f, "        {\n");
  fprintf(f, "          flags |= YALE_FLAG_START;\n");
  fprintf(f, "        }\n");
  fprintf(f, "        flags |= YALE_FLAG_END;\n");
  if (cbssz)
  {
    fprints(f, "        for (cbidx = 0; cbidx < cbstacksz; cbidx++)\n");
    fprints(f, "        {\n");
    fprints(f, "          cbmask |= (1ULL << cbstack[cbidx]);\n");
    fprints(f, "        }\n");
    fprints(f, "        for (cbidx = 0; cbidx < mycb->cbsz; cbidx++)\n");
    fprints(f, "        {\n");
    fprints(f, "          cbmask |= (1ULL<<mycb->cbs[cbidx]);\n");
    fprints(f, "        }\n");
    fprints(f, "        for (bitoff = 0; bitoff < 64; )\n");
    fprints(f, "        {\n");
    fprints(f, "          int ffsres;\n");
    fprints(f, "          if (cbmask & (1ULL<<bitoff))\n");
    fprints(f, "          {\n");
    fprints(f, "            cbr = cbtbl[bitoff](buf, i, flags, pctx);\n");
    fprints(f, "            if (cbr != (ssize_t)i && cbr != -EAGAIN && cbr != -EWOULDBLOCK)\n");
    fprints(f, "            {\n");
    fprints(f, "              *state = PARSER_UINT_MAX;");
    fprints(f, "              return cbr;");
    fprints(f, "            }\n");
    fprints(f, "          }\n");
    fprints(f, "          ffsres = ffsll(cbmask & ~((1ULL<<(bitoff+1))-1));\n");
    fprints(f, "          if (ffsres == 0)\n");
    fprints(f, "          {\n");
    fprints(f, "            bitoff = 64;\n");
    fprints(f, "          }\n");
    fprints(f, "          else\n");
    fprints(f, "          {\n");
    fprints(f, "            bitoff = ffsres - 1;\n");
    fprints(f, "          }\n");
    fprints(f, "        }\n");
  }
  else
  {
    fprints(f, "        for (cbidx = 0; cbidx < mycb->cbsz; cbidx++)\n");
    fprints(f, "        {\n");
    fprints(f, "          cbr = cbtbl[mycb->cbs[cbidx]](buf, i, flags, pctx);\n");
    fprints(f, "          if (cbr != (ssize_t)i && cbr != -EAGAIN && cbr != -EWOULDBLOCK)\n");
    fprints(f, "          {\n");
    fprints(f, "            *state = PARSER_UINT_MAX;");
    fprints(f, "            return cbr;");
    fprints(f, "          }\n");
    fprints(f, "        }\n");
  }
  fprints(f, "      }\n");
  fprints(f, "      if (cb1 != PARSER_UINT_MAX && st->accepting)\n");
  fprints(f, "      {\n");
  if (cbssz)
  {
    fprints(f, "        size_t cbidx;\n");
    fprints(f, "        uint64_t cbmask = 1ULL<<cb1;\n"); // FIXME may need a bigger one
    fprints(f, "        uint16_t bitoff;\n");
  }
  fprintf(f, "        enum yale_flags flags = 0;\n");
  fprintf(f, "        ssize_t cbr;\n");
  fprintf(f, "        if (start)\n");
  fprintf(f, "        {\n");
  fprintf(f, "          flags |= YALE_FLAG_START;\n");
  fprintf(f, "        }\n");
  fprintf(f, "        flags |= YALE_FLAG_END;\n");
  if (cbssz)
  {
    fprints(f, "        for (cbidx = 0; cbidx < cbstacksz; cbidx++)\n");
    fprints(f, "        {\n");
    fprints(f, "          cbmask |= (1ULL << cbstack[cbidx]);\n");
    fprints(f, "        }\n");
    fprints(f, "        for (bitoff = 0; bitoff < 64; )\n");
    fprints(f, "        {\n");
    fprints(f, "          int ffsres;\n");
    fprints(f, "          if (cbmask & (1ULL<<bitoff))\n");
    fprints(f, "          {\n");
    fprints(f, "            cbr = cbtbl[bitoff](buf, i, flags, pctx);\n");
    fprints(f, "            if (cbr != (ssize_t)i && cbr != -EAGAIN && cbr != -EWOULDBLOCK)\n");
    fprints(f, "            {\n");
    fprints(f, "              *state = PARSER_UINT_MAX;");
    fprints(f, "              return cbr;");
    fprints(f, "            }\n");
    fprints(f, "          }\n");
    fprints(f, "          ffsres = ffsll(cbmask & ~((1ULL<<(bitoff+1))-1));\n");
    fprints(f, "          if (ffsres == 0)\n");
    fprints(f, "          {\n");
    fprints(f, "            bitoff = 64;\n");
    fprints(f, "          }\n");
    fprints(f, "          else\n");
    fprints(f, "          {\n");
    fprints(f, "            bitoff = ffsres - 1;\n");
    fprints(f, "          }\n");
    fprints(f, "        }\n");
  }
  else
  {
    fprints(f, "        cbr = cbtbl[cb1](buf, i, flags, pctx);\n");
    fprints(f, "        if (cbr != (ssize_t)i && cbr != -EAGAIN && cbr != -EWOULDBLOCK)\n");
    fprints(f, "        {\n");
    fprints(f, "          *state = PARSER_UINT_MAX;");
    fprints(f, "          return cbr;");
    fprints(f, "        }\n");
  }
  fprints(f, "      }\n");
  fprints(f, "      return i;\n");
  fprints(f, "    }\n");
  fprints(f, "    st = &stbl[ctx->state]; // strangely, ctx->state seems faster here\n");
  fprints(f, "    if (st->accepting)\n");
  fprints(f, "    {\n");
  fprints(f, "      if (st->final)\n");
  fprints(f, "      {\n");
  fprints(f, "        *state = st->acceptid;\n");
  fprints(f, "        ctx->state = 0;\n");
  fprints(f, "        ctx->last_accept = LEXER_UINT_MAX;\n");
  fprints(f, "        if (cb2 && st->accepting)\n");
  fprints(f, "        {\n");
  fprintf(f, "          enum yale_flags flags = 0;\n");
  if (cbssz)
  {
    fprints(f, "          uint64_t cbmask = 0;\n"); // FIXME may need a bigger one
    fprints(f, "          uint16_t bitoff;\n");
  }
  fprintf(f, "          ssize_t cbr;\n");
  fprints(f, "          size_t cbidx;\n");
  fprints(f, "          const struct callbacks *mycb = &cb2[st->acceptid];\n");
  fprintf(f, "          if (start)\n");
  fprintf(f, "          {\n");
  fprintf(f, "            flags |= YALE_FLAG_START;\n");
  fprintf(f, "          }\n");
  fprintf(f, "          flags |= YALE_FLAG_END;\n");
  if (cbssz)
  {
    fprints(f, "          for (cbidx = 0; cbidx < cbstacksz; cbidx++)\n");
    fprints(f, "          {\n");
    fprints(f, "            cbmask |= (1ULL << cbstack[cbidx]);\n");
    fprints(f, "          }\n");
    fprints(f, "          for (cbidx = 0; cbidx < mycb->cbsz; cbidx++)\n");
    fprints(f, "          {\n");
    fprints(f, "            cbmask |= (1ULL << mycb->cbs[cbidx]);\n");
    fprints(f, "          }\n");
    fprints(f, "          for (bitoff = 0; bitoff < 64; )\n");
    fprints(f, "          {\n");
    fprints(f, "            int ffsres;\n");
    fprints(f, "            if (cbmask & (1ULL<<bitoff))\n");
    fprints(f, "            {\n");
    fprints(f, "              cbr = cbtbl[bitoff](buf, i + 1, flags, pctx);\n");
    fprints(f, "              if (cbr != (ssize_t)i+1 && cbr != -EAGAIN && cbr != -EWOULDBLOCK)\n");
    fprints(f, "              {\n");
    fprints(f, "                *state = PARSER_UINT_MAX;");
    fprints(f, "                return cbr;");
    fprints(f, "              }\n");
    fprints(f, "            }\n");
    fprints(f, "            ffsres = ffsll(cbmask & ~((1ULL<<(bitoff+1))-1));\n");
    fprints(f, "            if (ffsres == 0)\n");
    fprints(f, "            {\n");
    fprints(f, "              bitoff = 64;\n");
    fprints(f, "            }\n");
    fprints(f, "            else\n");
    fprints(f, "            {\n");
    fprints(f, "              bitoff = ffsres - 1;\n");
    fprints(f, "            }\n");
    fprints(f, "          }\n");
  }
  else
  {
    fprints(f, "          for (cbidx = 0; cbidx < mycb->cbsz; cbidx++)\n");
    fprints(f, "          {\n");
    fprints(f, "            cbr = cbtbl[mycb->cbs[cbidx]](buf, i + 1, flags, pctx);\n");
    fprints(f, "            if (cbr != (ssize_t)i+1 && cbr != -EAGAIN && cbr != -EWOULDBLOCK)\n");
    fprints(f, "            {\n");
    fprints(f, "              *state = PARSER_UINT_MAX;");
    fprints(f, "              return cbr;");
    fprints(f, "            }\n");
    fprints(f, "          }\n");
  }
  fprints(f, "        }\n");
  fprints(f, "        if (cb1 != PARSER_UINT_MAX && st->accepting)\n");
  fprints(f, "        {\n");
  if (cbssz)
  {
    fprints(f, "          size_t cbidx;\n");
    fprints(f, "          uint64_t cbmask = 1ULL<<cb1;\n"); // FIXME may need a bigger one
    fprints(f, "          uint16_t bitoff;\n");
  }
  fprintf(f, "          enum yale_flags flags = 0;\n");
  fprintf(f, "          ssize_t cbr;\n");
  fprintf(f, "          if (start)\n");
  fprintf(f, "          {\n");
  fprintf(f, "            flags |= YALE_FLAG_START;\n");
  fprintf(f, "          }\n");
  fprintf(f, "          flags |= YALE_FLAG_END;\n");
  if (cbssz)
  {
    fprints(f, "          for (cbidx = 0; cbidx < cbstacksz; cbidx++)\n");
    fprints(f, "          {\n");
    fprints(f, "            cbmask |= (1ULL << cbstack[cbidx]);\n");
    fprints(f, "          }\n");
    fprints(f, "          for (bitoff = 0; bitoff < 64; )\n");
    fprints(f, "          {\n");
    fprints(f, "            int ffsres;\n");
    fprints(f, "            if (cbmask & (1ULL<<bitoff))\n");
    fprints(f, "            {\n");
    fprints(f, "              cbr = cbtbl[bitoff](buf, i + 1, flags, pctx);\n");
    fprints(f, "              if (cbr != (ssize_t)i+1 && cbr != -EAGAIN && cbr != -EWOULDBLOCK)\n");
    fprints(f, "              {\n");
    fprints(f, "                *state = PARSER_UINT_MAX;");
    fprints(f, "                return cbr;");
    fprints(f, "              }\n");
    fprints(f, "            }\n");
    fprints(f, "            ffsres = ffsll(cbmask & ~((1ULL<<(bitoff+1))-1));\n");
    fprints(f, "            if (ffsres == 0)\n");
    fprints(f, "            {\n");
    fprints(f, "              bitoff = 64;\n");
    fprints(f, "            }\n");
    fprints(f, "            else\n");
    fprints(f, "            {\n");
    fprints(f, "              bitoff = ffsres - 1;\n");
    fprints(f, "            }\n");
    fprints(f, "          }\n");
  }
  else
  {
    fprints(f, "          cbr = cbtbl[cb1](buf, i + 1, flags, pctx);\n");
    fprints(f, "          if (cbr != (ssize_t)i+1 && cbr != -EAGAIN && cbr != -EWOULDBLOCK)\n");
    fprints(f, "          {\n");
    fprints(f, "            *state = PARSER_UINT_MAX;");
    fprints(f, "            return cbr;");
    fprints(f, "          }\n");
  }
  fprints(f, "        }\n");
  fprintf(f, "#if %s_BACKTRACKLEN_PLUS_1 > 1\n", parserupper);
  fprints(f, "        ctx->backtrackstart = ctx->backtrackend;\n");
  fprints(f, "#endif\n");
  fprints(f, "        return i + 1;\n");
  fprints(f, "      }\n");
  fprints(f, "      else\n");
  fprints(f, "      {\n");
  fprints(f, "        ctx->last_accept = ctx->state; // FIXME correct?\n");
  fprintf(f, "#if %s_BACKTRACKLEN_PLUS_1 > 1\n", parserupper);
  fprints(f, "        ctx->backtrackstart = ctx->backtrackend;\n");
  fprints(f, "#endif\n");
  fprints(f, "      }\n");
  fprints(f, "    }\n");
  fprints(f, "    else\n");
  fprints(f, "    {\n");
  fprints(f, "      if (ctx->last_accept != LEXER_UINT_MAX)\n");
  fprints(f, "      {\n");
  fprintf(f, "#if %s_BACKTRACKLEN_PLUS_1 > 1\n", parserupper);
  fprints(f, "        if (ctx->backtrackmid != ctx->backtrackend)\n");
  fprints(f, "        {\n");
  fprints(f, "          abort();\n");
  fprints(f, "        }\n");
  fprints(f, "        ctx->backtrack[ctx->backtrackend++] = ubuf[i]; // FIXME correct?\n");
  fprintf(f, "        if (ctx->backtrackend >= %s_BACKTRACKLEN_PLUS_1)\n", parserupper);
  fprints(f, "        {\n");
  fprints(f, "          ctx->backtrackend = 0;\n");
  fprints(f, "        }\n");
  fprints(f, "        ctx->backtrackmid = ctx->backtrackend;\n");
  fprints(f, "        if (ctx->backtrackstart == ctx->backtrackend)\n");
  fprints(f, "        {\n");
  fprints(f, "          abort();\n");
  fprints(f, "        }\n");
  fprints(f, "#else\n");
  fprints(f, "        abort();\n");
  fprints(f, "#endif\n");
  fprints(f, "      }\n");
  fprints(f, "    }\n");
  fprints(f, "  }\n"); // FIXME use taintid set:
  fprints(f, "  if (st && cb2)\n");
  fprints(f, "  {\n");
  fprints(f, "    uint64_t cbmask = 0;\n"); // FIXME may need a bigger one
  fprintf(f, "    enum yale_flags flags = 0;\n");
  fprints(f, "    size_t cbidx, taintidx;\n");
  fprints(f, "    uint16_t bitoff;\n");
  fprintf(f, "    ssize_t cbr;\n");
  fprintf(f, "    if (start)\n");
  fprintf(f, "    {\n");
  fprintf(f, "      flags |= YALE_FLAG_START;\n");
  fprintf(f, "    }\n");
  fprints(f, "    for (taintidx = 0; taintidx < st->taintidsz; taintidx++)\n");
  fprints(f, "    {\n");
  fprints(f, "      const struct callbacks *mycb = &cb2[st->taintids[taintidx]];\n");
  fprints(f, "      for (cbidx = 0; cbidx < mycb->cbsz; cbidx++)\n");
  fprints(f, "      {\n");
  fprints(f, "        cbmask |= 1ULL<<mycb->cbs[cbidx];\n");
  fprints(f, "      }\n");
  fprints(f, "    }\n");
  if (cbssz)
  {
    fprints(f, "    for (cbidx = 0; cbidx < cbstacksz; cbidx++)\n");
    fprints(f, "    {\n");
    fprints(f, "      cbmask |= 1ULL<<cbstack[cbidx];\n");
    fprints(f, "    }\n");
  }
  fprints(f, "    for (bitoff = 0; bitoff < 64; )\n");
  fprints(f, "    {\n");
  fprints(f, "      int ffsres;\n");
  fprints(f, "      if (cbmask & (1ULL<<bitoff))\n");
  fprints(f, "      {\n");
  fprints(f, "        cbr = cbtbl[bitoff](buf, sz, flags, pctx);\n");
  fprints(f, "        if (cbr != (ssize_t)sz && cbr != -EAGAIN && cbr != -EWOULDBLOCK)\n");
  fprints(f, "        {\n");
  fprints(f, "          return cbr;");
  fprints(f, "        }\n");
  fprints(f, "      }\n");
  fprints(f, "      ffsres = ffsll(cbmask & ~((1ULL<<(bitoff+1))-1));\n");
  fprints(f, "      if (ffsres == 0)\n");
  fprints(f, "      {\n");
  fprints(f, "        bitoff = 64;\n");
  fprints(f, "      }\n");
  fprints(f, "      else\n");
  fprints(f, "      {\n");
  fprints(f, "        bitoff = ffsres - 1;\n");
  fprints(f, "      }\n");
  fprints(f, "    }\n");
  fprints(f, "  }\n");
  fprints(f, "  if (st && cb1 != PARSER_UINT_MAX)\n"); // RFE correct not to have st->accepting?
  fprints(f, "  {\n");
  fprintf(f, "    enum yale_flags flags = 0;\n");
  fprintf(f, "    ssize_t cbr;\n");
  fprintf(f, "    if (start)\n");
  fprintf(f, "    {\n");
  fprintf(f, "      flags |= YALE_FLAG_START;\n");
  fprintf(f, "    }\n");
  fprints(f, "    cbr = cbtbl[cb1](buf, sz, flags, pctx);\n");
  fprints(f, "    if (cbr != (ssize_t)sz && cbr != -EAGAIN && cbr != -EWOULDBLOCK)\n");
  fprints(f, "    {\n");
  fprints(f, "      return cbr;");
  fprints(f, "    }\n");
  fprints(f, "  }\n");
  fprints(f, "  *state = PARSER_UINT_MAX;\n");
  fprints(f, "  return -EAGAIN; // Not yet\n");
  fprints(f, "}\n");

  free(parserupper);
}

uint32_t numbers_hash(const struct bitset *numbers)
{
  return yalemurmur_buf(0x12345678U, numbers, sizeof(*numbers));
}

uint32_t numbers_hash_fn(struct yale_hash_list_node *node, void *ud)
{
  struct numbers_set *ids = YALE_CONTAINER_OF(node, struct numbers_set, node);
  return numbers_hash(&ids->numbers);
}

void numbers_sets_init(struct numbers_sets *hash, void *(*alloc)(void*,size_t), void *allocud)
{
  yale_hash_table_init(&hash->hash, 8192, numbers_hash_fn, NULL, alloc, allocud);
}

int numbers_sets_put(struct numbers_sets *hash, const struct bitset *numbers, void *(*alloc)(void*,size_t), void *allocud)
{
  uint32_t hashval = numbers_hash(numbers);
  struct yale_hash_list_node *node;
  struct numbers_set *e;
  YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&hash->hash, node, hashval)
  {
    e = YALE_CONTAINER_OF(node, struct numbers_set, node);
    if (memcmp(&e->numbers, numbers, sizeof(*numbers)) == 0)
    {
      return 0;
    }
  }
  e = alloc(allocud, sizeof(*e));
  if (e == NULL)
  {
    abort(); // FIXME error handling
  }
  memcpy(&e->numbers, numbers, sizeof(*numbers));
  yale_hash_table_add_nogrow(&hash->hash, &e->node, hashval);
  return 1;
}

void numbers_sets_emit(FILE *f, struct numbers_sets *hash, const struct bitset *numbers, void *(*alloc)(void*,size_t), void *allocud)
{
  size_t j;
  if (numbers_sets_put(hash, numbers, alloc, allocud))
  {
    fprintf(f, "static const parser_uint_t taintidsetarray");
    for (j = 0; j < YALE_UINT_MAX_LEGAL + 1; /*j++*/)
    {
      yale_uint_t wordoff = j/64;
      yale_uint_t bitoff = j%64;
      if (numbers->bitset[wordoff] & (1ULL<<bitoff))
      {
        fprintf(f, "_%d", (int)j);
      }
      if (bitoff != 63)
      {
        j = (wordoff*64) + myffsll(numbers->bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
      }
      else
      {
        j++;
      }
    }
    fprintf(f, "[] = {");
    for (j = 0; j < YALE_UINT_MAX_LEGAL + 1; /*j++*/)
    {
      yale_uint_t wordoff = j/64;
      yale_uint_t bitoff = j%64;
      if (numbers->bitset[wordoff] & (1ULL<<bitoff))
      {
        fprintf(f, "%d,", (int)j);
      }
      if (bitoff != 63)
      {
        j = (wordoff*64) + myffsll(numbers->bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
      }
      else
      {
        j++;
      }
    }
    fprintf(f, "};\n");
  }
}
