#define _GNU_SOURCE
#include "yale.h"
#include "parser.h"
#include "yalecontainerof.h"
#include "yalemurmur.h"
#include <sys/uio.h>

#undef DO_PRINT_STACKCONFIG

void *parsergen_alloc(struct ParserGen *gen, size_t sz)
{
  void *result = gen->userareaptr;
  size_t alloc_sz = (sz+7)/8 * 8;
  if (gen->userareaptr + alloc_sz - gen->userarea > (ssize_t)sizeof(gen->userarea))
  {
    return NULL;
  }
  gen->userareaptr += alloc_sz;
  return result;
}

void *parsergen_alloc_fn(void *gen, size_t sz)
{
  return parsergen_alloc(gen, sz);
}

static int fprints(FILE *f, const char *s)
{
  return fputs(s, f);
}

void firstset_update(struct dict *da, struct dict *db)
{
  size_t i;
  for (i = 0; i < YALE_UINT_MAX_LEGAL + 1; i++)
  {
    bitset_update(&da->bitset[i], &db->bitset[i]);
  }
  bitset_update(&da->has, &db->has);
}

void firstset_update_one(struct dict *da, struct dict *db, yale_uint_t one)
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
  if (ri->cb != YALE_UINT_MAX_LEGAL)
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
  for (i = 0; i < YALE_UINT_MAX_LEGAL + 1; /*i++*/)
  {
    yale_uint_t wordoff = i/64;
    yale_uint_t bitoff = i%64;
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
      i = (wordoff*64) + myffsll(da->has.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
    }
    else
    {
      i++;
    }
  }
  return 1;
}

yale_uint_t get_sole_cb(struct dict *d, yale_uint_t x)
{
  yale_uint_t cb = YALE_UINT_MAX_LEGAL;
  int seen = 0;
  size_t i;
  for (i = 0; i < YALE_UINT_MAX_LEGAL + 1; /*i++*/)
  {
    yale_uint_t wordoff = i/64;
    yale_uint_t bitoff = i%64;
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
      i = (wordoff*64) + myffsll(d->bitset[x].bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
    }
    else
    {
      i++;
    }
  }
  if (seen && cb == YALE_UINT_MAX_LEGAL)
  {
    abort();
  }
  return seen ? cb : YALE_UINT_MAX_LEGAL;
}

uint32_t stack_hash(const yale_uint_t *stack, yale_uint_t sz)
{
  return yalemurmur_buf(0x12345678U, stack, sz);
}

uint32_t stack_hash_fn(struct yale_hash_list_node *node, void *ud)
{
  struct stackconfig *cfg = YALE_CONTAINER_OF(node, struct stackconfig, node);
  return stack_hash(cfg->stack, cfg->sz);
}

uint32_t lookuptbl_hash(yale_uint_t nonterminal, yale_uint_t terminal)
{
  struct yalemurmurctx ctx = YALEMURMURCTX_INITER(0x12345678U);
  yalemurmurctx_feed32(&ctx, nonterminal);
  yalemurmurctx_feed32(&ctx, terminal);
  return yalemurmurctx_get(&ctx);
}

uint32_t lookuptbl_hash_fn(struct yale_hash_list_node *node, void *ud)
{
  struct LookupTblEntry *e = YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
  return lookuptbl_hash(e->nonterminal, e->terminal);
}

void lookuptbl_put(struct ParserGen *gen,
                   yale_uint_t nonterminal, yale_uint_t terminal,
                   yale_uint_t cond,
                   yale_uint_t val, yale_uint_t cb)
{
  uint32_t hashval = lookuptbl_hash(nonterminal, terminal);
  struct yale_hash_list_node *node;
  struct LookupTblEntry *e;
  yale_uint_t i;
  YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
  {
    e = YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
    if (e->nonterminal == nonterminal && e->terminal == terminal &&
        e->cond == cond)
    {
      printf("conflict\n");
      abort(); // FIXME error handling
    }
  }
  if (gen->Tcnt >= sizeof(gen->Tentries)/sizeof(*gen->Tentries))
  {
    abort(); // FIXME error handling
  }
  e = &gen->Tentries[gen->Tcnt++];
  e->nonterminal = nonterminal;
  e->terminal = terminal;
  e->cond = cond;
  e->val = val;
  e->cb = cb;
  yale_hash_table_add_nogrow(&gen->Thash, &e->node, hashval);
  for (i = 0; i < gen->nonterminal_conds[nonterminal].condcnt; i++)
  {
    if (gen->nonterminal_conds[nonterminal].conds[i].cond == cond)
    {
      return;
    }
  }
  if (i >= sizeof(gen->nonterminal_conds[nonterminal].conds) /
           sizeof(*gen->nonterminal_conds[nonterminal].conds))
  {
    abort(); // FIXME error handling
  }
  gen->nonterminal_conds[nonterminal].conds[i].cond = cond;
  gen->nonterminal_conds[nonterminal].condcnt++;
}

uint32_t firstset_hash(const yale_uint_t *rhs, size_t sz)
{
  return yalemurmur_buf(0x12345678U, rhs, sz);
}

uint32_t firstset2_hash(const struct ruleitem *rhs, size_t sz)
{
  struct yalemurmurctx ctx = YALEMURMURCTX_INITER(0x12345678U);
  size_t i;
  for (i = 0; i < sz; i++)
  {
    yalemurmurctx_feed32(&ctx, (rhs[i].is_action<<1) | rhs[i].is_bytes);
    yalemurmurctx_feed32(&ctx, rhs[i].value);
    yalemurmurctx_feed32(&ctx, rhs[i].cb);
  }
  return yalemurmurctx_get(&ctx);
}

uint32_t firstset_hash_fn(struct yale_hash_list_node *node, void *ud)
{
  struct firstset_entry *cfg = YALE_CONTAINER_OF(node, struct firstset_entry, node);
  return firstset_hash(cfg->rhs, cfg->rhssz);
}

uint32_t firstset2_hash_fn(struct yale_hash_list_node *node, void *ud)
{
  struct firstset_entry2 *cfg = YALE_CONTAINER_OF(node, struct firstset_entry2, node);
  return firstset2_hash(cfg->rhs, cfg->rhssz);
}

void parsergen_init(struct ParserGen *gen, char *parsername)
{
#if 1
  size_t i;
#if 0
  size_t j;
#endif
#endif
  gen->userareaptr = gen->userarea;
  gen->tokencnt = 0;
  gen->nonterminalcnt = 0;
  gen->parsername = strdup(parsername);
  gen->state_include_str = NULL;
  gen->init_include_str = NULL;
  gen->start_state = YALE_UINT_MAX_LEGAL;
  gen->epsilon = YALE_UINT_MAX_LEGAL;
  gen->rulecnt = 0;
  gen->cbcnt = 0;
  gen->tokens_finalized = 0;
  gen->max_stack_size = 0;
  gen->max_bt = 0;
  gen->stackconfigcnt = 0;
  gen->Tcnt = 0;
  gen->condcnt = 0;
  memset(&gen->re_gen, 0, sizeof(gen->re_gen));
  memset(&gen->Fo2, 0, sizeof(gen->Fo2));
  for (i = 0; i < YALE_UINT_MAX_LEGAL; i++)
  {
    gen->nonterminal_conds[i].condcnt = 0;
  }
#if 0
#if 1
  for (i = 0; i < YALE_UINT_MAX_LEGAL; i++)
  {
    for (j = 0; j < YALE_UINT_MAX_LEGAL; j++)
    {
      gen->T[i][j].val = YALE_UINT_MAX_LEGAL;
      gen->T[i][j].cb = YALE_UINT_MAX_LEGAL;
    }
  }
#else
  memset(gen->T, 0xff, sizeof(gen->T));
#endif
#endif
  //memset(gen->Fo, 0, sizeof(gen->Fo)); // This is the overhead!
  transitionbufs_init(&gen->bufs, parsergen_alloc_fn, gen);
  gen->pick_thoses_cnt = 0;
  // leave gen->Fi purposefully uninitiailized as it's 66 MB
  yale_hash_table_init(&gen->Fi_hash, 8192, firstset_hash_fn, NULL,
                       parsergen_alloc_fn, gen);
  yale_hash_table_init(&gen->Fi2_hash, 8192, firstset2_hash_fn, NULL,
                       parsergen_alloc_fn, gen);
  yale_hash_table_init(&gen->stackconfigs_hash, 32768, stack_hash_fn, NULL,
                       parsergen_alloc_fn, gen);
  yale_hash_table_init(&gen->Thash, 32768, lookuptbl_hash_fn, NULL,
                       parsergen_alloc_fn, gen);
}

void firstset_entry2_deep_free(struct firstset_entry2 *e);

void parsergen_free(struct ParserGen *gen)
{
  size_t i;
  unsigned bucket;
  struct yale_hash_list_node *n, *x;
  for (i = 0; i < gen->tokencnt; i++)
  {
    free(gen->re_by_idx[i].iov_base);
    gen->re_by_idx[i].iov_base = NULL;
  }
  for (i = 0; i < gen->pick_thoses_cnt; i++)
  {
    free(gen->pick_thoses[i].ds);
    gen->pick_thoses[i].ds = NULL;
  }
  free(gen->state_include_str);
  free(gen->init_include_str);
  free(gen->parsername);
  transitionbufs_fini(&gen->bufs);
  YALE_HASH_TABLE_FOR_EACH_SAFE(&gen->Fi_hash, bucket, n, x)
  {
    yale_hash_table_delete(&gen->Fi_hash, n);
  }
  yale_hash_table_free(&gen->Fi_hash);
  YALE_HASH_TABLE_FOR_EACH_SAFE(&gen->Fi2_hash, bucket, n, x)
  {
    struct firstset_entry2 *e2 = YALE_CONTAINER_OF(n, struct firstset_entry2, node);
    firstset_entry2_deep_free(e2);
    yale_hash_table_delete(&gen->Fi2_hash, n);
  }
  for (i = 0; i < YALE_UINT_MAX_LEGAL + 1; i++)
  {
    firstset_values_deep_free(&gen->Fo2[i]);
  }
  yale_hash_table_free(&gen->Fi2_hash);
  YALE_HASH_TABLE_FOR_EACH_SAFE(&gen->stackconfigs_hash, bucket, n, x)
  {
    yale_hash_table_delete(&gen->stackconfigs_hash, n);
  }
  yale_hash_table_free(&gen->stackconfigs_hash);
}

struct firstset_entry *firstset_lookup_hashval(
    struct ParserGen *gen, const yale_uint_t *rhs, size_t rhssz, uint32_t hashval)
{
  struct yale_hash_list_node *node;
  YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Fi_hash, node, hashval)
  {
    struct firstset_entry *e = YALE_CONTAINER_OF(node, struct firstset_entry, node);
    if (e->rhssz != rhssz)
    {
      continue;
    }
    if (memcmp(e->rhs, rhs, rhssz*sizeof(*rhs)) != 0)
    {
      continue;
    }
    return e;
  }
  return NULL;
}

struct firstset_entry *firstset_lookup(struct ParserGen *gen, const yale_uint_t *rhs, size_t rhssz)
{
  return firstset_lookup_hashval(gen, rhs, rhssz, firstset_hash(rhs, rhssz));
}

int rhseq(const struct ruleitem *rhs1, const struct ruleitem *rhs2, size_t sz)
{
  size_t i;
  for (i = 0; i < sz; i++)
  {
    if (rhs1[i].is_action != rhs2[i].is_action ||
        rhs1[i].is_bytes != rhs2[i].is_bytes ||
        rhs1[i].value != rhs2[i].value ||
        rhs1[i].cb != rhs2[i].cb)
    {
      return 0;
    }
  }
  return 1;
}

struct firstset_entry2 *firstset2_lookup_hashval(
    struct ParserGen *gen, const struct ruleitem *rhs, size_t rhssz, uint32_t hashval)
{
  struct yale_hash_list_node *node;
  YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Fi2_hash, node, hashval)
  {
    struct firstset_entry2 *e = YALE_CONTAINER_OF(node, struct firstset_entry2, node);
    if (e->rhssz != rhssz)
    {
      continue;
    }
    if (!rhseq(e->rhs, rhs, rhssz))
    {
      continue;
    }
    return e;
  }
  return NULL;
}

struct firstset_entry2 *firstset2_lookup(struct ParserGen *gen, const struct ruleitem *rhs, size_t rhssz)
{
  return firstset2_lookup_hashval(gen, rhs, rhssz, firstset2_hash(rhs, rhssz));
}

void firstset2_setdefault(struct ParserGen *gen, const struct ruleitem *rhs, size_t rhssz)
{
  uint32_t hashval = firstset2_hash(rhs, rhssz);
  if (firstset2_lookup_hashval(gen, rhs, rhssz, hashval) != NULL)
  {
    return;
  }
  struct firstset_entry2 *Fi;
  Fi = parsergen_alloc(gen, sizeof(*Fi));
  Fi->rhs = parsergen_alloc(gen, sizeof(*Fi->rhs) * rhssz);
  Fi->rhssz = rhssz;
  memcpy(Fi->rhs, rhs, rhssz*sizeof(*rhs));
  Fi->values.values = NULL;
  Fi->values.valuessz = 0;
  yale_hash_table_add_nogrow(&gen->Fi2_hash, &Fi->node, hashval);
}

void firstset_setdefault(struct ParserGen *gen, const yale_uint_t *rhs, size_t rhssz)
{
  uint32_t hashval = firstset_hash(rhs, rhssz);
  if (firstset_lookup_hashval(gen, rhs, rhssz, hashval) != NULL)
  {
    return;
  }
  struct firstset_entry *Fi;
  Fi = parsergen_alloc(gen, sizeof(*Fi));
  Fi->rhssz = rhssz;
  memcpy(Fi->rhs, rhs, rhssz*sizeof(*rhs));
  memset(&Fi->dict, 0, sizeof(Fi->dict));
  yale_hash_table_add_nogrow(&gen->Fi_hash, &Fi->node, hashval);
}

int parsergen_is_rhs_terminal(struct ParserGen *gen, const struct ruleitem *rhs);

struct firstset_entry firstset_func(struct ParserGen *gen, const struct ruleitem *rhs, size_t rhssz)
{
  struct firstset_entry result;
  if (rhssz == 0)
  {
    struct ruleitem ri = {};
    memset(&result, 0, sizeof(result));
    ri.value = gen->epsilon;
    firstset_settoken(&result.dict, &ri);
    return result;
  }
  if (parsergen_is_rhs_terminal(gen, &rhs[0]))
  {
    memset(&result, 0, sizeof(result));
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
    struct firstset_entry result2;
    clr_bitset(&result.dict.has, gen->epsilon);
    result2 = firstset_func(gen, rhs+1, rhssz-1);
    firstset_update(&result.dict, &result2.dict);
    return result;
  }
}

struct firstset_value firstset_value_deep_copy(struct firstset_value orig)
{
  struct firstset_value result = orig;
  size_t i;
  result.cbs = malloc(sizeof(*result.cbs)*orig.cbsz);
  for (i = 0; i < orig.cbsz; i++)
  {
    result.cbs[i] = orig.cbs[i];
  }
  return result;
}

void firstset_value_deep_free(struct firstset_value *orig)
{
  free(orig->cbs);
  orig->cbs = NULL;
}

struct firstset_values firstset_values_deep_copy(struct firstset_values orig)
{
  struct firstset_values result;
  size_t i;
  result.valuessz = orig.valuessz;
  result.values = malloc(sizeof(*result.values)*orig.valuessz);
  for (i = 0; i < orig.valuessz; i++)
  {
    result.values[i] = firstset_value_deep_copy(orig.values[i]);
  }
  return result;
}

void firstset_values_deep_free(struct firstset_values *orig)
{
  size_t i;
  for (i = 0; i < orig->valuessz; i++)
  {
    firstset_value_deep_free(&orig->values[i]);
  }
  free(orig->values);
  orig->values = NULL;
}

void firstset_entry2_deep_free(struct firstset_entry2 *e)
{
  firstset_values_deep_free(&e->values);
  //free(e->rhs);
  e->rhs = NULL;
}

int cb_compar(const void *a, const void *b)
{
  const int *iap = a, *ibp = b;
  int ia = *iap, ib = *ibp;
  if (ia > ib)
  {
    return 1;
  }
  else if (ia < ib)
  {
    return -1;
  }
  return 0;
}

struct firstset_value firstset_value_deep_copy_cbadd(struct firstset_value orig, yale_uint_t cb)
{
  struct firstset_value result = orig;
  size_t i;
  result.cbsz = orig.cbsz + 1;
  result.cbs = malloc(sizeof(*result.cbs)*(orig.cbsz+1));
  for (i = 0; i < orig.cbsz; i++)
  {
    result.cbs[i] = orig.cbs[i];
  }
  result.cbs[orig.cbsz] = cb;
  qsort(result.cbs, orig.cbsz+1, sizeof(*result.cbs), cb_compar);
  return result;
}

struct firstset_values firstset_values_deep_copy_cbadd(struct firstset_values orig, yale_uint_t cb)
{
  struct firstset_values result;
  size_t i;
  result.valuessz = orig.valuessz;
  result.values = malloc(sizeof(*result.values)*orig.valuessz);
  for (i = 0; i < orig.valuessz; i++)
  {
    result.values[i] = firstset_value_deep_copy_cbadd(orig.values[i], cb);
  }
  return result;
}
int valeq(const struct firstset_value *val1, const struct firstset_value *val2)
{
  size_t i;
  if (val1->is_bytes != val2->is_bytes ||
      val1->token != val2->token ||
      val1->cbsz != val2->cbsz)
  {
    return 0;
  }
  for (i = 0; i < val1->cbsz; i++)
  {
    if (val1->cbs[i] != val2->cbs[i])
    {
      return 0;
    }
  }
  return 1;
}
int valcmp(const struct firstset_value *val1, const struct firstset_value *val2)
{
  size_t i;
  if (val1->is_bytes > val2->is_bytes)
  {
    return 1;
  }
  if (val1->is_bytes < val2->is_bytes)
  {
    return -1;
  }
  if (val1->token > val2->token)
  {
    return 1;
  }
  if (val1->token < val2->token)
  {
    return -1;
  }
  if (val1->cbsz > val2->cbsz)
  {
    return 1;
  }
  if (val1->cbsz < val2->cbsz)
  {
    return -1;
  }
  for (i = 0; i < val1->cbsz; i++)
  {
    if (val1->cbs[i] > val2->cbs[i])
    {
      return 1;
    }
    if (val1->cbs[i] < val2->cbs[i])
    {
      return -1;
    }
  }
  return 0;
}
int valcmpvoid(const void *val1, const void *val2)
{
  return valcmp(val1, val2);
}

void firstset2_update_one(struct firstset_values *val2, const struct firstset_values *val1, yale_uint_t one, int *changed)
{
  yale_uint_t val2origsz = val2->valuessz;
  yale_uint_t i = 0, j = 0;
  val2->values = realloc(val2->values, sizeof(*val2->values)*(val2origsz + val1->valuessz));
  if (changed)
  {
    *changed = 0;
  }
  while (i < val2origsz && j < val1->valuessz)
  {
    int cmpres = valcmp(&val2->values[i], &val1->values[j]);
    if (cmpres == 0)
    {
      i++;
      j++;
      continue;
    }
    if (cmpres < 0)
    {
      i++;
      continue;
    }
    if (val1->values[j].token != one)
    {
      j++;
      continue;
    }
    val2->values[val2->valuessz++] = firstset_value_deep_copy(val1->values[j]);
    j++;
    if (changed)
    {
      *changed = 1;
    }
  }
  while (j < val1->valuessz)
  {
    if (val1->values[j].token != one)
    {
      j++;
      continue;
    }
    val2->values[val2->valuessz++] = firstset_value_deep_copy(val1->values[j]);
    j++;
    if (changed)
    {
      *changed = 1;
    }
  }
  qsort(val2->values, val2->valuessz, sizeof(*val2->values), valcmpvoid);
}

void firstset2_update(struct ParserGen *gen, struct firstset_values *val2, const struct firstset_values *val1, int noepsilon, int *changed)
{
  yale_uint_t val2origsz = val2->valuessz;
  yale_uint_t i = 0, j = 0;
  val2->values = realloc(val2->values, sizeof(*val2->values)*(val2origsz + val1->valuessz));
  if (changed)
  {
    *changed = 0;
  }
  while (i < val2origsz && j < val1->valuessz)
  {
    int cmpres = valcmp(&val2->values[i], &val1->values[j]);
    if (cmpres == 0)
    {
      i++;
      j++;
      continue;
    }
    if (cmpres < 0)
    {
      i++;
      continue;
    }
    if (noepsilon && val1->values[j].token == gen->epsilon)
    {
      j++;
      continue;
    }
    val2->values[val2->valuessz++] = firstset_value_deep_copy(val1->values[j]);
    j++;
    if (changed)
    {
      *changed = 1;
    }
  }
  while (j < val1->valuessz)
  {
    if (noepsilon && val1->values[j].token == gen->epsilon)
    {
      j++;
      continue;
    }
    val2->values[val2->valuessz++] = firstset_value_deep_copy(val1->values[j]);
    j++;
    if (changed)
    {
      *changed = 1;
    }
  }
  qsort(val2->values, val2->valuessz, sizeof(*val2->values), valcmpvoid);
}
void firstset2_update_epsilon(struct firstset_values *val2, const struct firstset_values *val1)
{
  return firstset2_update(NULL, val2, val1, 0, NULL);
}
void firstset2_update_noepsilon(struct ParserGen *gen, struct firstset_values *val2, const struct firstset_values *val1)
{
  return firstset2_update(gen, val2, val1, 1, NULL);
}

struct firstset_values firstset_func2(struct ParserGen *gen, const struct ruleitem *rhs, size_t rhssz)
{
  struct firstset_entry2 *e2;
  struct ruleitem rhs0;
  size_t i;
  int found = 0;
  if (rhssz == 0)
  {
    struct firstset_values values;
    struct firstset_value *value = malloc(1*sizeof(*value));
    value[0].is_bytes = 0;
    value[0].token = gen->epsilon;
    value[0].cbsz = 0;
    value[0].cbs = NULL;
    values.values = value;
    values.valuessz = 1;
    return values;
  }
  if (parsergen_is_rhs_terminal(gen, &rhs[0]))
  {
    struct firstset_values values;
    struct firstset_value *value = malloc(1*sizeof(*value));
    value[0].is_bytes = rhs[0].is_bytes;
    value[0].token = rhs[0].value;
    if (rhs[0].cb != YALE_UINT_MAX_LEGAL)
    {
      value[0].cbsz = 1;
      value[0].cbs = malloc(1*sizeof(*value[0].cbs));
      value[0].cbs[0] = rhs[0].cb;
    }
    else
    {
      value[0].cbsz = 0;
      value[0].cbs = NULL;
    }
    values.values = value;
    values.valuessz = 1;
    return values;
  }
  rhs0.is_action = rhs[0].is_action;
  rhs0.is_bytes = rhs[0].is_bytes;
  rhs0.value = rhs[0].value;
  rhs0.cb = YALE_UINT_MAX_LEGAL;
  e2 = firstset2_lookup(gen, &rhs0, 1);
  found = 0;
  for (i = 0; i < e2->values.valuessz; i++)
  {
    if (e2->values.values[i].token == gen->epsilon)
    {
      found = 1;
      break;
    }
  }
  if (!found)
  {
    if (rhs[0].cb != YALE_UINT_MAX_LEGAL)
    {
      return firstset_values_deep_copy_cbadd(e2->values, rhs[0].cb);
    }
    else
    {
      return firstset_values_deep_copy(e2->values);
    }
  }
  else
  {
    struct firstset_values result2;
    result2 = firstset_func2(gen, rhs+1, rhssz-1);
    firstset2_update_noepsilon(gen, &result2, &e2->values);
    return result2;
  }
}

void stackconfig_print(struct ParserGen *gen, yale_uint_t scidx)
{
#ifdef DO_PRINT_STACKCONFIG
  yale_uint_t sz = gen->stackconfigs[scidx]->sz;
  yale_uint_t i;
  printf("s");
  for (i = 0; i < sz; i++)
  {
    printf("_%d", (int)gen->stackconfigs[scidx]->stack[i]);
  }
  printf(" [label=\"");
  for (i = 0; i < sz; i++)
  {
    if (i == 0)
    {
      printf("%d", (int)gen->stackconfigs[scidx]->stack[i]);
    }
    else
    {
      printf(",%d", (int)gen->stackconfigs[scidx]->stack[i]);
    }
  }
  printf("\"];\n");
#endif
}

void stacktransitionlast_print(struct ParserGen *gen, yale_uint_t scidx)
{
#ifdef DO_PRINT_STACKCONFIG
  yale_uint_t sz = gen->stackconfigs[scidx]->sz;
  yale_uint_t i;
  printf("s");
  for (i = 0; i < sz; i++)
  {
    printf("_%d", (int)gen->stackconfigs[scidx]->stack[i]);
  }
  printf(" -> s");
  for (i = 0; i < sz-1; i++)
  {
    printf("_%d", (int)gen->stackconfigs[scidx]->stack[i]);
  }
  printf(" [label=\"");
  printf("%d", (int)gen->stackconfigs[scidx]->stack[sz-1]);
  printf("\"];\n");
#endif
}

void stacktransitionarbitrary_print(struct ParserGen *gen, yale_uint_t scidx1,
                                    yale_uint_t scidx2, yale_uint_t tkn)
{
#ifdef DO_PRINT_STACKCONFIG
  yale_uint_t sz1 = gen->stackconfigs[scidx1]->sz;
  yale_uint_t sz2 = gen->stackconfigs[scidx2]->sz;
  yale_uint_t i;
  printf("s");
  for (i = 0; i < sz1; i++)
  {
    printf("_%d", (int)gen->stackconfigs[scidx1]->stack[i]);
  }
  printf(" -> s");
  for (i = 0; i < sz2; i++)
  {
    printf("_%d", (int)gen->stackconfigs[scidx2]->stack[i]);
  }
  printf(" [label=\"");
  printf("(%d)", (int)tkn);
  printf("\"];\n");
#endif
}


size_t stackconfig_append(struct ParserGen *gen, const yale_uint_t *stack, yale_uint_t sz)
{
  size_t i = gen->stackconfigcnt;
  uint32_t hashval = stack_hash(stack, sz);
  struct yale_hash_list_node *node;
  YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->stackconfigs_hash, node, hashval)
  {
    struct stackconfig *cfg = YALE_CONTAINER_OF(node, struct stackconfig, node);
    if (cfg->sz != sz)
    {
      continue;
    }
    if (memcmp(cfg->stack, stack, sz*sizeof(yale_uint_t)) != 0)
    {
      continue;
    }
    i = cfg->i;
    //printf("Found %d!\n", (int)sz);
    break;
  }
  if (i == gen->stackconfigcnt)
  {
    if (gen->stackconfigcnt >= sizeof(gen->stackconfigs)/sizeof(*gen->stackconfigs))
    {
      abort();
    }
    gen->stackconfigs[i] = parsergen_alloc(gen, sizeof(*gen->stackconfigs[i]));
    gen->stackconfigs[i]->stack = parsergen_alloc(gen, sz*sizeof(yale_uint_t));
    memcpy(gen->stackconfigs[i]->stack, stack, sz*sizeof(yale_uint_t));
    gen->stackconfigs[i]->sz = sz;
    gen->stackconfigs[i]->i = i;
    yale_hash_table_add_nogrow(&gen->stackconfigs_hash, &gen->stackconfigs[i]->node, hashval);
    gen->stackconfigcnt++;
    stackconfig_print(gen, i);
    //printf("Not found %d!\n", (int)sz);
  }
  return i;
}

int parsergen_is_terminal(struct ParserGen *gen, yale_uint_t x);


ssize_t max_stack_sz(struct ParserGen *gen)
{
  size_t maxsz = 1;
  size_t i, j;
  yale_uint_t stack[YALE_UINT_MAX_LEGAL];
  size_t sz;
  yale_uint_t a;
  gen->stackconfigcnt = 1;
  gen->stackconfigs[0] = parsergen_alloc(gen, sizeof(*gen->stackconfigs[0]));
  gen->stackconfigs[0]->stack = parsergen_alloc(gen, 1*sizeof(yale_uint_t));
  gen->stackconfigs[0]->stack[0] = gen->start_state;
  gen->stackconfigs[0]->sz = 1;
  stackconfig_print(gen, 0);
  //printf("Start state is %d, terminal? %d\n", gen->start_state, parsergen_is_terminal(gen, gen->start_state));
  for (i = 0; i < gen->stackconfigcnt; i++)
  {
    struct stackconfig *current = gen->stackconfigs[i];
    if (current->sz > maxsz)
    {
      maxsz = current->sz;
    }
    if (current->sz > 0)
    {
      yale_uint_t last = current->stack[current->sz-1];
      if (parsergen_is_terminal(gen, last) || last == YALE_UINT_MAX_LEGAL)
      {
        stackconfig_append(gen, current->stack, current->sz-1);
        stacktransitionlast_print(gen, i);
        continue;
      }
      for (a = 0; a < gen->tokencnt; a++)
      {
        uint32_t hashval = lookuptbl_hash(last, a);
        struct yale_hash_list_node *node;
        YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
        {
          yale_uint_t rule;
          struct LookupTblEntry *e =
            YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
          if (e->nonterminal != last)
          {
            continue;
          }
          if (e->terminal != a)
          {
            continue;
          }
          rule = e->val;
          if (rule != YALE_UINT_MAX_LEGAL)
          {
            //printf("Rule differs from YALE_UINT_MAX_LEGAL\n");
            memcpy(stack, current->stack, (current->sz-1)*sizeof(*stack));
            sz = current->sz - 1;
            if (sz + gen->rules[rule].itemcnt > YALE_UINT_MAX_LEGAL)
            {
              return -1;
            }
            for (j = 0; j < gen->rules[rule].itemcnt; j++)
            {
              struct ruleitem *it =
                &gen->rules[rule].rhs[gen->rules[rule].itemcnt-j-1];
              if (it->is_action)
              {
                stack[sz++] = YALE_UINT_MAX_LEGAL;
              }
              else
              {
                stack[sz++] = it->value;
              }
            }
            size_t tmp = stackconfig_append(gen, stack, sz);
            stacktransitionarbitrary_print(gen, i, tmp, a);
          }
        }
      }
      for (a = YALE_UINT_MAX_LEGAL - 1; a < YALE_UINT_MAX_LEGAL; a++)
      {
        uint32_t hashval = lookuptbl_hash(last, a);
        struct yale_hash_list_node *node;
        YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
        {
          yale_uint_t rule;
          struct LookupTblEntry *e =
            YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
          if (e->nonterminal != last)
          {
            continue;
          }
          if (e->terminal != a)
          {
            continue;
          }
          rule = e->val;
          if (rule != YALE_UINT_MAX_LEGAL)
          {
            //printf("Rule differs from YALE_UINT_MAX_LEGAL\n");
            memcpy(stack, current->stack, (current->sz-1)*sizeof(*stack));
            sz = current->sz - 1;
            if (sz + gen->rules[rule].itemcnt > YALE_UINT_MAX_LEGAL)
            {
              return -1;
            }
            for (j = 0; j < gen->rules[rule].itemcnt; j++)
            {
              struct ruleitem *it =
                &gen->rules[rule].rhs[gen->rules[rule].itemcnt-j-1];
              if (it->is_action)
              {
                stack[sz++] = YALE_UINT_MAX_LEGAL;
              }
              else
              {
                stack[sz++] = it->value;
              }
            }
            size_t tmp = stackconfig_append(gen, stack, sz);
            stacktransitionarbitrary_print(gen, i, tmp, a);
          }
        }
      }
    }
  }
  return maxsz;
}

int has_firstset(struct firstset_values *vals, yale_uint_t terminal)
{
  size_t i;
  for (i = 0; i < vals->valuessz; i++)
  {
    if (vals->values[i].token == terminal)
    {
      return 1;
    }
  }
  return 0;
}

void gen_parser(struct ParserGen *gen)
{
  int changed;
  size_t i, j;

  //memset(gen->Fo, 0, sizeof(gen->Fo[0])*(gen->tokencnt + gen->nonterminalcnt));
  for (i = gen->tokencnt; i < gen->tokencnt + gen->nonterminalcnt; i++)
  {
    gen->Fo[i] = parsergen_alloc(gen, sizeof(*gen->Fo[i]));
    memset(gen->Fo[i], 0, sizeof(*gen->Fo[i]));
  }

  for (i = gen->tokencnt; i < gen->tokencnt + gen->nonterminalcnt; i++)
  {
    yale_uint_t rhsit = i;
    uint32_t hashval = firstset_hash(&rhsit, 1);
    struct firstset_entry *Fi;
    Fi = parsergen_alloc(gen, sizeof(*Fi));
    Fi->rhssz = 1;
    Fi->rhs[0] = i;
    memset(&Fi->dict, 0, sizeof(Fi->dict));
    yale_hash_table_add_nogrow(&gen->Fi_hash, &Fi->node, hashval);
  }
  for (i = gen->tokencnt; i < gen->tokencnt + gen->nonterminalcnt; i++)
  {
    yale_uint_t rhsit = i;
    struct ruleitem rhs = {.value = rhsit, .cb = YALE_UINT_MAX_LEGAL};
    uint32_t hashval = firstset2_hash(&rhs, 1);
    struct firstset_entry2 *Fi;
    Fi = parsergen_alloc(gen, sizeof(*Fi));
    Fi->rhs = parsergen_alloc(gen, sizeof(*Fi->rhs) * 1);
    Fi->rhs[0] = rhs;
    Fi->rhssz = 1;
    Fi->values.values = NULL;
    Fi->values.valuessz = 0;
    yale_hash_table_add_nogrow(&gen->Fi2_hash, &Fi->node, hashval);
  }
  changed = 1;
  while (changed)
  {
    changed = 0;
    for (i = 0; i < gen->rulecnt; i++)
    {
      struct firstset_entry fs =
        firstset_func(gen, gen->rules[i].rhsnoact, gen->rules[i].noactcnt);
      yale_uint_t rhs[gen->rules[i].noactcnt];
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
      yale_uint_t nonterminal = gen->rules[i].lhs;
      yale_uint_t rhs[gen->rules[i].noactcnt];
      for (j = 0; j < gen->rules[i].noactcnt; j++)
      {
        rhs[j] = gen->rules[i].rhsnoact[j].value;
      }
      struct firstset_entry *fa = firstset_lookup(gen, rhs, gen->rules[i].noactcnt);
      struct firstset_entry *fb = firstset_lookup(gen, &nonterminal, 1);
      if (firstset_issubset(&fa->dict, &fb->dict))
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
      int changedtmp = 0;
      struct firstset_values fv =
        firstset_func2(gen, gen->rules[i].rhsnoact, gen->rules[i].noactcnt);
      firstset2_setdefault(gen, gen->rules[i].rhsnoact, gen->rules[i].noactcnt);
      if (firstset2_lookup(gen, gen->rules[i].rhsnoact, gen->rules[i].noactcnt) == NULL)
      {
        abort();
      }
      firstset2_update(gen, &firstset2_lookup(gen, gen->rules[i].rhsnoact, gen->rules[i].noactcnt)->values, &fv, 0, &changedtmp);
      firstset_values_deep_free(&fv);
      if (changedtmp)
      {
        changed = 1;
      }
    }
    for (i = 0; i < gen->rulecnt; i++)
    {
      int changedtmp = 0;
      yale_uint_t nonterminal = gen->rules[i].lhs;
      struct ruleitem rit = {.value = nonterminal, .cb = YALE_UINT_MAX_LEGAL};
      struct firstset_entry2 *fa = firstset2_lookup(gen, gen->rules[i].rhsnoact, gen->rules[i].noactcnt);
      struct firstset_entry2 *fb = firstset2_lookup(gen, &rit, 1);
      firstset2_update(gen, &fb->values, &fa->values, 0, &changedtmp);
      if (changedtmp)
      {
        changed = 1;
      }
    }
  }
  changed = 1;
  while (changed)
  {
    changed = 0;
    for (i = 0; i < gen->rulecnt; i++)
    {
      yale_uint_t nonterminal = gen->rules[i].lhs;
      yale_uint_t rhs[gen->rules[i].noactcnt];
      for (j = 0; j < gen->rules[i].noactcnt; j++)
      {
        rhs[j] = gen->rules[i].rhsnoact[j].value;
      }
      for (j = 0; j < gen->rules[i].noactcnt; j++)
      {
        yale_uint_t rhsmid = rhs[j];
        struct firstset_entry firstrhsright;
        yale_uint_t terminal;
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
            if (!has_bitset(&gen->Fo[rhsmid]->has, terminal))
            {
              changed = 1;
              firstset_update_one(gen->Fo[rhsmid], &firstrhsright.dict, terminal);
            }
          }
        }
        for (terminal = YALE_UINT_MAX_LEGAL - 1; terminal < YALE_UINT_MAX_LEGAL; terminal++)
        {
          if (has_bitset(&firstrhsright.dict.has, terminal))
          {
            if (!has_bitset(&gen->Fo[rhsmid]->has, terminal))
            {
              changed = 1;
              firstset_update_one(gen->Fo[rhsmid], &firstrhsright.dict, terminal);
            }
          }
        }
        if (has_bitset(&firstrhsright.dict.has, gen->epsilon))
        {
          if (!firstset_issubset(gen->Fo[nonterminal], gen->Fo[rhsmid]))
          {
            changed = 1;
            firstset_update(gen->Fo[rhsmid], gen->Fo[nonterminal]);
          }
        }
        if (gen->rules[i].noactcnt == 0 || j == gen->rules[i].noactcnt - 1U)
        {
          if (!firstset_issubset(gen->Fo[nonterminal], gen->Fo[rhsmid]))
          {
            changed = 1;
           firstset_update(gen->Fo[rhsmid], gen->Fo[nonterminal]);
          }
        }
      }
    }
  }
  changed = 1;
  while (changed)
  {
    changed = 0;
    for (i = 0; i < gen->rulecnt; i++)
    {
      yale_uint_t nonterminal = gen->rules[i].lhs;
      yale_uint_t rhs[gen->rules[i].noactcnt];
      for (j = 0; j < gen->rules[i].noactcnt; j++)
      {
        rhs[j] = gen->rules[i].rhsnoact[j].value;
      }
      for (j = 0; j < gen->rules[i].noactcnt; j++)
      {
        yale_uint_t rhsmid = rhs[j];
        struct firstset_values firstrhsright;
        yale_uint_t terminal;
        if (parsergen_is_terminal(gen, rhsmid))
        {
          continue;
        }
        firstrhsright =
          firstset_func2(gen, &gen->rules[i].rhsnoact[j+1], gen->rules[i].noactcnt - j - 1);
        for (terminal = 0; terminal < gen->tokencnt; terminal++)
        {
          if (has_firstset(&firstrhsright, terminal))
          {
            int changedtmp = 0;
            firstset2_update_one(&gen->Fo2[rhsmid], &firstrhsright, terminal, &changedtmp);
            if (changedtmp)
            {
              changed = 1;
            }
          }
        }
        for (terminal = YALE_UINT_MAX_LEGAL - 1; terminal < YALE_UINT_MAX_LEGAL; terminal++)
        {
          if (has_firstset(&firstrhsright, terminal))
          {
            int changedtmp = 0;
            firstset2_update_one(&gen->Fo2[rhsmid], &firstrhsright, terminal, &changedtmp);
            if (changedtmp)
            {
              changed = 1;
            }
          }
        }
        if (has_firstset(&firstrhsright, gen->epsilon))
        {
          int changedtmp = 0;
          firstset2_update(gen, &gen->Fo2[rhsmid], &gen->Fo2[nonterminal], 0,
                           &changedtmp);
          if (changedtmp)
          {
            changed = 1;
          }
        }
        if (gen->rules[i].noactcnt == 0 || j == gen->rules[i].noactcnt - 1U)
        {
          int changedtmp = 0;
          firstset2_update(gen, &gen->Fo2[rhsmid], &gen->Fo2[nonterminal], 0,
                           &changedtmp);
          if (changedtmp)
          {
            changed = 1;
          }
        }
        firstset_values_deep_free(&firstrhsright);
      }
    }
  }
  for (i = 0; i < gen->rulecnt; i++)
  {
    yale_uint_t rhs[gen->rules[i].noactcnt];
    yale_uint_t a;
    yale_uint_t A = gen->rules[i].lhs;
    for (j = 0; j < gen->rules[i].noactcnt; j++)
    {
      rhs[j] = gen->rules[i].rhsnoact[j].value;
    }
    for (a = 0; a < gen->tokencnt; a++)
    {
      struct firstset_entry *fi = firstset_lookup(gen, rhs, gen->rules[i].noactcnt);
      struct dict *fo = gen->Fo[A];
      if (has_bitset(&fi->dict.has, a))
      {
        lookuptbl_put(gen, A, a, gen->rules[i].cond, i, get_sole_cb(&fi->dict, a));
      }
      if (has_bitset(&fi->dict.has, gen->epsilon) && has_bitset(&fo->has, a))
      {
        lookuptbl_put(gen, A, a, gen->rules[i].cond, i, get_sole_cb(fo, a));
      }
    }
    for (a = YALE_UINT_MAX_LEGAL - 1; a < YALE_UINT_MAX_LEGAL; a++)
    {
      struct firstset_entry *fi = firstset_lookup(gen, rhs, gen->rules[i].noactcnt);
      struct dict *fo = gen->Fo[A];
      if (has_bitset(&fi->dict.has, a))
      {
        lookuptbl_put(gen, A, a, gen->rules[i].cond, i, get_sole_cb(&fi->dict, a));
      }
      if (has_bitset(&fi->dict.has, gen->epsilon) && has_bitset(&fo->has, a))
      {
        lookuptbl_put(gen, A, a, gen->rules[i].cond, i, get_sole_cb(fo, a));
      }
    }
  }
  for (i = 0; i < gen->tokencnt; i++)
  {
    gen->pick_those[gen->pick_thoses_cnt][0] = i;
    gen->pick_thoses[gen->pick_thoses_cnt].pick_those =
      gen->pick_those[gen->pick_thoses_cnt];
    gen->pick_thoses[gen->pick_thoses_cnt].len = 1;
    gen->pick_thoses[gen->pick_thoses_cnt].ds = NULL;
    gen->pick_thoses[gen->pick_thoses_cnt].dscnt = 0;
    gen->pick_thoses_cnt++;
  }
  for (i = gen->tokencnt; i < gen->tokencnt + gen->nonterminalcnt; i++)
  {
    yale_uint_t c;
    for (c = 0; c < gen->nonterminal_conds[i].condcnt; c++)
    {
      yale_uint_t cond = gen->nonterminal_conds[i].conds[c].cond;
      size_t len = 0;
      yale_uint_t buf[YALE_UINT_MAX_LEGAL + 1];
      for (j = 0; j < gen->tokencnt; j++)
      {
        uint32_t hashval = lookuptbl_hash(i, j);
        struct yale_hash_list_node *node;
        YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
        {
          struct LookupTblEntry *e =
            YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
          if (e->nonterminal == i && e->terminal == j && e->cond == cond &&
              e->val != YALE_UINT_MAX_LEGAL)
          {
            if (len >= YALE_UINT_MAX_LEGAL)
            {
              abort();
            }
            buf[len++] = j;
          }
        }
      }
      if (len == 0)
      {
        uint32_t hashval = lookuptbl_hash(i, YALE_UINT_MAX_LEGAL-1);
        int found = 0;
        struct yale_hash_list_node *node;
        YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
        {
          struct LookupTblEntry *e =
            YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
          if (e->nonterminal == i && e->terminal == YALE_UINT_MAX_LEGAL-1 &&
              e->val != YALE_UINT_MAX_LEGAL && e->cond == cond)
          {
            found = 1;
            break;
          }
        }
        if (found)
        {
          //printf("Omitting empty states vector due to bytes token\n");
          continue;
        }
      }
      for (j = 0; j < gen->pick_thoses_cnt; j++)
      {
        if (gen->pick_thoses[j].len != len)
        {
          continue;
        }
        if (memcmp(gen->pick_thoses[j].pick_those, buf, len*sizeof(yale_uint_t)) != 0)
        {
          continue;
        }
        break;
      }
      gen->nonterminal_conds[i].conds[c].pick_those = j;
      if (j == gen->pick_thoses_cnt)
      {
        if (j >= sizeof(gen->pick_thoses)/sizeof(*gen->pick_thoses) ||
            j >= sizeof(gen->pick_those)/sizeof(*gen->pick_those))
        {
          abort();
        }
        len = 0;
        for (j = 0; j < gen->tokencnt; j++)
        {
          uint32_t hashval = lookuptbl_hash(i, j);
          struct yale_hash_list_node *node;
          YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
          {
            struct LookupTblEntry *e =
              YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
            if (e->nonterminal == i && e->terminal == j && e->cond == cond &&
                e->val != YALE_UINT_MAX_LEGAL)
            {
              if (len >= YALE_UINT_MAX_LEGAL)
              {
                abort();
              }
              gen->pick_those[gen->pick_thoses_cnt][len++] = j;
            }
          }
        }
        gen->pick_thoses[gen->pick_thoses_cnt].pick_those =
          gen->pick_those[gen->pick_thoses_cnt];
        gen->pick_thoses[gen->pick_thoses_cnt].len = len;
        gen->pick_thoses[gen->pick_thoses_cnt].ds = NULL;
        gen->pick_thoses[gen->pick_thoses_cnt].dscnt = 0;
        gen->pick_thoses_cnt++;
      }
    }
  }
  for (i = 0; i < gen->pick_thoses_cnt; i++)
  {
    pick(gen->ns, gen->ds, gen->re_by_idx, &gen->pick_thoses[i], gen->priorities);
  }
  collect(gen->pick_thoses, gen->pick_thoses_cnt, &gen->bufs, parsergen_alloc_fn, gen);
  for (i = 0; i < gen->pick_thoses_cnt; i++)
  {
    ssize_t curbt;
    curbt = maximal_backtrack(gen->pick_thoses[i].ds, 0, 250);
    if (curbt < 0)
    {
      abort(); // FIXME error handling
    }
    if (gen->max_bt < curbt)
    {
      gen->max_bt = curbt;
    }
  }
  gen->max_stack_size = max_stack_sz(gen);
}

void parsergen_dump_parser(struct ParserGen *gen, FILE *f)
{
  size_t i, j, X, x;
  size_t curidx = 0;
  yale_uint_t c;
  dump_chead(f, gen->parsername, gen->nofastpath);
  dump_collected(f, gen->parsername, &gen->bufs);
  for (i = 0; i < gen->pick_thoses_cnt; i++)
  {
    dump_one(f, gen->parsername, &gen->pick_thoses[i]);
  }
  fprintf(f, "const parser_uint_t %s_num_terminals;\n", gen->parsername);
  fprintf(f, "ssize_t(*%s_callbacks[])(const char*, size_t, int, struct %s_parserctx*) = {\n", gen->parsername, gen->parsername);
  for (i = 0; i < gen->cbcnt; i++)
  {
    fprintf(f, "%s, ", gen->cbs[i].name);
  }
  fprints(f, "};\n");
  fprintf(f, "struct %s_parserstatetblentry {\n", gen->parsername);
  fprints(f, "  const uint8_t is_bytes;\n");
  fprints(f, "  const parser_uint_t bytes_cb;\n");
  fprints(f, "  const struct state *re;\n");
  fprintf(f, "  //const parser_uint_t rhs[%d];\n", gen->tokencnt);
  fprintf(f, "  const parser_uint_t cb[%d];\n", gen->tokencnt);
  fprints(f, "};\n");
  fprintf(f, "const parser_uint_t %s_num_terminals = %d;\n", gen->parsername, gen->tokencnt);
  fprintf(f, "const parser_uint_t %s_start_state = %d;\n", gen->parsername, gen->start_state);
  fprintf(f, "const struct reentry %s_reentries[] = {\n", gen->parsername);
  for (i = 0; i < gen->tokencnt; i++)
  {
    fprints(f, "{\n");
    fprintf(f, ".re = %s_states_%d,", gen->parsername, (int)i);
    fprints(f, "},\n");
  }
  fprints(f, "};\n");
  fprintf(f, "const struct %s_parserstatetblentry %s_parserstatetblentries[] = {\n", gen->parsername, gen->parsername);
  for (X = gen->tokencnt; X < gen->tokencnt + gen->nonterminalcnt; X++)
  {
    for (c = 0; c < gen->nonterminal_conds[X].condcnt; c++)
    {
      yale_uint_t cond = gen->nonterminal_conds[X].conds[c].cond;
      gen->nonterminal_conds[X].conds[c].statetblidx = curidx++;
      int is_bytes = 0, is_re = 0;
      yale_uint_t bytes_cb = YALE_UINT_MAX_LEGAL;
      {
        uint32_t hashval = lookuptbl_hash(X, YALE_UINT_MAX_LEGAL-1);
        struct yale_hash_list_node *node;
        YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
        {
          struct LookupTblEntry *e =
            YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
          if (e->nonterminal == X && e->terminal == YALE_UINT_MAX_LEGAL-1 &&
              e->cond == cond &&
              e->val != YALE_UINT_MAX_LEGAL)
          {
            is_bytes = 1;
            bytes_cb = e->cb;
            break;
          }
        }
      }
      for (i = 0; i < gen->tokencnt; i++)
      {
        uint32_t hashval = lookuptbl_hash(X, i);
        struct yale_hash_list_node *node;
        YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
        {
          struct LookupTblEntry *e =
            YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
          if (e->nonterminal == X && e->terminal == i &&
              e->cond == cond &&
              e->val != YALE_UINT_MAX_LEGAL)
          {
            is_re = 1;
            break;
          }
        }
      }
      if (is_bytes && is_re)
      {
        abort(); // FIXME error handling
      }
      for (x = 0; x < gen->tokencnt; x++)
      {
        //yale_uint_t id = gen->pick_thoses_id_by_nonterminal[X]; // FIXME cond
        yale_uint_t id = gen->nonterminal_conds[X].conds[c].pick_those;
        uint32_t hashval = lookuptbl_hash(X, x);
        struct yale_hash_list_node *node;
        YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
        {
          struct LookupTblEntry *e =
            YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
          if (e->nonterminal == X && e->terminal == x &&
              e->cond == cond && e->cb != YALE_UINT_MAX_LEGAL)
          {
            check_cb(gen->pick_thoses[id].ds, 0, x);
            break;
          }
        }
      }
      fprints(f, "{\n");
      if (is_bytes)
      {
        fprintf(f, ".is_bytes = 1, ");
        fprintf(f, ".bytes_cb = ");
        if (bytes_cb == YALE_UINT_MAX_LEGAL)
        {
          fprintf(f, "PARSER_UINT_MAX,\n");
        }
        else
        {
          fprintf(f, "%d,\n", bytes_cb);
        }
      }
      else
      {
        fprintf(f, ".is_bytes = 0, ");
        fprintf(f, ".re = %s_states", gen->parsername);
        for (i = 0; i < gen->tokencnt; i++)
        {
          uint32_t hashval = lookuptbl_hash(X, i);
          struct yale_hash_list_node *node;
          YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
          {
            struct LookupTblEntry *e =
              YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
            if (e->nonterminal == X && e->terminal == i &&
                e->cond == cond &&
                e->val != YALE_UINT_MAX_LEGAL)
            {
              fprintf(f, "_%d", (int)i);
              break;
            }
          }
        }
        fprints(f, ",\n");
        fprints(f, ".cb = {\n");
        for (i = 0; i < gen->tokencnt; i++)
        {
          uint32_t hashval = lookuptbl_hash(X, i);
          int found = 0;
          struct yale_hash_list_node *node;
          YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
          {
            struct LookupTblEntry *e =
              YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
            if (e->nonterminal == X && e->terminal == i &&
                e->cond == cond &&
                e->val != YALE_UINT_MAX_LEGAL)
            {
              fprintf(f, "%d, ", e->cb);
              found = 1;
              break;
            }
          }
          if (!found)
          {
            fprintf(f, "PARSER_UINT_MAX, ");
          }
        }
        fprints(f, "},\n");
      }
      fprints(f, "},\n");
    }
  }
  fprints(f, "};\n");
  for (i = 0; i < gen->rulecnt; i++)
  {
    fprintf(f, "const struct ruleentry %s_rule_%d[] = {\n", gen->parsername, (int)i);
    for (j = 0; j < gen->rules[i].itemcnt; j++)
    {
      struct ruleitem *it = &gen->rules[i].rhs[gen->rules[i].itemcnt-1-j];
      fprintf(f, "{\n");
      if (it->is_action)
      {
        if (it->value != YALE_UINT_MAX_LEGAL)
        {
          abort();
        }
        fprintf(f, ".rhs = PARSER_UINT_MAX, .cb = %d", it->cb);
      }
      else if (it->cb == YALE_UINT_MAX_LEGAL)
      {
        if (it->value == YALE_UINT_MAX_LEGAL-1)
        {
          fprintf(f, ".rhs = PARSER_UINT_MAX-1, .cb = PARSER_UINT_MAX");
        }
        else
        {
          fprintf(f, ".rhs = %d, .cb = PARSER_UINT_MAX", it->value);
        }
      }
      else
      {
        if (it->value == YALE_UINT_MAX_LEGAL-1)
        {
          fprintf(f, ".rhs = PARSER_UINT_MAX-1, .cb = %d", it->cb);
        }
        else
        {
          check_cb(gen->pick_thoses[it->value].ds, 0, it->value);
          fprintf(f, ".rhs = %d, .cb = %d", it->value, it->cb);
        }
      }
      fprints(f, "},\n");
    }
    fprints(f, "};\n");
    fprintf(f, "const parser_uint_t %s_rule_%d_len = sizeof(%s_rule_%d)/sizeof(struct ruleentry);\n", gen->parsername, (int)i, gen->parsername, (int)i);
  }
  fprintf(f, "const struct rule %s_rules[] = {\n", gen->parsername);
  for (i = 0; i < gen->rulecnt; i++)
  {
    fprints(f, "{\n");
    fprintf(f, "  .lhs = %d,\n", gen->rules[i].lhs);
    fprintf(f, "  .rhssz = sizeof(%s_rule_%d)/sizeof(struct ruleentry),\n", gen->parsername, (int)i);
    fprintf(f, "  .rhs = %s_rule_%d,\n", gen->parsername, (int)i);
    fprints(f, "},\n");
  }
  fprints(f, "};\n");
  fprints(f, "static inline ssize_t\n");
  fprintf(f, "%s_get_saved_token(struct %s_parserctx *pctx, const struct state *restates,\n", gen->parsername, gen->parsername);
  fprints(f, "                const char *blkoff, size_t szoff, parser_uint_t *state,\n"
             "                const parser_uint_t *cbs, parser_uint_t cb1)//, void *baton)\n"
             "{\n"
             "  if (pctx->saved_token != PARSER_UINT_MAX)\n"
             "  {\n"
             "    *state = pctx->saved_token;\n"
             "    pctx->saved_token = PARSER_UINT_MAX;\n"
             "    return 0;\n"
             "  }\n");
  fprintf(f, "  return %s_feed_statemachine(&pctx->rctx, restates, blkoff, szoff, state, %s_callbacks, cbs, cb1);//, baton);\n", gen->parsername, gen->parsername);
  fprints(f, "}\n"
             "\n"
             "#define EXTRA_SANITY\n"
             "\n");
  fprintf(f, "ssize_t %s_parse_block(struct %s_parserctx *pctx, const char *blk, size_t sz)//, void *baton)\n", gen->parsername, gen->parsername);
  fprints(f, "{\n"
             "  size_t off = 0;\n"
             "  ssize_t ret;\n"
             "  parser_uint_t curstateoff;\n"
             "  parser_uint_t curstate;\n"
             "  parser_uint_t state;\n"
             "  parser_uint_t ruleid;\n"
             "  parser_uint_t i; // FIXME is 8 bits enough?\n"
             "  parser_uint_t cb1;\n"
             "  const struct state *restates;\n"
             "  const struct rule *rule;\n"
             "  const parser_uint_t *cbs;\n");
  fprintf(f, "  ssize_t (*cb1f)(const char *, size_t, int, struct %s_parserctx*);\n", gen->parsername);
  fprints(f, "\n"
             "  while (off < sz || pctx->saved_token != PARSER_UINT_MAX)\n"
             "  {\n"
             "    if (unlikely(pctx->stacksz == 0))\n"
             "    {\n"
             "      if (off >= sz && pctx->saved_token == PARSER_UINT_MAX)\n"
             "      {\n"
             "#ifdef EXTRA_SANITY\n");
  fprints(f, "        if (off > sz)\n"
             "        {\n"
             "          abort();\n"
             "        }\n"
             "#endif\n"
             "        return sz; // EOF\n"
             "      }\n"
             "      else\n"
             "      {\n");
  fprints(f, "#ifdef EXTRA_SANITY\n"
             "        if (off > sz)\n"
             "        {\n"
             "          abort();\n"
             "        }\n"
             "#endif\n"
             "        return -EBADMSG;\n"
             "      }\n"
             "    }\n"
             "    curstate = pctx->stack[pctx->stacksz - 1].rhs;\n");
  if (gen->bytes_size_type == NULL || strcmp(gen->bytes_size_type, "void") != 0)
  {
    fprints(f, "    if (curstate == PARSER_UINT_MAX - 1)\n");
    fprints(f, "    {\n");
    fprints(f, "      parser_uint_t bytes_cb = pctx->stack[pctx->stacksz - 1].cb;\n");
    fprintf(f, "      ret = pctx->bytes_sz < (sz-off) ? pctx->bytes_sz : (sz-off);\n");
    fprintf(f, "      if (bytes_cb != PARSER_UINT_MAX)\n");
    fprintf(f, "      {\n");
    fprintf(f, "        enum yale_flags flags = 0;\n");
    fprintf(f, "        if (pctx->bytes_start)\n");
    fprintf(f, "        {\n");
    fprintf(f, "          flags |= YALE_FLAG_START;\n");
    fprintf(f, "        }\n");
    fprintf(f, "        if (ret == pctx->bytes_sz)\n");
    fprintf(f, "        {\n");
    fprintf(f, "          flags |= YALE_FLAG_END;\n");
    fprintf(f, "        }\n");
    fprintf(f, "        ssize_t cbr = %s_callbacks[bytes_cb](blk+off, ret, flags, pctx);\n", gen->parsername);
    fprints(f, "        if (cbr != ret && cbr != -EAGAIN && cbr != -EWOULDBLOCK)\n");
    fprints(f, "        {\n");
    fprints(f, "          return cbr;");
    fprints(f, "        }\n");
    fprintf(f, "        pctx->bytes_start = 0;\n");
    fprintf(f, "      }\n");
    fprintf(f, "      pctx->bytes_sz -= ret;\n");
    fprintf(f, "      off += ret;\n");
    fprintf(f, "      if (pctx->bytes_sz)\n");
    fprintf(f, "      {\n");
    fprintf(f, "        //off = sz;\n");
    fprintf(f, "        return -EAGAIN;\n");
    fprintf(f, "      }\n");
    fprintf(f, "      pctx->bytes_start = 1;\n");
    fprints(f, "      pctx->stacksz--;\n");
    fprints(f, "    }\n");
    fprintf(f, "    else if (curstate < %s_num_terminals)\n", gen->parsername);
  }
  else
  {
    fprintf(f, "    if (curstate < %s_num_terminals)\n", gen->parsername);
  }
  fprints(f, "    {\n");
  fprintf(f, "      restates = %s_reentries[curstate].re;\n", gen->parsername);
  fprints(f, "      cb1 = pctx->stack[pctx->stacksz - 1].cb;\n");
  fprintf(f, "      ret = %s_get_saved_token(pctx, restates, blk+off, sz-off, &state, NULL, cb1);//, baton);\n", gen->parsername);
  fprints(f, "      if (ret == -EAGAIN)\n"
             "      {\n"
             "        //off = sz;\n"
             "        return -EAGAIN;\n"
             "      }\n"
             "      else if (ret < 0)\n"
             "      {\n"
             "        //fprintf(stderr, \"Parser error: tokenizer error, curstate=%%d\\n\", curstate);\n"
             "        //fprintf(stderr, \"blk[off] = '%%c'\\n\", blk[off]);\n");
  fprints(f, "        //abort();\n"
             "        //exit(1);\n"
             "        return -EINVAL;\n"
             "      }\n"
             "      else\n"
             "      {\n"
             "        off += ret;\n"
             "#if 0\n"
             "        if (off > sz)\n"
             "        {\n"
             "          abort();\n"
             "        }\n"
             "#endif\n");
  fprints(f, "      }\n"
             "#ifdef EXTRA_SANITY\n"
             "      if (curstate != state)\n"
             "      {\n"
             "        //fprintf(stderr, \"Parser error: state mismatch %%d %%d\\n\",\n"
             "        //                (int)curstate, (int)state);\n"
             "        //exit(1);\n"
             "        //return -EINVAL;\n"
             "        abort();\n"
             "      }\n"
             "#endif\n");
  fprints(f, "      //printf(\"Got expected token %%d\\n\", (int)state);\n"
             "      pctx->stacksz--;\n"
             "    }\n"
             "    else if (likely(curstate != PARSER_UINT_MAX))\n"
             "    {\n");
  fprintf(f, "      int is_bytes;\n");
  fprintf(f, "      curstateoff = PARSER_UINT_MAX;\n");
  fprintf(f, "      switch (curstate)\n");
  fprintf(f, "      {\n");
  for (X = gen->tokencnt; X < gen->tokencnt + gen->nonterminalcnt; X++)
  {
    fprintf(f, "        case %d:\n", (int)X);
    for (c = 0; c < gen->nonterminal_conds[X].condcnt; c++)
    {
      if (gen->nonterminal_conds[X].conds[c].cond == YALE_UINT_MAX_LEGAL)
      {
        fprintf(f, "          curstateoff = %d;\n", (int)gen->nonterminal_conds[X].conds[c].statetblidx);
      }
    }
    for (c = 0; c < gen->nonterminal_conds[X].condcnt; c++)
    {
      if (gen->nonterminal_conds[X].conds[c].cond != YALE_UINT_MAX_LEGAL)
      {
        fprintf(f, "          if (%s)\n", gen->conds[gen->nonterminal_conds[X].conds[c].cond]);
        fprintf(f, "          {\n");
        fprintf(f, "            curstateoff = %d;\n", (int)gen->nonterminal_conds[X].conds[c].statetblidx);
        fprintf(f, "          }\n");
      }
    }
    fprintf(f, "          break;\n");
  }
  fprintf(f, "        default:\n");
  fprintf(f, "          abort();\n");
  fprintf(f, "      }\n");
  //fprintf(f, "      curstateoff = curstate - %s_num_terminals;\n", gen->parsername);
  fprintf(f, "      if (curstateoff == PARSER_UINT_MAX)\n");
  fprintf(f, "      {\n");
  fprintf(f, "        return -EINVAL;\n");
  fprintf(f, "      }\n");
  fprintf(f, "      restates = %s_parserstatetblentries[curstateoff].re;\n", gen->parsername);
  fprintf(f, "      cbs = %s_parserstatetblentries[curstateoff].cb;\n", gen->parsername);
  fprintf(f, "      is_bytes = %s_parserstatetblentries[curstateoff].is_bytes;\n", gen->parsername);
  fprintf(f, "      if (is_bytes)\n");
  fprintf(f, "      {\n");
  if (gen->bytes_size_type == NULL || strcmp(gen->bytes_size_type, "void") != 0)
  {
    fprintf(f, "        parser_uint_t bytes_cb = %s_parserstatetblentries[curstateoff].bytes_cb;\n", gen->parsername);
    fprintf(f, "        enum yale_flags flags = 0;\n");
    fprintf(f, "        ret = pctx->bytes_sz < (sz-off) ? pctx->bytes_sz : (sz-off);\n");
    fprintf(f, "        if (pctx->bytes_start)\n");
    fprintf(f, "        {\n");
    fprintf(f, "          flags |= YALE_FLAG_START;\n");
    fprintf(f, "        }\n");
    fprintf(f, "        if (ret == pctx->bytes_sz)\n");
    fprintf(f, "        {\n");
    fprintf(f, "          flags |= YALE_FLAG_END;\n");
    fprintf(f, "        }\n");
    fprintf(f, "        if (bytes_cb != PARSER_UINT_MAX)\n");
    fprintf(f, "        {\n");
    fprintf(f, "          ssize_t cbr = %s_callbacks[bytes_cb](blk+off, ret, flags, pctx);\n", gen->parsername);
    fprints(f, "          if (cbr != ret && cbr != -EAGAIN && cbr != -EWOULDBLOCK)\n");
    fprints(f, "          {\n");
    fprints(f, "            return cbr;");
    fprints(f, "          }\n");
    fprintf(f, "          pctx->bytes_start = 0;\n");
    fprintf(f, "        }\n");
    fprintf(f, "        pctx->bytes_sz -= ret;\n");
    fprintf(f, "        off += ret;\n");
    fprintf(f, "        if (pctx->bytes_sz)\n");
    fprintf(f, "        {\n");
    fprintf(f, "          //off = sz;\n");
    fprintf(f, "          return -EAGAIN;\n");
    fprintf(f, "        }\n");
    fprintf(f, "        pctx->bytes_start = 1;\n");
    fprintf(f, "        state = PARSER_UINT_MAX-1;\n");
  }
  else
  {
    fprintf(f, "        abort();\n");
  }
  fprintf(f, "      }\n");
  fprintf(f, "      else\n");
  fprintf(f, "      {\n");
  fprintf(f, "        ret = %s_get_saved_token(pctx, restates, blk+off, sz-off, &state, cbs, PARSER_UINT_MAX);//, baton);\n", gen->parsername);
  fprints(f, "        if (ret == -EAGAIN)\n"
             "        {\n"
             "          //off = sz;\n"
             "          return -EAGAIN;\n"
             "        }\n"
             "        else if (ret < 0 || state == PARSER_UINT_MAX)\n"
             "        {\n"
             "          //fprintf(stderr, \"Parser error: tokenizer error, curstate=%%d, token=%%d\\n\", (int)curstate, (int)state);\n");
  fprints(f, "          //exit(1);\n"
             "          return -EINVAL;\n"
             "        }\n"
             "        else\n"
             "        {\n"
             "          off += ret;\n"
             "#if 0\n"
             "          if (off > sz)\n");
  fprints(f, "          {\n"
             "            abort();\n"
             "          }\n"
             "#endif\n"
             "        }\n"
             "      }\n"
             "      //printf(\"Got token %%d, curstate=%%d\\n\", (int)state, (int)curstate);\n"
             "      switch (curstate)\n"
             "      {\n");
  for (X = gen->tokencnt; X < gen->tokencnt + gen->nonterminalcnt; X++)
  {
    fprintf(f, "      case %d:\n", (int)X);
    fprintf(f, "        ruleid=PARSER_UINT_MAX;\n");
    for (c = 0; c < gen->nonterminal_conds[X].condcnt; c++)
    {
      yale_uint_t cond = gen->nonterminal_conds[X].conds[c].cond;
      if (cond == YALE_UINT_MAX_LEGAL)
      {
        fprints(f, "        switch (state)\n"
                   "        {\n");
        for (x = 0; x < gen->tokencnt; x++)
        {
          uint32_t hashval = lookuptbl_hash(X, x);
          struct yale_hash_list_node *node;
          YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
          {
            struct LookupTblEntry *e =
              YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
            if (e->nonterminal == X && e->terminal == x && e->cond == cond &&
                e->val != YALE_UINT_MAX_LEGAL)
            {
              fprintf(f, "        case %d:\n", (int)x);
              fprintf(f, "          ruleid=%d;\n", (int)e->val);
              fprints(f, "          break;\n");
            }
          }
        }
        {
          uint32_t hashval = lookuptbl_hash(X, YALE_UINT_MAX_LEGAL-1);
          struct yale_hash_list_node *node;
          YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
          {
            struct LookupTblEntry *e =
              YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
            if (e->nonterminal == X && e->terminal == YALE_UINT_MAX_LEGAL-1 &&
                e->cond == cond && e->val != YALE_UINT_MAX_LEGAL)
            {
              fprintf(f, "        case PARSER_UINT_MAX-1:\n");
              fprintf(f, "          ruleid=%d;\n", (int)e->val);
              fprints(f, "          break;\n");
            }
          }
        }
        fprints(f, "        default:\n"
                   "          ruleid=PARSER_UINT_MAX;\n"
                   "          break;\n"
                   "        }\n");
      }
    }
    for (c = 0; c < gen->nonterminal_conds[X].condcnt; c++)
    {
      yale_uint_t cond = gen->nonterminal_conds[X].conds[c].cond;
      if (cond != YALE_UINT_MAX_LEGAL)
      {
        fprintf(f, "        if (%s)\n", gen->conds[cond]);
        fprintf(f, "        {\n");
        fprints(f, "          switch (state)\n"
                   "          {\n");
        for (x = 0; x < gen->tokencnt; x++)
        {
          uint32_t hashval = lookuptbl_hash(X, x);
          struct yale_hash_list_node *node;
          YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
          {
            struct LookupTblEntry *e =
              YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
            if (e->nonterminal == X && e->terminal == x && e->cond == cond &&
                e->val != YALE_UINT_MAX_LEGAL)
            {
              fprintf(f, "          case %d:\n", (int)x);
              fprintf(f, "            ruleid=%d;\n", (int)e->val);
              fprints(f, "            break;\n");
            }
          }
        }
        {
          uint32_t hashval = lookuptbl_hash(X, YALE_UINT_MAX_LEGAL-1);
          struct yale_hash_list_node *node;
          YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
          {
            struct LookupTblEntry *e =
              YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
            if (e->nonterminal == X && e->terminal == YALE_UINT_MAX_LEGAL-1 &&
                e->cond == cond && e->val != YALE_UINT_MAX_LEGAL)
            {
              fprintf(f, "          case PARSER_UINT_MAX-1:\n");
              fprintf(f, "            ruleid=%d;\n", (int)e->val);
              fprints(f, "            break;\n");
            }
          }
        }
        fprints(f, "          default:\n"
                   "            ruleid=PARSER_UINT_MAX;\n"
                   "            break;\n"
                   "          }\n");
        fprintf(f, "        }\n");
      }
    }
    fprints(f, "        break;\n");
  }
  fprints(f, "      default:\n"
             "        abort();\n"
             "      }\n");
  fprintf(f, "      //ruleid = %s_parserstatetblentries[curstateoff].rhs[state];\n", gen->parsername);
  fprintf(f, "      if (ruleid == PARSER_UINT_MAX)\n");
  fprintf(f, "      {\n");
  fprintf(f, "        return -EINVAL;\n");
  fprintf(f, "      }\n");
  fprintf(f, "      rule = &%s_rules[ruleid];\n", gen->parsername);
  fprints(f, "      pctx->stacksz--;\n"
             "#if 0\n"
             "      if (rule->lhs != curstate)\n"
             "      {\n"
             "        abort();\n"
             "      }\n"
             "#endif\n"
             "      if (pctx->stacksz + rule->rhssz > sizeof(pctx->stack)/sizeof(struct ruleentry))\n");
  fprints(f, "      {\n"
             "        abort();\n"
             "      }\n"
             "      i = 0;\n"
             "      while (i + 4 <= rule->rhssz)\n"
             "      {\n"
             "        pctx->stack[pctx->stacksz++] = rule->rhs[i+0];\n"
             "        pctx->stack[pctx->stacksz++] = rule->rhs[i+1];\n"
             "        pctx->stack[pctx->stacksz++] = rule->rhs[i+2];\n");
  fprints(f, "        pctx->stack[pctx->stacksz++] = rule->rhs[i+3];\n"
             "        i += 4;\n"
             "      }\n"
             "      for (; i < rule->rhssz; i++)\n"
             "      {\n"
             "        pctx->stack[pctx->stacksz++] = rule->rhs[i];\n"
             "      }\n");
  fprintf(f, "      if (rule->rhssz && (pctx->stack[pctx->stacksz-1].rhs < %s_num_terminals || pctx->stack[pctx->stacksz-1].rhs == PARSER_UINT_MAX-1))\n", gen->parsername);
  fprints(f, "      {\n"
             "        pctx->stacksz--; // Has to be correct token so let's process immediately\n"
             "      }\n"
             "      else\n"
             "      {\n"
             "        pctx->saved_token = state;\n"
             "      }\n"
             "    }\n"
             "    else // if (curstate == PARSER_UINT_MAX)\n"
             "    {\n");
  fprintf(f, "      cb1f = %s_callbacks[pctx->stack[pctx->stacksz - 1].cb];\n", gen->parsername);
  fprints(f, "      ssize_t cbr = cb1f(NULL, 0, YALE_FLAG_ACTION, pctx);//, baton);\n"
             "      if (cbr != 0 && cbr != -EAGAIN && cbr != -EWOULDBLOCK)\n"
             "      {\n"
             "        return cbr;\n"
             "      }\n"
             "      pctx->stacksz--;\n"
             "    }\n"
             "  }\n"
             "  if (pctx->stacksz == 0)\n"
             "  {\n"
             "    if (off >= sz && pctx->saved_token == PARSER_UINT_MAX)\n"
             "    {\n");
  fprints(f, "#ifdef EXTRA_SANITY\n"
             "      if (off > sz)\n"
             "      {\n"
             "        abort();\n"
             "      }\n"
             "#endif\n"
             "      return sz; // EOF\n");
  fprints(f, "    }\n"
             "  }\n"
             "#ifdef EXTRA_SANITY\n"
             "  if (off > sz)\n"
             "  {\n"
             "    abort();\n"
             "  }\n"
             "#endif\n"
             "  return -EAGAIN;\n"
             "}\n");
}


void parsergen_dump_headers(struct ParserGen *gen, FILE *f)
{
  fprints(f, "#include \"yalecommon.h\"\n");
  dump_headers(f, gen->parsername, gen->max_bt);
  fprintf(f, "struct %s_parserctx {\n", gen->parsername);
  if (gen->bytes_size_type == NULL || strcmp(gen->bytes_size_type, "void") != 0)
  {
    fprintf(f, "  %s bytes_sz;\n", gen->bytes_size_type ? gen->bytes_size_type : "uint64_t");
  }
  fprints(f, "  parser_uint_t stacksz;\n");
  fprints(f, "  parser_uint_t saved_token;\n");
  fprints(f, "  uint8_t bytes_start;\n");
  fprintf(f, "  struct ruleentry stack[%d];\n", gen->max_stack_size);
  fprintf(f, "  struct %s_rectx rctx;\n", gen->parsername);
  if (gen->state_include_str)
  {
    fprintf(f, "  %s\n", gen->state_include_str);
  }
  fprints(f, "};\n");
  fprints(f, "\n");
  fprintf(f, "static inline void %s_parserctx_init(struct %s_parserctx *pctx)\n",
          gen->parsername, gen->parsername);
  fprints(f, "{\n");
  fprints(f, "  pctx->saved_token = PARSER_UINT_MAX;\n");
  fprints(f, "  pctx->bytes_start = 1;\n");
  if (gen->bytes_size_type == NULL || strcmp(gen->bytes_size_type, "void") != 0)
  {
    fprints(f, "  pctx->bytes_sz = 0;\n");
  }
  fprints(f, "  pctx->stacksz = 1;\n");
  fprintf(f, "  pctx->stack[0].rhs = %d;\n", gen->start_state);
  fprints(f, "  pctx->stack[0].cb = PARSER_UINT_MAX;\n");
  fprintf(f, "  %s_init_statemachine(&pctx->rctx);\n", gen->parsername);
  if (gen->init_include_str)
  {
    fprintf(f, "  %s\n", gen->init_include_str);
  }
  fprints(f, "}\n");
  fprints(f, "\n");
  fprintf(f, "ssize_t %s_parse_block(struct %s_parserctx *pctx, const char *blk, size_t sz);//, void *baton);\n", gen->parsername, gen->parsername);
}

void parsergen_state_include(struct ParserGen *gen, char *stateinclude)
{
  if (gen->state_include_str != NULL || stateinclude == NULL)
  {
    abort();
  }
  gen->state_include_str = strdup(stateinclude);
}

void parsergen_init_include(struct ParserGen *gen, char *initinclude)
{
  if (gen->init_include_str != NULL || initinclude == NULL)
  {
    abort();
  }
  gen->init_include_str = strdup(initinclude);
}

void parsergen_set_start_state(struct ParserGen *gen, yale_uint_t start_state)
{
  if (gen->start_state != YALE_UINT_MAX_LEGAL || start_state == YALE_UINT_MAX_LEGAL)
  {
    abort();
  }
  gen->start_state = start_state;
}

static void *memdup(const void *base, size_t sz)
{
  void *result = malloc(sz);
  memcpy(result, base, sz);
  return result;
}

yale_uint_t parsergen_add_token(struct ParserGen *gen, char *re, size_t resz, int prio)
{
  if (gen->tokens_finalized)
  {
    abort();
  }
  if (gen->tokencnt >= YALE_UINT_MAX_LEGAL)
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

yale_uint_t parsergen_add_nonterminal(struct ParserGen *gen)
{
  if (!gen->tokens_finalized)
  {
    abort();
  }
  if (gen->tokencnt + gen->nonterminalcnt >= YALE_UINT_MAX_LEGAL)
  {
    abort();
  }
  return gen->tokencnt + (gen->nonterminalcnt++);
}

int parsergen_is_terminal(struct ParserGen *gen, yale_uint_t x)
{
  if (!gen->tokens_finalized)
  {
    abort();
  }
  return x < gen->tokencnt || x == YALE_UINT_MAX_LEGAL - 1;
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
  if (rhs->is_bytes)
  {
    return 1; // Let's consider bytes as terminal
  }
  return parsergen_is_terminal(gen, rhs->value);
}

void parsergen_set_conds(struct ParserGen *gen, char **conds, yale_uint_t condcnt)
{
  gen->condcnt = condcnt;
  memcpy(gen->conds, conds, condcnt*sizeof(*conds));
}

void parsergen_set_rules(struct ParserGen *gen, const struct rule *rules, yale_uint_t rulecnt, const struct namespaceitem *ns)
{
  yale_uint_t i;
  yale_uint_t j;
  gen->rulecnt = rulecnt;
  for (i = 0; i < rulecnt; i++)
  {
    gen->rules[i] = rules[i];
    for (j = 0; j < gen->rules[i].itemcnt; j++)
    {
      if (gen->rules[i].rhs[j].is_action)
      {
        if (gen->rules[i].rhs[j].value != YALE_UINT_MAX_LEGAL)
        {
          abort();
        }
      }
      else if (gen->rules[i].rhs[j].is_bytes)
      {
        if (gen->rules[i].rhs[j].value != YALE_UINT_MAX_LEGAL - 1)
        {
          abort();
        }
      }
      else
      {
        gen->rules[i].rhs[j].value = ns[gen->rules[i].rhs[j].value].val;
      }
    }
    for (j = 0; j < gen->rules[i].noactcnt; j++)
    {
      if (gen->rules[i].rhsnoact[j].is_action)
      {
        abort();
      }
      else if (gen->rules[i].rhsnoact[j].is_bytes)
      {
        if (gen->rules[i].rhsnoact[j].value != YALE_UINT_MAX_LEGAL - 1)
        {
          abort();
        }
      }
      else
      {
        gen->rules[i].rhsnoact[j].value = ns[gen->rules[i].rhsnoact[j].value].val;
      }
    }
  }
}

void parsergen_set_cb(struct ParserGen *gen, const struct cb *cbs, yale_uint_t cbcnt)
{
  yale_uint_t i;
  gen->cbcnt = cbcnt;
  for (i = 0; i < cbcnt; i++)
  {
    gen->cbs[i] = cbs[i];
  }
}

void parsergen_set_bytessizetype(struct ParserGen *gen, char *type)
{
  gen->bytes_size_type = type;
}

void parsergen_nofastpath(struct ParserGen *gen)
{
  gen->nofastpath = 1;
}
