#include "yale.h"
#include <sys/uio.h>

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

uint8_t bitset_issubset(const struct bitset *ba, const struct bitset *bb)
{
  size_t i;
  for (i = 0; i < 4; i++)
  {
    if (ba->bitset[0] & ~bb->bitset[0])
    {
      return 0;
    }
  }
  return 1;
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

void clr_bitset(struct bitset *bs, uint8_t bit)
{
  uint8_t wordoff = bit/64;
  uint8_t bitoff = bit%64;
  bs->bitset[wordoff] &= ~(1ULL<<bitoff);
}

struct dict {
  struct bitset bitset[256];
  struct bitset has;
};

void firstset_update(struct dict *da, struct dict *db)
{
  size_t i;
  for (i = 0; i < 256; i++)
  {
    bitset_update(&da->bitset[i], &db->bitset[i]);
  }
  bitset_update(&da->has, &db->has);
}

void firstset_update_one(struct dict *da, struct dict *db, uint8_t one)
{
  if (!has_bitset(&db->has, one))
  {
    abort();
  }
  set_bitset(&da->has, one);
  bitset_update(&da->bitset[one], &db->bitset[one]);
}

void firstset_settoken(struct dict *da, const struct ruleitem *ri)
{
  if (ri->is_action)
  {
    abort();
  }
  if (ri->cb != 255)
  {
    set_bitset(&da->bitset[ri->value], ri->cb);
  }
  set_bitset(&da->has, ri->value);
}

void firstset_setsingleton(struct dict *da, struct ruleitem *ri)
{
  memset(da, 0, sizeof(*da));
  firstset_settoken(da, ri);
}

int firstset_issubset(struct dict *da, struct dict *db)
{
  size_t i;
  for (i = 0; i < 256; /*i++*/)
  {
    uint8_t wordoff = i/64;
    uint8_t bitoff = i%64;
    if (da->has.bitset[wordoff] & (1ULL<<bitoff))
    {
      if (!(db->has.bitset[wordoff] & (1ULL<<bitoff)))
      {
        return 0;
      }
      if (!bitset_issubset(&da->bitset[i], &db->bitset[i]))
      {
        return 0;
      }
    }
    if (bitoff != 63)
    { 
      i = (wordoff*64) + ffsll(da->has.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
    }
    else
    {
      i++;
    }
  }
  return 1;
}

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

uint8_t get_sole_cb(struct dict *d, uint8_t x)
{
  uint8_t cb = 255;
  int seen = 0;
  size_t i;
  for (i = 0; i < 256; /*i++*/)
  {
    uint8_t wordoff = i/64;
    uint8_t bitoff = i%64;
    if (d->bitset[x].bitset[wordoff] & (1ULL<<bitoff))
    {
      if (seen)
      {
        abort(); // FIXME error handling
      }
      seen = 1;
      cb = i;
    }
    if (bitoff != 63)
    {
      i = (wordoff*64) + ffsll(d->bitset[x].bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
    }
    else
    {
      i++;
    }
  }
  if (seen && cb == 255)
  {
    abort();
  }
  return seen ? cb : 255;
}

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
  uint8_t max_stack_size;
  struct REGen re_gen;
  struct LookupTblEntry T[255][255]; // val==255: invalid, cb==255: no callback
  struct dict Fo[256]; // 2 MB
  struct firstset_entry Fi[8192]; // 66 MB
  size_t Ficnt;
};

void parsergen_init(struct ParserGen *gen, char *parsername)
{
  gen->tokencnt = 0;
  gen->nonterminalcnt = 0;
  gen->parsername = strdup(parsername);
  gen->state_include_str = NULL;
  gen->start_state = 255;
  gen->epsilon = 255;
  gen->rulecnt = 0;
  gen->cbcnt = 0;
  gen->tokens_finalized = 0;
  gen->max_stack_size = 0;
  memset(&gen->re_gen, 0, sizeof(gen->re_gen));
  memset(gen->T, 0xff, sizeof(gen->T));
  memset(gen->Fo, 0, sizeof(gen->Fo));
  gen->Ficnt = 0;
  // leave gen->Fi purposefully uninitiailized as it's 66 MB
}

struct firstset_entry *firstset_lookup(struct ParserGen *gen, const uint8_t *rhs, size_t rhssz)
{
  size_t i;
  for (i = 0; i < gen->Ficnt; i++)
  {
    if (gen->Fi[i].rhssz != rhssz)
    {
      continue;
    }
    if (memcmp(gen->Fi[i].rhs, rhs, rhssz*sizeof(*rhs)) != 0)
    {
      continue;
    }
    return &gen->Fi[i];
  }
  return NULL;
}

void firstset_setdefault(struct ParserGen *gen, const uint8_t *rhs, size_t rhssz)
{
  if (firstset_lookup(gen, rhs, rhssz) != NULL)
  {
    return;
  }
  gen->Fi[gen->Ficnt].rhssz = rhssz;
  memcpy(gen->Fi[gen->Ficnt].rhs, rhs, rhssz*sizeof(*rhs));
  memset(&gen->Fi[gen->Ficnt].dict, 0, sizeof(gen->Fi[gen->Ficnt].dict));
  gen->Ficnt++;
}

int parsergen_is_rhs_terminal(struct ParserGen *gen, const struct ruleitem *rhs);

struct firstset_entry firstset_func(struct ParserGen *gen, const struct ruleitem *rhs, size_t rhssz)
{
  struct firstset_entry result = {};
  if (rhssz == 0)
  {
    struct ruleitem ri = {};
    ri.value = gen->epsilon;
    firstset_settoken(&result.dict, &ri);
    return result;
  }
  if (parsergen_is_rhs_terminal(gen, &rhs[0]))
  {
    firstset_settoken(&result.dict, &rhs[0]);
    return result;
  }
  result = *firstset_lookup(gen, &rhs[0].value, 1);
  if (!has_bitset(&result.dict.has, gen->epsilon))
  {
    return result;
  }
  else
  {
    struct firstset_entry result2 = {};
    clr_bitset(&result.dict.has, gen->epsilon);
    result2 = firstset_func(gen, rhs+1, rhssz-1);
    firstset_update(&result.dict, &result2.dict);
    return result;
  }
}

int parsergen_is_terminal(struct ParserGen *gen, uint8_t x);

void gen_parser(struct ParserGen *gen)
{
  int changed;
  size_t i, j;
  for (i = 0; i < gen->tokencnt; i++)
  {
    gen->Fi[i].rhssz = 1;
    gen->Fi[i].rhs[0] = i;
    memset(&gen->Fi[i].dict, 0, sizeof(gen->Fi[i].dict));
  }
  changed = 1;
  while (changed)
  {
    changed = 0;
    for (i = 0; i < gen->rulecnt; i++)
    {
      struct firstset_entry fs =
        firstset_func(gen, gen->rules[i].rhsnoact, gen->rules[i].noactcnt);
      uint8_t rhs[gen->rules[i].noactcnt];
      for (j = 0; j < gen->rules[i].noactcnt; j++)
      {
        rhs[j] = gen->rules[i].rhsnoact[j].value;
      }
      firstset_setdefault(gen, rhs, gen->rules[i].noactcnt);
      if (firstset_issubset(&fs.dict, &firstset_lookup(gen, rhs, gen->rules[i].noactcnt)->dict))
      {
        continue;
      }
      firstset_update(&firstset_lookup(gen, rhs, gen->rules[i].noactcnt)->dict, &fs.dict);
      changed = 1;
    }
    for (i = 0; i < gen->rulecnt; i++)
    {
      uint8_t nonterminal = gen->rules[i].lhs;
      uint8_t rhs[gen->rules[i].noactcnt];
      for (j = 0; j < gen->rules[i].noactcnt; j++)
      {
        rhs[j] = gen->rules[i].rhsnoact[j].value;
      }
      if (firstset_issubset(&firstset_lookup(gen, rhs, gen->rules[i].noactcnt)->dict, &firstset_lookup(gen, &nonterminal, 1)->dict))
      {
        continue;
      }
      firstset_update(&firstset_lookup(gen, &nonterminal, 1)->dict, &firstset_lookup(gen, rhs, gen->rules[i].noactcnt)->dict);
      changed = 1;
    }
  }
  changed = 1;
  while (changed)
  {
    changed = 0;
    for (i = 0; i < gen->rulecnt; i++)
    {
      uint8_t nonterminal = gen->rules[i].lhs;
      uint8_t rhs[gen->rules[i].noactcnt];
      for (j = 0; j < gen->rules[i].noactcnt; j++)
      {
        rhs[j] = gen->rules[i].rhsnoact[j].value;
      }
      for (j = 0; j < gen->rules[i].noactcnt; j++)
      {
        uint8_t rhsmid = rhs[j];
        struct firstset_entry firstrhsright;
        uint8_t terminal;
        if (parsergen_is_terminal(gen, rhsmid))
        {
          continue;
        }
        firstrhsright =
          firstset_func(gen, &gen->rules[i].rhsnoact[j+1], gen->rules[i].noactcnt - j - 1);
        for (terminal = 0; terminal < gen->tokencnt; terminal++)
        {
          if (has_bitset(&firstrhsright.dict.has, terminal))
          {
            if (!has_bitset(&gen->Fo[rhsmid].has, terminal))
            {
              changed = 1;
              firstset_update_one(&gen->Fo[rhsmid], &firstrhsright.dict, terminal);
            }
          }
        }
        if (has_bitset(&firstrhsright.dict.has, gen->epsilon))
        {
          if (!firstset_issubset(&gen->Fo[nonterminal], &gen->Fo[rhsmid]))
          {
            changed = 1;
            firstset_update(&gen->Fo[rhsmid], &gen->Fo[nonterminal]);
          }
        }
        if (j == gen->rules[i].noactcnt - 1)
        {
          if (!firstset_issubset(&gen->Fo[nonterminal], &gen->Fo[rhsmid]))
          {
            changed = 1;
           firstset_update(&gen->Fo[rhsmid], &gen->Fo[nonterminal]);
          }
        }
      }
    }
  }
  for (i = 0; i < gen->rulecnt; i++)
  {
    uint8_t rhs[gen->rules[i].noactcnt];
    uint8_t a;
    uint8_t A = gen->rules[i].lhs;
    for (j = 0; j < gen->rules[i].noactcnt; j++)
    {
      rhs[j] = gen->rules[i].rhsnoact[j].value;
    }
    for (a = 0; a < gen->tokencnt; a++)
    {
      struct firstset_entry *fi = firstset_lookup(gen, rhs, gen->rules[i].noactcnt);
      struct dict *fo = &gen->Fo[a];
      if (has_bitset(&fi->dict.has, a))
      {
        if (gen->T[A][a].val != 255 || gen->T[A][a].cb != 255)
        {
          abort();
        }
        gen->T[A][a].val = i;
        gen->T[A][a].cb = get_sole_cb(&fi->dict, a);
      }
      if (has_bitset(&fi->dict.has, gen->epsilon) && has_bitset(&fo->has, a))
      {
        if (gen->T[A][a].val != 255 || gen->T[A][a].cb != 255)
        {
          abort();
        }
        gen->T[A][a].val = i;
        gen->T[A][a].cb = get_sole_cb(fo, a);
      }
    }
  }
}

void parsergen_state_include(struct ParserGen *gen, char *stateinclude)
{
  if (gen->state_include_str != NULL || stateinclude == NULL)
  {
    abort();
  }
  gen->state_include_str = strdup(stateinclude);
}

void parsergen_set_start_state(struct ParserGen *gen, uint8_t start_state)
{
  if (gen->start_state != 255 || start_state == 255)
  {
    abort();
  }
  gen->start_state = start_state;
}

void *memdup(void *base, size_t sz)
{
  void *result = malloc(sz);
  memcpy(result, base, sz);
  return result;
}

uint8_t parsergen_add_token(struct ParserGen *gen, char *re, size_t resz, int prio)
{
  if (gen->tokens_finalized)
  {
    abort();
  }
  if (gen->tokencnt >= 255)
  {
    abort();
  }
  gen->re_by_idx[gen->tokencnt].iov_base = memdup(re, resz);
  gen->re_by_idx[gen->tokencnt].iov_len = resz;
  gen->priorities[gen->tokencnt] = prio;
  return gen->tokencnt++;
}

void parsergen_finalize_tokens(struct ParserGen *gen)
{
  if (gen->tokens_finalized)
  {
    abort();
  }
  gen->tokens_finalized = 1;
}

uint8_t parsergen_add_nonterminal(struct ParserGen *gen)
{
  if (!gen->tokens_finalized)
  {
    abort();
  }
  if (gen->tokencnt + gen->nonterminalcnt >= 255)
  {
    abort();
  }
  return gen->nonterminalcnt++;
}

int parsergen_is_terminal(struct ParserGen *gen, uint8_t x)
{
  if (!gen->tokens_finalized)
  {
    abort();
  }
  return x < gen->tokencnt;
}

int parsergen_is_rhs_terminal(struct ParserGen *gen, const struct ruleitem *rhs)
{
  if (!gen->tokens_finalized)
  {
    abort();
  }
  if (rhs->is_action)
  {
    return 0;
  }
  return parsergen_is_terminal(gen, rhs->value);
}

void parsergen_set_rules(struct ParserGen *gen, const struct rule *rules, uint8_t rulecnt, const struct namespaceitem *ns)
{
  uint8_t i;
  uint8_t j;
  gen->rulecnt = rulecnt;
  for (i = 0; i < rulecnt; i++)
  {
    gen->rules[i] = rules[i];
    for (j = 0; j < gen->rules[i].itemcnt; j++)
    {
      gen->rules[i].rhs[j].value = ns[gen->rules[i].rhs[j].value].val;
    }
    for (j = 0; j < gen->rules[i].noactcnt; j++)
    {
      gen->rules[i].rhsnoact[j].value = ns[gen->rules[i].rhsnoact[j].value].val;
    }
  }
}

void parsergen_set_cb(struct ParserGen *gen, const struct cb *cbs, uint8_t cbcnt)
{
  uint8_t i;
  gen->cbcnt = cbcnt;
  for (i = 0; i < cbcnt; i++)
  {
    gen->cbs[i] = cbs[i];
  }
}
