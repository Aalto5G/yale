#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <ctype.h>



struct bitset {
  uint64_t bitset[4];
};

int bitset_empty(struct bitset *a)
{
  size_t i;
  for (i = 0; i < 4; i++)
  {
    if (a->bitset[i] != 0)
    {
      return 0;
    }
  }
  return 1;
}

struct re;

struct wildcard {
};

struct emptystr {
};

struct literals {
  struct bitset bitmask;
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
  uint8_t *pick_those;
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
    struct wildcard wc;
    struct emptystr e;
    struct literals lit;
    struct concat cat;
    struct altern alt;
    struct alternmulti altmulti;
    struct star star;
  } u;
};

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
    uint8_t wordoff = i/64;
    uint8_t bitoff = i%64;
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

struct dfa_node {
  uint8_t d[256];
  uint8_t default_tr;
  uint8_t acceptid;
  uint8_t tainted:1;
  uint8_t accepting:1;
  uint8_t final:1;
  struct bitset acceptidset;
  uint64_t algo_tmp;
  size_t transitions_id;
};

struct nfa_node {
  struct bitset d[256];
  struct bitset defaults;
  struct bitset epsilon;
  uint8_t accepting:1;
  uint8_t taintid;
};

void nfa_init(struct nfa_node *n, int accepting, int taintid)
{
  memset(n, 0, sizeof(*n));
  n->accepting = !!accepting;
  n->taintid = taintid;
}

void nfa_connect(struct nfa_node *n, char ch, uint8_t node2)
{
  uint8_t wordoff = node2/64;
  uint8_t bitoff = node2%64;
  n->d[(unsigned char)ch].bitset[wordoff] |= (1ULL<<bitoff);
}

void nfa_connect_epsilon(struct nfa_node *n, uint8_t node2)
{
  uint8_t wordoff = node2/64;
  uint8_t bitoff = node2%64;
  n->epsilon.bitset[wordoff] |= (1ULL<<bitoff);
}

void nfa_connect_default(struct nfa_node *n, uint8_t node2)
{
  uint8_t wordoff = node2/64;
  uint8_t bitoff = node2%64;
  n->defaults.bitset[wordoff] |= (1ULL<<bitoff);
}

#if 0
uint8_t ffs_bitset(const struct bitset *bs)
{
  size_t i;
  for (i = 0; i < 4; i++)
  {
    ffsll(bs->bitset[i]);
  }
}
#endif

uint8_t pick_rm_first(struct bitset *bs)
{
  size_t i;
  int j;
  int ffsres;
  for (i = 0; i < 4; i++)
  {
    ffsres = ffsll(bs->bitset[i]);
    if (ffsres)
    {
      j = ffsres - 1;
      bs->bitset[i] &= ~(1ULL<<j);
      return i*64 + j;
    }
  }
  abort();
}

uint8_t has_bitset(struct bitset *bs, uint8_t bit)
{
  uint8_t wordoff = bit/64;
  uint8_t bitoff = bit%64;
  return !!(bs->bitset[wordoff] & (1ULL<<bitoff));
}

void set_bitset(struct bitset *bs, uint8_t bit)
{
  uint8_t wordoff = bit/64;
  uint8_t bitoff = bit%64;
  bs->bitset[wordoff] |= (1ULL<<bitoff);
}

void epsilonclosure(struct nfa_node *ns, struct bitset nodes,
                    struct bitset *closurep, int *tainted,
                    struct bitset *acceptidsetp)
{
  struct bitset closure = nodes;
  struct bitset taintidset = {};
  struct bitset acceptidset = {};
  uint8_t stack[256] = {};
  uint8_t nidx;
  size_t stacksz = 0;
  size_t i;
  uint16_t taintcnt = 0;
  for (i = 0; i < 256; /*i++*/)
  {
    uint8_t wordoff = i/64;
    uint8_t bitoff = i%64;
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
      i = (wordoff*64) + ffsll(nodes.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
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
    for (i = 0; i < 256; /*i++*/)
    {
      uint8_t wordoff = i/64;
      uint8_t bitoff = i%64;
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
        i = (wordoff*64) + ffsll(n->epsilon.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
      }
      else
      {
        i++;
      }
    }
  }
  for (i = 0; i < 256; /*i++*/)
  {
    uint8_t wordoff = i/64;
    uint8_t bitoff = i%64;
    if (closure.bitset[wordoff] & (1ULL<<bitoff))
    {
      struct nfa_node *n = &ns[i];
      if (n->taintid != 255)
      {
        uint8_t wordoff2 = n->taintid/64;
        uint8_t bitoff2 = n->taintid%64;
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
      i = (wordoff*64) + ffsll(closure.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
    }
    else
    {
      i++;
    }
  }
  *closurep = closure;
  *acceptidsetp = acceptidset;
  *tainted = (taintcnt > 1);
}

void dfa_init(struct dfa_node *n, int accepting, int tainted, struct bitset *acceptidset)
{
#if 0
  uint16_t i;
#endif
  memset(n, 0, sizeof(*n));
  memset(n->d, 0xff, 256*sizeof(*n->d));
#if 0
  for (i = 0; i < 256; i++)
  {
    n->d[i] = 255;
  }
#endif
  n->default_tr = 255;
  n->acceptid = 255;
  if (accepting && bitset_empty(acceptidset))
  {
    printf("Accepting yet acceptidset empty\n");
    abort();
  }
  n->accepting = !!accepting;
  n->tainted = !!tainted;
  n->acceptidset = *acceptidset;
  n->transitions_id = SIZE_MAX;
}

void dfa_init_empty(struct dfa_node *n)
{
  struct bitset acceptidset = {};
  dfa_init(n, 0, 0, &acceptidset);
}

void dfa_connect(struct dfa_node *n, char ch, uint8_t node2)
{
  if (n->d[(unsigned char)ch] != 255 || node2 == 255)
  {
    printf("node2 %u\n", (unsigned)node2);
    printf("already connected %u\n", (unsigned)n->d[(unsigned char)ch]);
    abort();
  }
  n->d[(unsigned char)ch] = node2;
}

void dfa_connect_default(struct dfa_node *n, uint8_t node2)
{
  printf("connecting default\n");
  if (n->default_tr != 255 || node2 == 255)
  {
    printf("default conflict\n");
    printf("node2 %u\n", (unsigned)node2);
    printf("already connected %u\n", (unsigned)n->default_tr);
    abort();
  }
  n->default_tr = node2;
}

void bitset_update(struct bitset *a, const struct bitset *b)
{
  size_t i;
  for (i = 0; i < 4; i++)
  {
    a->bitset[i] |= b->bitset[i];
  }
}
int bitset_equal(const struct bitset *a, const struct bitset *b)
{
  size_t i;
  for (i = 0; i < 4; i++)
  {
    if (a->bitset[i] != b->bitset[i])
    {
      return 0;
    }
  }
  return 1;
}

void
check_recurse_acceptid_is(struct dfa_node *ds, uint8_t state, uint8_t acceptid)
{
  struct bitset tovisit = {};
  struct bitset visited = {};
  size_t i;
  set_bitset(&tovisit, state);
  while (!bitset_empty(&tovisit))
  {
    uint8_t queued = pick_rm_first(&tovisit);
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
      if (node->d[i] != 255)
      {
        if (!has_bitset(&visited, node->d[i]))
        {
          set_bitset(&tovisit, node->d[i]);
        }
      }
    }
    if (node->default_tr != 255)
    {
      if (!has_bitset(&visited, node->default_tr))
      {
        set_bitset(&tovisit, node->default_tr);
      }
    }
  }
}

void
check_recurse_acceptid_is_not(struct dfa_node *ds, uint8_t state, uint8_t acceptid)
{
  struct bitset tovisit = {};
  struct bitset visited = {};
  size_t i;
  set_bitset(&tovisit, state);
  while (!bitset_empty(&tovisit))
  {
    uint8_t queued = pick_rm_first(&tovisit);
    struct dfa_node *node = &ds[queued];
    if (has_bitset(&visited, queued))
    {
      continue;
    }
    set_bitset(&visited, queued);
    if (ds[queued].accepting && ds[queued].acceptid == acceptid)
    {
      abort(); // FIXME error handling
    }
    for (i = 0; i < 256; i++)
    {
      if (node->d[i] != 255)
      {
        if (!has_bitset(&visited, node->d[i]))
        {
          set_bitset(&tovisit, node->d[i]);
        }
      }
    }
    if (node->default_tr != 255)
    {
      if (!has_bitset(&visited, node->default_tr))
      {
        set_bitset(&tovisit, node->default_tr);
      }
    }
  }
}

void check_cb_first(struct dfa_node *ds, uint8_t acceptid, uint8_t state)
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

void check_cb(struct dfa_node *ds, uint8_t state, uint8_t acceptid)
{
  size_t i;
  for (i = 0; i < 256; i++)
  {
    if (ds[state].d[i] != 255)
    {
      check_cb_first(ds, acceptid, ds[state].d[i]);
    }
  }
  if (ds[state].default_tr != 255)
  {
    check_cb_first(ds, acceptid, ds[state].default_tr);
  }
}

struct bitset_hash_item {
  struct bitset key;
  uint8_t dfanodeid;
};

struct bitset_hash {
  struct bitset_hash_item tbl[255];
  uint8_t tblsz;
};

// FIXME this algorithm requires thorough review
ssize_t state_backtrack(struct dfa_node *ds, uint8_t state, size_t bound)
{
  struct bitset tovisit = {};
  struct bitset visited = {};
  size_t max_backtrack = 0;
  size_t i;
  ds[state].algo_tmp = 0;
  set_bitset(&tovisit, state);
  while (!bitset_empty(&tovisit))
  {
    uint8_t queued = pick_rm_first(&tovisit);
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
      if (node->d[i] != 255)
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
    if (node->default_tr != 255)
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

void __attribute__((noinline)) set_accepting(struct dfa_node *ds, uint8_t state, int *priorities)
{
  struct bitset tovisit = {};
  struct bitset visited = {};
  size_t i;
  int seen = 0;
  int priority;
  size_t count_thisprio = 0;
  uint8_t acceptid = 0;
  uint8_t wordoff, bitoff;
  set_bitset(&tovisit, state);
  while (!bitset_empty(&tovisit))
  {
    uint8_t queued = pick_rm_first(&tovisit);
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
      for (i = 0; i < 256; /*i++*/)
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
          i = (wordoff*64) + ffsll(ds[queued].acceptidset.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
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
      if (ds[queued].d[i] != 255)
      {
        if (!has_bitset(&visited, ds[queued].d[i]))
        {
          set_bitset(&tovisit, ds[queued].d[i]);
        }
      }
    }
    if (ds[queued].default_tr != 255)
    {
      if (!has_bitset(&visited, ds[queued].default_tr))
      {
        set_bitset(&tovisit, ds[queued].default_tr);
      }
    }
  }
}

ssize_t maximal_backtrack(struct dfa_node *ds, uint8_t state, size_t bound)
{
  struct bitset tovisit = {};
  struct bitset visited = {};
  ssize_t state_bt;
  ssize_t max_backtrack = 0;
  size_t i;
  set_bitset(&tovisit, state);
  while (!bitset_empty(&tovisit))
  {
    uint8_t queued = pick_rm_first(&tovisit);
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
      if (ds[queued].d[i] != 255)
      {
        if (!has_bitset(&visited, ds[queued].d[i]))
        {
          set_bitset(&tovisit, ds[queued].d[i]);
        }
      }
    }
    if (ds[queued].default_tr != 255)
    {
      if (!has_bitset(&visited, ds[queued].default_tr))
      {
        set_bitset(&tovisit, ds[queued].default_tr);
      }
    }
  }
  return max_backtrack;
}


void dfaviz(struct dfa_node *ds, uint8_t cnt)
{
  uint8_t i;
  uint16_t j;
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
      if (ds[i].d[j] != 255)
      {
        printf("n%d -> n%d [label=\"%c\"];\n", i, ds[i].d[j], (unsigned char)j);
      }
    }
  }
  printf("}\n");
}

void nfaviz(struct nfa_node *ns, uint8_t cnt)
{
  uint8_t i;
  uint16_t j;
  uint16_t k;
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
      for (k = 0; k < 256; k++)
      {
        uint8_t wordoff = k/64;
        uint8_t bitoff = k%64;
        if (bs->bitset[wordoff] & (1ULL<<bitoff))
        {
          printf("n%d -> n%d [label=\"%c\"];\n", i, k, (unsigned char)j);
        }
      }
    }
  }
  printf("}\n");
}

uint8_t nfa2dfa(struct nfa_node *ns, struct dfa_node *ds, uint8_t begin)
{
  struct bitset initial = {};
  struct bitset dfabegin = {};
  int tainted;
  struct bitset acceptidset = {};
  struct bitset defaults = {};
  const struct bitset defaults_empty = {};
  uint8_t wordoff = begin/64;
  uint8_t bitoff = begin%64;
  uint8_t curdfanode = 0;
  uint8_t dfanodeid, dfanodeid2;
  int accepting = 0;
  struct bitset queue[256];
  size_t queuesz;
  struct bitset_hash d = {};
  uint16_t i, j;

  initial.bitset[wordoff] |= (1ULL<<bitoff);

  epsilonclosure(ns, initial, &dfabegin, &tainted, &acceptidset);
  for (i = 0; i < 256; /*i++*/)
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
      i = (wordoff*64) + ffsll(dfabegin.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
    }
    else
    {
      i++;
    }
  }

  d.tbl[d.tblsz].dfanodeid = curdfanode;
  memcpy(&d.tbl[d.tblsz++].key, &dfabegin, sizeof(dfabegin));
  dfa_init(&ds[curdfanode], accepting, tainted, &acceptidset);
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
    for (i = 0; i < 256; /*i++*/) // for nn in nns
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
        i = (wordoff*64) + ffsll(nns.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
      }
      else
      {
        i++;
      }
    }
    epsilonclosure(ns, defaults, &defaultsec, &tainted, &acceptidset);
    if (!bitset_empty(&defaultsec))
    {
      dfanodeid = 255;
      for (i = 0; i < d.tblsz; i++)
      {
        if (bitset_equal(&d.tbl[i].key, &defaultsec))
        {
          dfanodeid = d.tbl[i].dfanodeid;
          break;
        }
      }
      if (dfanodeid == 255)
      {
        accepting = 0;
        for (i = 0; i < 256; /*i++*/)
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
            i = (wordoff*64) + ffsll(defaultsec.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
          }
          else
          {
            i++;
          }
        }
        if (curdfanode >= 255)
        {
          abort();
        }
        d.tbl[d.tblsz].dfanodeid = curdfanode;
        memcpy(&d.tbl[d.tblsz++].key, &defaultsec, sizeof(defaultsec));
        dfa_init(&ds[curdfanode], accepting, tainted, &acceptidset);
        dfanodeid = curdfanode++;
        if (queuesz >= sizeof(queue)/sizeof(*queue))
        {
          abort();
        }
        queue[queuesz++] = defaultsec;
      }

      dfanodeid2 = 255;
      for (i = 0; i < d.tblsz; i++)
      {
        if (bitset_equal(&d.tbl[i].key, &nns))
        {
          dfanodeid2 = d.tbl[i].dfanodeid;
          break;
        }
      }
      if (dfanodeid2 == 255)
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
      epsilonclosure(ns, d2[i], &ec, &tainted, &acceptidset);

      dfanodeid = 255;
      for (j = 0; j < d.tblsz; j++)
      {
        if (bitset_equal(&d.tbl[j].key, &ec))
        {
          dfanodeid = d.tbl[j].dfanodeid;
          break;
        }
      }
      if (dfanodeid == 255)
      {
        accepting = 0;
        for (j = 0; j < 256; /*j++*/)
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
            j = (wordoff*64) + ffsll(ec.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
          }
          else
          {
            j++;
          }
        }
        if (curdfanode >= 255)
        {
          abort();
        }
        d.tbl[d.tblsz].dfanodeid = curdfanode;
        memcpy(&d.tbl[d.tblsz++].key, &ec, sizeof(ec));
        dfa_init(&ds[curdfanode], accepting, tainted, &acceptidset);
        dfanodeid = curdfanode++;
        if (queuesz >= sizeof(queue)/sizeof(*queue))
        {
          abort();
        }
        queue[queuesz++] = ec;
      }

      dfanodeid2 = 255;
      for (j = 0; j < d.tblsz; j++)
      {
        if (bitset_equal(&d.tbl[j].key, &nns))
        {
          dfanodeid2 = d.tbl[j].dfanodeid;
          break;
        }
      }
      if (dfanodeid2 == 255)
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
      if (ds[i].d[j] != 255)
      {
        break;
      }
    }
    if (j == 256 && ds[i].default_tr == 255)
    {
      ds[i].final = 1;
    }
  }
  return curdfanode;
}

struct re *parse_re(const char *re, size_t resz, size_t *remainderstart);

static inline void set_char(uint64_t bitmask[4], unsigned char ch)
{
  uint8_t wordoff = ch/64;
  uint8_t bitoff = ch%64;
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
    uint8_t wordoff = i/64;
    uint8_t bitoff = i%64;
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
    uint8_t wordoff = uch/64;
    uint8_t bitoff = uch%64;
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

struct re *parse_res(struct iovec *regexps, uint8_t *pick_those, size_t resz)
{
  struct re **res;
  struct re *result;
  size_t i;
  uint8_t *pick_those2 = malloc(sizeof(*pick_those2)*resz);
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
            struct nfa_node *ns, uint8_t *ncnt,
            uint8_t begin, uint8_t end,
            uint8_t taintid)
{
  switch (regexp->type)
  {
    case STAR:
    {
      uint8_t begin1 = (*ncnt)++;
      uint8_t end1 = (*ncnt)++;
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
        uint8_t wordoff = i/64;
        uint8_t bitoff = i%64;
        if (regexp->u.lit.bitmask.bitset[wordoff] & 1ULL<<bitoff)
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
      uint8_t middle = (*ncnt)++;
      nfa_init(&ns[middle], 0, taintid);
      gennfa(regexp->u.cat.re1, ns, ncnt, begin, middle, taintid);
      gennfa(regexp->u.cat.re2, ns, ncnt, middle, end, taintid);
      return;
    }
  }
}

void gennfa_main(struct re *regexp,
                 struct nfa_node *ns, uint8_t *ncnt,
                 uint8_t taintid)
{
  uint8_t begin = (*ncnt)++;
  uint8_t end = (*ncnt)++;
  nfa_init(&ns[begin], 0, 255);
  nfa_init(&ns[end], 1, 255);
  gennfa(regexp, ns, ncnt, begin, end, taintid);
}

void gennfa_alternmulti(struct re *regexp,
                        struct nfa_node *ns, uint8_t *ncnt)
{
  uint8_t begin = (*ncnt)++;
  size_t i;
  nfa_init(&ns[begin], 0, 255);
  for (i = 0; i < regexp->u.altmulti.resz; i++)
  {
    uint8_t end = (*ncnt)++;
    nfa_init(&ns[end], 1, regexp->u.altmulti.pick_those[i]);
    gennfa(regexp->u.altmulti.res[i], ns, ncnt, begin, end, regexp->u.altmulti.pick_those[i]);
  }
}

struct pick_those_struct {
  uint8_t *pick_those;
  size_t len;
  struct dfa_node *ds;
  size_t dscnt;
};

struct transitionbuf {
  uint8_t transitions[256];
};

#define MAX_TRANS 65536 // 256 automatons, 256 states per automaton

struct transitionbufs {
  struct transitionbuf all[MAX_TRANS];
  size_t cnt;
};

size_t
get_transid(const uint8_t *transitions, struct transitionbufs *bufs)
{
  size_t j;
  for (j = 0; j < bufs->cnt; j++)
  {
    if (memcmp(transitions, bufs->all[j].transitions, 256) == 0)
    {
      break;
    }
  }
  if (j == bufs->cnt)
  {
    if (j == MAX_TRANS)
    {
      abort(); // FIXME error handling
    }
    memcpy(bufs->all[j].transitions, transitions, 256);
    bufs->cnt++;
  }
  return j;
}

void
perf_trans(uint8_t *transitions, struct transitionbufs *bufs)
{
  size_t i;
  memset(bufs, 0, sizeof(*bufs));
  for (i = 0; i < MAX_TRANS; i++)
  {
    transitions[0] = i&256;
    transitions[1] = i>>8;
    get_transid(transitions, bufs);
  }
}

void
pick(struct nfa_node *nsglobal, struct dfa_node *dsglobal,
     struct iovec *res, struct pick_those_struct *pick_those, int *priorities)
{
  uint8_t ncnt;
  struct re *re;
  size_t i;
  for (i = 0; i < 255; i++)
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
        struct transitionbufs *bufs)
{
  size_t i, j;
  for (i = 0; i < cnt; i++)
  {
    struct dfa_node *ds = pick_thoses[i].ds;
    uint8_t dscnt = pick_thoses[i].dscnt;
    for (j = 0; j < dscnt; j++)
    {
      ds[j].transitions_id = get_transid(ds[j].d, bufs);
    }
  }
}

int fprints(FILE *f, const char *s)
{
  return fputs(s, f);
}

void dump_headers(FILE *f, const char *parsername, size_t max_bt)
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
  fprints(f, "  uint8_t state; // 0 is initial state\n");
  fprints(f, "  uint8_t last_accept; // 255 means never accepted\n");
  fprints(f, "  uint8_t backtrackstart;\n");
  fprints(f, "  uint8_t backtrackend;\n");
  fprintf(f, "  uint8_t backtrack[%s_BACKTRACKLEN_PLUS_1];\n", parserupper);
  fprints(f, "};\n");
  fprints(f, "\n");
  fprints(f, "static inline void\n");
  fprintf(f, "%s_init_statemachine(struct %s_rectx *ctx)\n", parsername, parsername);
  fprints(f, "{\n");
  fprints(f, "  ctx->state = 0;\n");
  fprints(f, "  ctx->last_accept = 255;\n");
  fprints(f, "  ctx->backtrackstart = 0;\n");
  fprints(f, "  ctx->backtrackend = 0;\n");
  fprints(f, "}\n");
  fprints(f, "\n");
  fprints(f, "ssize_t\n");
  fprintf(f, "%s_feed_statemachine(struct %s_rectx *ctx, const struct state *stbl, const void *buf, size_t sz, uint8_t *state, void(*cbtbl[])(const char*, size_t, struct %s_parserctx*), const uint8_t *cbs, uint8_t cb1);//, void *baton);\n", parsername, parsername, parsername);
  fprints(f, "\n");
  free(parserupper);
}

void
dump_collected(FILE *f, const char *parsername, struct transitionbufs *bufs)
{
  size_t i;
  size_t j;
  fprints(f, "#ifdef SMALL_CODE\n");
  fprintf(f, "const uint8_t %s_transitiontbl[][256] = {\n", parsername);
  for (i = 0; i < bufs->cnt; i++)
  {
    fprintf(f, "{");
    for (j = 0; j < 256; j++)
    {
      if (bufs->all[i].transitions[j] == 255)
      {
        fprints(f, "255, ");
      }
      else
      {
        fprintf(f, "%d, ", (int)bufs->all[i].transitions[j]);
      }
    }
    fprints(f, "},\n");
  }
  fprints(f, "};\n");
  fprints(f, "#endif\n");
}

void
dump_one(FILE *f, const char *parsername, struct pick_those_struct *pick_those)
{
  size_t i, j;
  fprintf(f, "const struct state %s_states", parsername);
  for (i = 0; i < pick_those->len; i++)
  {
    fprintf(f, "_%d", pick_those->pick_those[i]);
  }
  fprints(f, "[] = {\n");
  for (i = 0; i < pick_those->dscnt; i++)
  {
    struct dfa_node *ds = &pick_those->ds[i];
    fprintf(f, "{ .accepting = %d, .acceptid = %d, .final = %d,\n",
               (int)ds->accepting, (int)ds->acceptid, (int)ds->final);
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
      if (ds->d[j] == 255)
      {
        fprints(f, "255, ");
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
dump_chead(FILE *f, const char *parsername)
{
  char *parserupper = strdup(parsername);
  size_t len = strlen(parsername);
  size_t i;
  for (i = 0; i < len; i++)
  {
    parserupper[i] = toupper((unsigned char)parserupper[i]);
  }
  fprints(f, "static inline int\n");
  fprintf(f, "%s_is_fastpath(const struct state *st, unsigned char uch)\n", parsername);
  fprints(f, "{\n");
  fprints(f, "  return !!(st->fastpathbitmask[uch/64] & (1ULL<<(uch%64)));\n");
  fprints(f, "}\n");
  fprints(f, "\n");
  fprints(f, "ssize_t\n");
  fprintf(f, "%s_feed_statemachine(struct %s_rectx *ctx, const struct state *stbl, const void *buf, size_t sz, uint8_t *state, void(*cbtbl[])(const char*, size_t, struct %s_parserctx*), const uint8_t *cbs, uint8_t cb1)//, void *baton)\n", parsername, parsername, parsername);
  fprints(f, "{\n");
  fprints(f, "  const unsigned char *ubuf = (unsigned char*)buf;\n");
  fprints(f, "  const struct state *st = NULL;\n");
  fprints(f, "  size_t i;\n");
  fprints(f, "  uint8_t newstate;\n");
  fprintf(f, "  struct %s_parserctx *pctx = CONTAINER_OF(ctx, struct %s_parserctx, rctx);\n", parsername, parsername);
  fprints(f, "  if (ctx->state == 255)\n");
  fprints(f, "  {\n");
  fprints(f, "    *state = 255;\n");
  fprints(f, "    return -EINVAL;\n");
  fprints(f, "  }\n");
  fprints(f, "  //printf(\"Called: %s\\n\", buf);\n");
  fprints(f, "  if (unlikely(ctx->backtrackstart != ctx->backtrackend))\n");
  fprints(f, "  {\n");
  fprints(f, "    while (ctx->backtrackstart != ctx->backtrackend)\n");
  fprints(f, "    {\n");
  fprints(f, "      st = &stbl[ctx->state];\n");
  fprints(f, "      ctx->state = st->transitions[ctx->backtrack[ctx->backtrackstart]];\n");
  fprints(f, "      if (unlikely(ctx->state == 255))\n");
  fprints(f, "      {\n");
  fprints(f, "        if (ctx->last_accept == 255)\n");
  fprints(f, "        {\n");
  fprints(f, "          *state = 255;\n");
  fprints(f, "          return -EINVAL;\n");
  fprints(f, "        }\n");
  fprints(f, "        ctx->state = ctx->last_accept;\n");
  fprints(f, "        ctx->last_accept = 255;\n");
  fprints(f, "        st = &stbl[ctx->state];\n");
  fprints(f, "        *state = st->acceptid;\n");
  fprints(f, "        ctx->state = 0;\n");
  fprints(f, "        return 0;\n");
  fprints(f, "      }\n");
  fprints(f, "      ctx->backtrackstart++;\n");
  fprintf(f, "      if (ctx->backtrackstart >= %s_BACKTRACKLEN_PLUS_1)\n", parserupper);
  fprints(f, "      {\n");
  fprints(f, "        ctx->backtrackstart = 0;\n");
  fprints(f, "      }\n");
  fprints(f, "      st = &stbl[ctx->state];\n");
  fprints(f, "      if (st->accepting)\n");
  fprints(f, "      {\n");
  fprints(f, "        if (st->final)\n");
  fprints(f, "        {\n");
  fprints(f, "          *state = st->acceptid;\n");
  fprints(f, "          ctx->state = 0;\n");
  fprints(f, "          ctx->last_accept = 255;\n");
  fprints(f, "          return 0;\n");
  fprints(f, "        }\n");
  fprints(f, "        else\n");
  fprints(f, "        {\n");
  fprints(f, "          ctx->last_accept = ctx->state; // FIXME correct?\n");
  fprints(f, "        }\n");
  fprints(f, "      }\n");
  fprints(f, "    }\n");
  fprints(f, "  }\n");
  fprints(f, "  for (i = 0; i < sz; i++)\n");
  fprints(f, "  {\n");
  fprints(f, "    st = &stbl[ctx->state];\n");
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
  fprints(f, "    newstate = st->transitions[ubuf[i]];\n");
  fprints(f, "    if (newstate != ctx->state) // Improves perf a lot\n");
  fprints(f, "    {\n");
  fprints(f, "      ctx->state = newstate;\n");
  fprints(f, "    }\n");
  fprints(f, "    //printf(\"New state: %d\\n\", ctx->state);\n");
  fprints(f, "    if (unlikely(newstate == 255)) // use newstate here, not ctx->state, faster\n");
  fprints(f, "    {\n");
  fprints(f, "      if (ctx->last_accept == 255)\n");
  fprints(f, "      {\n");
  fprints(f, "        *state = 255;\n");
  fprints(f, "        //printf(\"Error\\n\");\n");
  fprints(f, "        return -EINVAL;\n");
  fprints(f, "      }\n");
  fprints(f, "      ctx->state = ctx->last_accept;\n");
  fprints(f, "      ctx->last_accept = 255;\n");
  fprints(f, "      st = &stbl[ctx->state];\n");
  fprints(f, "      *state = st->acceptid;\n");
  fprints(f, "      ctx->state = 0;\n");
  fprints(f, "      if (cbs && st->accepting && cbs[st->acceptid] != 255)\n");
  fprints(f, "      {\n");
  fprints(f, "        cbtbl[cbs[st->acceptid]](buf, i, pctx);\n");
  fprints(f, "      }\n");
  fprints(f, "      if (cb1 != 255 && st->accepting)\n");
  fprints(f, "      {\n");
  fprints(f, "        cbtbl[cb1](buf, i, pctx);\n");
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
  fprints(f, "        ctx->last_accept = 255;\n");
  fprints(f, "        if (cbs && st->accepting && cbs[st->acceptid] != 255)\n");
  fprints(f, "        {\n");
  fprints(f, "          cbtbl[cbs[st->acceptid]](buf, i + 1, pctx);\n");
  fprints(f, "        }\n");
  fprints(f, "        if (cb1 != 255 && st->accepting)\n");
  fprints(f, "        {\n");
  fprints(f, "          cbtbl[cb1](buf, i + 1, pctx);\n");
  fprints(f, "        }\n");
  fprints(f, "        return i + 1;\n");
  fprints(f, "      }\n");
  fprints(f, "      else\n");
  fprints(f, "      {\n");
  fprints(f, "        ctx->last_accept = ctx->state; // FIXME correct?\n");
  fprints(f, "      }\n");
  fprints(f, "    }\n");
  fprints(f, "    else\n");
  fprints(f, "    {\n");
  fprints(f, "      if (ctx->last_accept != 255)\n");
  fprints(f, "      {\n");
  fprints(f, "        ctx->backtrack[ctx->backtrackstart++] = ubuf[i]; // FIXME correct?\n");
  fprintf(f, "        if (ctx->backtrackstart >= %s_BACKTRACKLEN_PLUS_1)\n", parserupper);
  fprints(f, "        {\n");
  fprints(f, "          ctx->backtrackstart = 0;\n");
  fprints(f, "        }\n");
  fprints(f, "        if (ctx->backtrackstart == ctx->backtrackend)\n");
  fprints(f, "        {\n");
  fprints(f, "          abort();\n");
  fprints(f, "        }\n");
  fprints(f, "      }\n");
  fprints(f, "    }\n");
  fprints(f, "  }\n");
  fprints(f, "  if (st && cbs && st->accepting && cbs[st->acceptid] != 255)\n");
  fprints(f, "  {\n");
  fprints(f, "    cbtbl[cbs[st->acceptid]](buf, sz, pctx);\n");
  fprints(f, "  }\n");
  fprints(f, "  if (st && cb1 != 255 && st->accepting)\n");
  fprints(f, "  {\n");
  fprints(f, "    cbtbl[cb1](buf, sz, pctx);\n");
  fprints(f, "  }\n");
  fprints(f, "  *state = 255;\n");
  fprints(f, "  return -EAGAIN; // Not yet\n");
  fprints(f, "}\n");

  free(parserupper);
}

struct nfa_node ns[255];
struct dfa_node ds[255];
struct transitionbufs bufs;

int main(int argc, char **argv)
{
  size_t i;
#if 0
  size_t j, k;
  uint8_t dscnt;
  uint8_t ncnt;
  struct re *re;
  const char *res[3] = {"ab","abcd","abce"};
  uint8_t pick_those[3] = {0,1,2};
#endif
  const char re0[] = "[Hh][Oo][Ss][Tt]";
  const char re1[] = "\r?\n";
  const char re2[] = " ";
  const char re3[] = " HTTP/[0-9]+[.][0-9]+\r?\n";
  const char re4[] = ":[ \t]*";
  const char re5[] = "[-!#$%&'*+.^_`|~0-9A-Za-z]+";
  const char re6[] = "[\t\x20-\x7E\x80-\xFF]*";
  const char re7[] = "[]:/?#@!$&'()*+,;=0-9A-Za-z._~%[-]+";
  const char re8[] = "\r?\n[\t ]+";
#if 0
  const char *http_res[] = {
    "[Hh][Oo][Ss][Tt]" ,
    "\r?\n" ,
    " " ,
    " HTTP/[0-9]+[.][0-9]+\r?\n" ,
    ":[ \t]*" ,
    "[-!#$%&'*+.^_`|~0-9A-Za-z]+" ,
    "[\t\x20-\x7E\x80-\xFF]*" ,
    "[]:/?#@!$&'()*+,;=0-9A-Za-z._~%[-]+" ,
    "\r?\n[\t ]+" ,
  };
#endif
  struct iovec http_res[] = {
    {.iov_base = (void*)re0, .iov_len = sizeof(re0)-1, },
    {.iov_base = (void*)re1, .iov_len = sizeof(re1)-1, },
    {.iov_base = (void*)re2, .iov_len = sizeof(re2)-1, },
    {.iov_base = (void*)re3, .iov_len = sizeof(re3)-1, },
    {.iov_base = (void*)re4, .iov_len = sizeof(re4)-1, },
    {.iov_base = (void*)re5, .iov_len = sizeof(re5)-1, },
    {.iov_base = (void*)re6, .iov_len = sizeof(re6)-1, },
    {.iov_base = (void*)re7, .iov_len = sizeof(re7)-1, },
    {.iov_base = (void*)re8, .iov_len = sizeof(re8)-1, },
  };
  int priorities[] = {
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
  };
  uint8_t pick_those0[] = {0};
  uint8_t pick_those1[] = {0,1,5};
  uint8_t pick_those2[] = {0,5};
  uint8_t pick_those3[] = {1};
  uint8_t pick_those4[] = {1,8};
  uint8_t pick_those5[] = {2};
  uint8_t pick_those6[] = {3};
  uint8_t pick_those7[] = {4};
  uint8_t pick_those8[] = {5};
  uint8_t pick_those9[] = {6};
  uint8_t pick_those10[] = {7};
  uint8_t pick_those11[] = {8};
  struct pick_those_struct pick_thoses[] = {
    {.pick_those=pick_those0, .len=sizeof(pick_those0)/sizeof(*pick_those0)},
    {.pick_those=pick_those1, .len=sizeof(pick_those1)/sizeof(*pick_those1)},
    {.pick_those=pick_those2, .len=sizeof(pick_those2)/sizeof(*pick_those2)},
    {.pick_those=pick_those3, .len=sizeof(pick_those3)/sizeof(*pick_those3)},
    {.pick_those=pick_those4, .len=sizeof(pick_those4)/sizeof(*pick_those4)},
    {.pick_those=pick_those5, .len=sizeof(pick_those5)/sizeof(*pick_those5)},
    {.pick_those=pick_those6, .len=sizeof(pick_those6)/sizeof(*pick_those6)},
    {.pick_those=pick_those7, .len=sizeof(pick_those7)/sizeof(*pick_those7)},
    {.pick_those=pick_those8, .len=sizeof(pick_those8)/sizeof(*pick_those8)},
    {.pick_those=pick_those9, .len=sizeof(pick_those9)/sizeof(*pick_those9)},
    {.pick_those=pick_those10, .len=sizeof(pick_those10)/sizeof(*pick_those10)},
    {.pick_those=pick_those11, .len=sizeof(pick_those11)/sizeof(*pick_those11)},
  };
  ssize_t maxbt = 0;
  FILE *f;

  //uint8_t transitions[256] = {};
  //perf_trans(transitions, &bufs);

#if 0
  for (i = 0; i < 255; i++)
  {
    dfa_init_empty(&ds[i]);
  }

  nfa_init(&ns[0], 0, 255);
  nfa_init(&ns[1], 0, 0);
  nfa_init(&ns[2], 0, 1);
  nfa_init(&ns[3], 0, 2);
  nfa_init(&ns[4], 0, 0);
  nfa_init(&ns[5], 1, 1);
  nfa_init(&ns[6], 0, 2);
  nfa_init(&ns[7], 0, 0);
  nfa_init(&ns[8], 0, 2);
  nfa_init(&ns[9], 1, 0);
  nfa_init(&ns[10], 1, 2);

  nfa_connect(&ns[0], 'a', 1);
  nfa_connect(&ns[1], 'b', 4);
  nfa_connect(&ns[4], 'c', 7);
  nfa_connect(&ns[7], 'e', 9);

  nfa_connect(&ns[0], 'a', 2);
  nfa_connect(&ns[2], 'b', 5);

  nfa_connect(&ns[0], 'a', 3);
  nfa_connect(&ns[3], 'b', 6);
  nfa_connect(&ns[6], 'c', 8);
  nfa_connect(&ns[8], 'd', 10);

  dscnt = nfa2dfa(ns, ds, 0);
  printf("DFA state count %d\n", (int)dscnt);
  printf("\n\n\n\n");
  dfaviz(ds, dscnt);
  printf("\n\n\n\n");

  for (i = 0; i < 255; i++)
  {
    dfa_init_empty(&ds[i]);
  }

  ncnt = 0;
  const char *relit = "ab|abcd|abce";
  size_t remainderstart;
  re = parse_re(relit, strlen(relit), &remainderstart);
  gennfa_main(re, ns, &ncnt, 0);
  free_re(re);
  printf("NFA state count %d\n", (int)ncnt);
  printf("\n\n\n\n");
  nfaviz(ns, ncnt);
  printf("\n\n\n\n");
  dscnt = nfa2dfa(ns, ds, 0);
  printf("DFA state count %d\n", (int)dscnt);
  printf("\n\n\n\n");
  dfaviz(ds, dscnt);
  printf("\n\n\n\n");

  for (i = 0; i < 255; i++)
  {
    dfa_init_empty(&ds[i]);
  }

  ncnt = 0;
  re = parse_res(res, pick_those, 3);
  gennfa_alternmulti(re, ns, &ncnt);
  free_re(re);
  printf("NFA state count %d\n", (int)ncnt);
  printf("\n\n\n\n");
  nfaviz(ns, ncnt);
  printf("\n\n\n\n");
  dscnt = nfa2dfa(ns, ds, 0);
  printf("DFA state count %d\n", (int)dscnt);
  printf("\n\n\n\n");
  dfaviz(ds, dscnt);
  printf("\n\n\n\n");
#endif

#if 0
  for (k = 0; k < 100; k++)
  {
    for (j = 0; j < sizeof(pick_thoses)/sizeof(*pick_thoses); j++)
    //for (j = 0; j < 1; j++)
    {
      for (i = 0; i < 255; i++)
      {
        dfa_init_empty(&ds[i]);
      }
  
      printf("Pick those %d\n", (int)j);

      ncnt = 0;
      re = parse_res(http_res, pick_thoses[j].pick_those, pick_thoses[j].len);
      gennfa_alternmulti(re, ns, &ncnt);
      free_re(re);
      printf("NFA state count %d\n", (int)ncnt);
      //printf("\n\n\n\n");
      //nfaviz(ns, ncnt);
      //printf("\n\n\n\n");
      dscnt = nfa2dfa(ns, ds, 0);
      printf("DFA state count %d\n", (int)dscnt);
      set_accepting(ds, 0, priorities);
      //printf("\n\n\n\n");
      //dfaviz(ds, dscnt);
      //printf("\n\n\n\n");
    }
  }
#endif

  size_t outiter;
  for (outiter = 0; outiter < 100; outiter++)
  {
    for (i = 0; i < sizeof(pick_thoses)/sizeof(*pick_thoses); i++)
    {
      pick(ns, ds, http_res, &pick_thoses[i], priorities);
    }
    collect(pick_thoses, sizeof(pick_thoses)/sizeof(*pick_thoses), &bufs);
    for (i = 0; i < sizeof(pick_thoses)/sizeof(*pick_thoses); i++)
    {
      ssize_t curbt;
      curbt = maximal_backtrack(pick_thoses[i].ds, 0, 250);
      if (curbt < 0)
      {
        abort(); // FIXME error handling
      }
      if (maxbt < curbt)
      {
        maxbt = curbt;
      }
    }
  
    printf("Max backtrack %zd\n", maxbt);
    f = fopen("httpparser2.h", "w");
    fprints(f, "#ifndef _HTTPPARSER_H_\n");
    fprints(f, "#define _HTTPPARSER_H_\n");
    fprints(f, "#include \"yalecommon.h\"\n");
    dump_headers(f, "http", maxbt);
    fprints(f, "#endif\n");
    fclose(f);
    f = fopen("httpparser2.c", "w");
    fprints(f, "#include \"httpparser2.h\"\n");
    dump_chead(f, "http");
    dump_collected(f, "http", &bufs);
    for (i = 0; i < sizeof(pick_thoses)/sizeof(*pick_thoses); i++)
    {
      dump_one(f, "http", &pick_thoses[i]);
    }
    fclose(f);
  }
}
