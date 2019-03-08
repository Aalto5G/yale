#define _GNU_SOURCE
#include "yale.h"
#include "parser.h"
#include "yalecontainerof.h"
#include "yalemurmur.h"
#include <sys/uio.h>

#undef DO_PRINT_STACKCONFIG

static void *parsergen_alloc(struct ParserGen *gen, size_t sz)
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

static void *parsergen_alloc_fn(void *gen, size_t sz)
{
  return parsergen_alloc(gen, sz);
}

static int fprints(FILE *f, const char *s)
{
  return fputs(s, f);
}

static uint32_t stack_cb_hash(const struct stackconfigitem *stack, yale_uint_t sz, yale_uint_t cbsz)
{
  struct yalemurmurctx ctx = YALEMURMURCTX_INITER(0x12345678U);
  yalemurmurctx_feed32(&ctx, cbsz);
  yalemurmurctx_feed_buf(&ctx, stack, sz*sizeof(*stack));
  return yalemurmurctx_get(&ctx);
}

static uint32_t stack_cb_hash_fn(struct yale_hash_list_node *node, void *ud)
{
  struct stackconfig *cfg = YALE_CONTAINER_OF(node, struct stackconfig, node);
  return stack_cb_hash(cfg->stack, cfg->sz, cfg->cbsz);
}

static uint32_t lookuptbl_hash(yale_uint_t nonterminal, yale_uint_t terminal)
{
  struct yalemurmurctx ctx = YALEMURMURCTX_INITER(0x12345678U);
  yalemurmurctx_feed32(&ctx, nonterminal);
  yalemurmurctx_feed32(&ctx, terminal);
  return yalemurmurctx_get(&ctx);
}

static uint32_t lookuptbl_hash_fn(struct yale_hash_list_node *node, void *ud)
{
  struct LookupTblEntry *e = YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
  return lookuptbl_hash(e->nonterminal, e->terminal);
}

static void lookuptbl_put(struct ParserGen *gen,
                   yale_uint_t nonterminal, yale_uint_t terminal,
                   yale_uint_t cond,
                   yale_uint_t val,
                   yale_uint_t *cbs, yale_uint_t cbsz,
                   int conflict)
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
      printf("lookup table conflict\n");
      exit(1);
    }
  }
  if (gen->Tcnt >= sizeof(gen->Tentries)/sizeof(*gen->Tentries))
  {
    printf("lookup table size exceeded\n");
    exit(1);
  }
  e = &gen->Tentries[gen->Tcnt++];
  e->nonterminal = nonterminal;
  e->terminal = terminal;
  e->cond = cond;
  e->val = val;
  e->conflict = !!conflict;
  memset(&e->cbs, 0, sizeof(e->cbs));
  for (i = 0; i < cbsz; i++)
  {
    set_bitset(&e->cbs, cbs[i]);
  }
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
    printf("too many conditions for nonterminal\n");
    exit(1);
  }
  gen->nonterminal_conds[nonterminal].conds[i].cond = cond;
  gen->nonterminal_conds[nonterminal].condcnt++;
}

static uint32_t firstset2_hash(const struct ruleitem *rhs, size_t sz)
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

static uint32_t firstset2_hash_fn(struct yale_hash_list_node *node, void *ud)
{
  struct firstset_entry2 *cfg = YALE_CONTAINER_OF(node, struct firstset_entry2, node);
  return firstset2_hash(cfg->rhs, cfg->rhssz);
}

void parsergen_init(struct ParserGen *gen, char *parsername)
{
  size_t i;
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
    gen->nonterminal_conds[i].is_shortcut = 0;
  }
  transitionbufs_init(&gen->bufs, parsergen_alloc_fn, gen);
  gen->pick_thoses_cnt = 0;
  yale_hash_table_init(&gen->Fi2_hash, 8192, firstset2_hash_fn, NULL,
                       parsergen_alloc_fn, gen);
  yale_hash_table_init(&gen->stackconfigs_hash, 32768, stack_cb_hash_fn, NULL,
                       parsergen_alloc_fn, gen);
  yale_hash_table_init(&gen->Thash, 32768, lookuptbl_hash_fn, NULL,
                       parsergen_alloc_fn, gen);
  numbers_sets_init(&gen->numbershash, parsergen_alloc_fn, gen);
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

static int rhseq(const struct ruleitem *rhs1, const struct ruleitem *rhs2, size_t sz)
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

static struct firstset_entry2 *firstset2_lookup_hashval(
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

static struct firstset_entry2 *firstset2_lookup(struct ParserGen *gen, const struct ruleitem *rhs, size_t rhssz)
{
  return firstset2_lookup_hashval(gen, rhs, rhssz, firstset2_hash(rhs, rhssz));
}

static void firstset2_setdefault(struct ParserGen *gen, const struct ruleitem *rhs, size_t rhssz)
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

int parsergen_is_rhs_terminal(struct ParserGen *gen, const struct ruleitem *rhs);

static struct firstset_value firstset_value_deep_copy(struct firstset_value orig)
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

static void firstset_value_deep_free(struct firstset_value *orig)
{
  free(orig->cbs);
  orig->cbs = NULL;
}

static struct firstset_values firstset_values_deep_copy(struct firstset_values orig)
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

static int cb_compar(const void *a, const void *b)
{
  const int *iap = a, *ibp = b;
  yale_uint_t ia = *iap, ib = *ibp;
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

static struct firstset_value firstset_value_deep_copy_cbadd(struct firstset_value orig, yale_uint_t cb)
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

static struct firstset_values firstset_values_deep_copy_cbadd(struct firstset_values orig, yale_uint_t cb)
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
static int valcmp(const struct firstset_value *val1, const struct firstset_value *val2)
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
static int valcmpvoid(const void *val1, const void *val2)
{
  return valcmp(val1, val2);
}

static void firstset2_update_one(struct firstset_values *val2, const struct firstset_values *val1, yale_uint_t one, int *changed)
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
#if 0
static void firstset2_update_epsilon(struct firstset_values *val2, const struct firstset_values *val1)
{
  return firstset2_update(NULL, val2, val1, 0, NULL);
}
#endif
static void firstset2_update_noepsilon(struct ParserGen *gen, struct firstset_values *val2, const struct firstset_values *val1)
{
  return firstset2_update(gen, val2, val1, 1, NULL);
}

static struct firstset_values firstset_func2(struct ParserGen *gen, const struct ruleitem *rhs, size_t rhssz)
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
    struct firstset_values toupdate;
    toupdate = firstset_values_deep_copy_cbadd(e2->values, rhs[0].cb);
    firstset2_update_noepsilon(gen, &result2, &toupdate);
    firstset_values_deep_free(&toupdate);
    return result2;
  }
}

static void stackconfig_print(struct ParserGen *gen, yale_uint_t scidx)
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

static void stacktransitionlast_print(struct ParserGen *gen, yale_uint_t scidx)
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

static void stacktransitionarbitrary_print(struct ParserGen *gen, yale_uint_t scidx1,
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


static size_t stackconfig_append(struct ParserGen *gen, const struct stackconfigitem *stack, yale_uint_t sz,
                          yale_uint_t cbsz)
{
  size_t i = gen->stackconfigcnt;
  uint32_t hashval = stack_cb_hash(stack, sz, cbsz);
  struct yale_hash_list_node *node;
  YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->stackconfigs_hash, node, hashval)
  {
    struct stackconfig *cfg = YALE_CONTAINER_OF(node, struct stackconfig, node);
    if (cfg->sz != sz)
    {
      continue;
    }
    if (memcmp(cfg->stack, stack, sz*sizeof(*stack)) != 0)
    {
      continue;
    }
    if (cfg->cbsz != cbsz)
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
      printf("Error: too many PDA stack configurations\n");
      exit(1);
    }
    gen->stackconfigs[i] = parsergen_alloc(gen, sizeof(*gen->stackconfigs[i]));
    gen->stackconfigs[i]->stack = parsergen_alloc(gen, sz*sizeof(*stack));
    memcpy(gen->stackconfigs[i]->stack, stack, sz*sizeof(*stack));
    gen->stackconfigs[i]->sz = sz;
    gen->stackconfigs[i]->cbsz = cbsz;
    gen->stackconfigs[i]->i = i;
    yale_hash_table_add_nogrow(&gen->stackconfigs_hash, &gen->stackconfigs[i]->node, hashval);
    gen->stackconfigcnt++;
    stackconfig_print(gen, i);
    //printf("Not found %d!\n", (int)sz);
  }
  return i;
}

int parsergen_is_terminal(struct ParserGen *gen, yale_uint_t x);


ssize_t max_stack_sz(struct ParserGen *gen, size_t *maxcbszptr)
{
  size_t maxsz = 1;
  size_t maxcbsz = 0;
  size_t i, j;
  struct stackconfigitem stack[YALE_UINT_MAX_LEGAL];
  size_t sz;
  size_t cbsz = 0;
  yale_uint_t a;
  gen->stackconfigcnt = 1;
  gen->stackconfigs[0] = parsergen_alloc(gen, sizeof(*gen->stackconfigs[0]));
  gen->stackconfigs[0]->stack = parsergen_alloc(gen, 1*sizeof(*stack));
  gen->stackconfigs[0]->stack[0].val = gen->start_state;
  gen->stackconfigs[0]->stack[0].cb = YALE_UINT_MAX_LEGAL;
  gen->stackconfigs[0]->sz = 1;
  gen->stackconfigs[0]->cbsz = 0;
  stackconfig_print(gen, 0);
  //printf("Start state is %d, terminal? %d\n", gen->start_state, parsergen_is_terminal(gen, gen->start_state));
  for (i = 0; i < gen->stackconfigcnt; i++)
  {
    struct stackconfig *current = gen->stackconfigs[i];
    if (current->sz > maxsz)
    {
      maxsz = current->sz;
    }
    if (current->cbsz > maxcbsz)
    {
      maxcbsz = current->cbsz;
    }
    if (current->sz > 0)
    {
      struct stackconfigitem last = current->stack[current->sz-1];
      if (last.val == YALE_UINT_MAX_LEGAL - 2)
      {
        if (current->cbsz == 0)
        {
          abort(); // Should never happen
        }
        stackconfig_append(gen, current->stack, current->sz-1, current->cbsz - 1);
        stacktransitionlast_print(gen, i);
        continue;
      }
      if (parsergen_is_terminal(gen, last.val) || last.val == YALE_UINT_MAX_LEGAL)
      {
        stackconfig_append(gen, current->stack, current->sz-1, current->cbsz);
        stacktransitionlast_print(gen, i);
        continue;
      }
      for (a = 0; a < gen->tokencnt; a++)
      {
        uint32_t hashval = lookuptbl_hash(last.val, a);
        struct yale_hash_list_node *node;
        YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
        {
          yale_uint_t rule;
          struct LookupTblEntry *e =
            YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
          if (e->nonterminal != last.val)
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
            cbsz = current->cbsz;
            if (sz + gen->rules[rule].itemcnt > YALE_UINT_MAX_LEGAL)
            {
              return -1;
            }
            if (last.cb != YALE_UINT_MAX_LEGAL)
            {
              if (cbsz + 1 > YALE_UINT_MAX_LEGAL)
              {
                return -1;
              }
              if (sz + gen->rules[rule].itemcnt + 1 > YALE_UINT_MAX_LEGAL)
              {
                return -1;
              }
              cbsz++;
              stack[sz].val = YALE_UINT_MAX_LEGAL - 2;
              stack[sz].cb = YALE_UINT_MAX_LEGAL;
              sz++;
            }
            for (j = 0; j < gen->rules[rule].itemcnt; j++)
            {
              struct ruleitem *it =
                &gen->rules[rule].rhs[gen->rules[rule].itemcnt-j-1];
              stack[sz].cb = YALE_UINT_MAX_LEGAL;
              if (it->is_action)
              {
                stack[sz].val = YALE_UINT_MAX_LEGAL;
              }
              else
              {
                stack[sz].val = it->value;
              }
              if (!parsergen_is_terminal(gen, it->value) && !it->is_action)
              {
                stack[sz].cb = it->cb;
              }
              sz++;
            }
            size_t tmp = stackconfig_append(gen, stack, sz, cbsz);
            stacktransitionarbitrary_print(gen, i, tmp, a);
          }
        }
      }
      for (a = YALE_UINT_MAX_LEGAL - 1; a < YALE_UINT_MAX_LEGAL; a++)
      {
        uint32_t hashval = lookuptbl_hash(last.val, a);
        struct yale_hash_list_node *node;
        YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
        {
          yale_uint_t rule;
          struct LookupTblEntry *e =
            YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
          if (e->nonterminal != last.val)
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
            cbsz = current->cbsz;
            if (sz + gen->rules[rule].itemcnt > YALE_UINT_MAX_LEGAL)
            {
              return -1;
            }
            if (last.cb != YALE_UINT_MAX_LEGAL)
            {
              if (cbsz + 1 > YALE_UINT_MAX_LEGAL)
              {
                return -1;
              }
              if (sz + gen->rules[rule].itemcnt + 1 > YALE_UINT_MAX_LEGAL)
              {
                return -1;
              }
              cbsz++;
              stack[sz].val = YALE_UINT_MAX_LEGAL - 2;
              stack[sz].cb = YALE_UINT_MAX_LEGAL;
              sz++;
            }
            for (j = 0; j < gen->rules[rule].itemcnt; j++)
            {
              struct ruleitem *it =
                &gen->rules[rule].rhs[gen->rules[rule].itemcnt-j-1];
              stack[sz].cb = YALE_UINT_MAX_LEGAL;
              if (it->is_action)
              {
                stack[sz].val = YALE_UINT_MAX_LEGAL;
              }
              else
              {
                stack[sz].val = it->value;
              }
              if (!parsergen_is_terminal(gen, it->value) && !it->is_action)
              {
                stack[sz].cb = it->cb;
              }
              sz++;
            }
            size_t tmp = stackconfig_append(gen, stack, sz, cbsz);
            stacktransitionarbitrary_print(gen, i, tmp, a);
          }
        }
      }
    }
  }
  *maxcbszptr = maxcbsz;
  return maxsz;
}

static int has_firstset(struct firstset_values *vals, yale_uint_t terminal)
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
  size_t tmpsz;
  ssize_t tmpssz;

  // Computing first-sets, step 1:
  // 1. initialize every FI(Ai) with the empty set
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
  // Computing first-sets, steps 2 and 3:
  // 2. add fi(wi) to FI(wi) for every rule Ai -> wi,
  //    where fi is defined as follows...
  // 3. add FI(wi) to FI(Ai) for every rule Ai -> wi
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
  // Computing follow-sets, step 2:
  // 2. if there is a rule of the form Aj -> w Ai w' then ...
  // And step 3:
  // 3. repeat step 2 until all FO sets stay the same
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
        firstrhsright = // fi(w')
          firstset_func2(gen, &gen->rules[i].rhsnoact[j+1], gen->rules[i].noactcnt - j - 1);
        // if terminal a is in fi(w'), then add a to FO(Ai)
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
        // if epsilon is in fi(w')
        if (has_firstset(&firstrhsright, gen->epsilon))
        {
          // ...then add FO(Aj) to FO(Ai)
          int changedtmp = 0;
          firstset2_update(gen, &gen->Fo2[rhsmid], &gen->Fo2[nonterminal], 0,
                           &changedtmp);
          if (changedtmp)
          {
            changed = 1;
          }
        }
        // if w' has length 0
        if (gen->rules[i].noactcnt == 0 || j == gen->rules[i].noactcnt - 1U)
        {
          // ...then add FO(Aj) to FO(Ai)
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
  // T[A,a] contains the rule A->w if and only if...
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
      struct firstset_entry2 *fi2 = firstset2_lookup(gen, gen->rules[i].rhsnoact, gen->rules[i].noactcnt);
      struct firstset_value *fval = NULL;
      int found;
      int conflict;
      struct firstset_values *fo2 = &gen->Fo2[A];
      if (has_firstset(&fi2->values, a)) // a is in FI(w)
      {
        found = 0;
        conflict = 0;
        for (j = 0; j < fi2->values.valuessz; j++)
        {
          if (fi2->values.values[j].token == a)
          {
            if (found)
            {
              conflict = 1;
            }
            fval = &fi2->values.values[j];
            found = 1;
          }
        }
        lookuptbl_put(gen, A, a, gen->rules[i].cond, i, fval->cbs, fval->cbsz, conflict);
      }
      fval = NULL;
      // ...or epsilon is in FI(w) and a is in FO(A)
      if (has_firstset(&fi2->values, gen->epsilon) && has_firstset(fo2, a))
      {
        found = 0;
        conflict = 0;
        for (j = 0; j < fo2->valuessz; j++)
        {
          if (fo2->values[j].token == a)
          {
            if (found)
            {
              conflict = 1;
            }
            fval = &fo2->values[j];
            found = 1;
          }
        }
        lookuptbl_put(gen, A, a, gen->rules[i].cond, i, fval->cbs, fval->cbsz, conflict);
      }
    }
    for (a = YALE_UINT_MAX_LEGAL - 1; a < YALE_UINT_MAX_LEGAL; a++)
    {
      struct firstset_entry2 *fi2 = firstset2_lookup(gen, gen->rules[i].rhsnoact, gen->rules[i].noactcnt);
      struct firstset_value *fval = NULL;
      int found;
      int conflict;
      struct firstset_values *fo2 = &gen->Fo2[A];
      if (has_firstset(&fi2->values, a)) // a is in FI(w)
      {
        found = 0;
        conflict = 0;
        for (j = 0; j < fi2->values.valuessz; j++)
        {
          if (fi2->values.values[j].token == a)
          {
            if (found)
            {
              conflict = 1;
            }
            fval = &fi2->values.values[j];
            found = 1;
          }
        }
        lookuptbl_put(gen, A, a, gen->rules[i].cond, i, fval->cbs, fval->cbsz, conflict);
      }
      fval = NULL;
      // ...or epsilon is in FI(w) and a is in FO(A)
      if (has_firstset(&fi2->values, gen->epsilon) && has_firstset(fo2, a))
      {
        found = 0;
        conflict = 0;
        for (j = 0; j < fo2->valuessz; j++)
        {
          if (fo2->values[j].token == a)
          {
            if (found)
            {
              conflict = 1;
            }
            fval = &fo2->values[j];
            found = 1;
          }
        }
        lookuptbl_put(gen, A, a, gen->rules[i].cond, i, fval->cbs, fval->cbsz, conflict);
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
    pick(gen->ns, gen->ds, gen->re_by_idx, &gen->pick_thoses[i], gen->priorities,
         gen->caseis);
  }
  collect(gen->pick_thoses, gen->pick_thoses_cnt, &gen->bufs, parsergen_alloc_fn, gen);
  for (i = 0; i < gen->pick_thoses_cnt; i++)
  {
    ssize_t curbt;
    curbt = maximal_backtrack(gen->pick_thoses[i].ds, 0, 250);
    if (curbt < 0)
    {
      printf("too long or unbounded DFA backtrack\n");
      exit(1);
    }
    if (gen->max_bt < curbt)
    {
      gen->max_bt = curbt;
    }
  }
  tmpssz = max_stack_sz(gen, &tmpsz);
  if (tmpssz < 0)
  {
    printf("Error: PDA stack overflow\n");
    exit(1);
  }
  gen->max_stack_size = tmpssz;
  gen->max_cb_stack_size = tmpsz;
  for (i = gen->tokencnt; i < gen->tokencnt + gen->nonterminalcnt; i++)
  {
    int ok = 1;
    yale_uint_t x;
    yale_uint_t ruleid = YALE_UINT_MAX_LEGAL;
    yale_uint_t cond = YALE_UINT_MAX_LEGAL;
    if (gen->nonterminal_conds[i].condcnt != 1)
    {
      continue;
    }
    if (gen->nonterminal_conds[i].conds[0].cond != YALE_UINT_MAX_LEGAL)
    {
      continue;
    }

    for (x = 0; x < gen->tokencnt; x++)
    {
      uint32_t hashval = lookuptbl_hash(i, x);
      struct yale_hash_list_node *node;
      YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
      {
        struct LookupTblEntry *e =
          YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
        if (e->nonterminal == i && e->terminal == x && e->cond == cond &&
            e->val != YALE_UINT_MAX_LEGAL)
        {
          if (ruleid == YALE_UINT_MAX_LEGAL)
          {
            ruleid = e->val;
          }
          if (ruleid != e->val)
          {
            ok = 0; // we could break but too complex...
          }
        }
      }
    }
    {
      uint32_t hashval = lookuptbl_hash(i, YALE_UINT_MAX_LEGAL-1);
      struct yale_hash_list_node *node;
      YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
      {
        struct LookupTblEntry *e =
          YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
        if (e->nonterminal == i && e->terminal == YALE_UINT_MAX_LEGAL-1 &&
            e->cond == cond && e->val != YALE_UINT_MAX_LEGAL)
        {
          if (ruleid == YALE_UINT_MAX_LEGAL)
          {
            ruleid = e->val;
          }
          if (ruleid != e->val)
          {
            ok = 0; // we could break but too complex...
          }
        }
      }
    }
    if (!ok)
    {
      continue;
    }
    gen->nonterminal_conds[i].is_shortcut = 1;
    gen->nonterminal_conds[i].shortcut_rule = ruleid;
  }
  for (i = gen->tokencnt; i < gen->tokencnt + gen->nonterminalcnt; i++)
  {
    yale_uint_t c, x;
    for (c = 0; c < gen->nonterminal_conds[i].condcnt; c++)
    {
      yale_uint_t cond = gen->nonterminal_conds[i].conds[c].cond;
      {
        uint32_t hashval = lookuptbl_hash(i, YALE_UINT_MAX_LEGAL-1);
        struct yale_hash_list_node *node;
        YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
        {
          struct LookupTblEntry *e =
            YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
          if (e->nonterminal == i && e->terminal == YALE_UINT_MAX_LEGAL-1 &&
              e->cond == cond &&
              e->val != YALE_UINT_MAX_LEGAL)
          {
            if (  (!gen->shortcutting || !gen->nonterminal_conds[i].is_shortcut)
                && e->conflict)
            {
              printf("Error: callback conflict\n");
              exit(1);
            }
            break;
          }
        }
      }
      for (x = 0; x < gen->tokencnt; x++)
      {
        uint32_t hashval = lookuptbl_hash(i, x);
        struct yale_hash_list_node *node;
        YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
        {
          struct LookupTblEntry *e =
            YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
          if (e->nonterminal == i && e->terminal == x &&
              e->cond == cond)
          {
            if (  (!gen->shortcutting || !gen->nonterminal_conds[i].is_shortcut)
                && e->conflict)
            {
              printf("Error: callback conflict\n");
              exit(1);
            }
            break;
          }
        }
      }
    }
  }
}

static void
if_bitnz_call_cbsflg(FILE *f, const char *indent, const char *parsername, const char *varname, const char *flagname)
{
  fprintf(f, "%sif (%s_bitnz(&%s) && 1)\n", indent, parsername, varname);
  fprintf(f, "%s{\n", indent);
  fprintf(f, "%s  ssize_t cbr2 = %s_call_cbsflg(pctx, &%s, blk, 0, %s, %s_callbacks);\n", indent, parsername, varname, flagname, parsername);
  fprintf(f, "%s  if (cbr2 != -EAGAIN)\n", indent);
  fprintf(f, "%s  {\n", indent);
  fprintf(f, "%s    return cbr2;\n", indent);
  fprintf(f, "%s  }\n", indent);
  fprintf(f, "%s}\n", indent);
}

static void
end_cbs(FILE *f, const char *indent)
{
  fprintf(f, "%sssize_t end_cbs_ret = end_cbs(pctx, blk);\n", indent);
  fprintf(f, "%sif (end_cbs_ret != -EAGAIN)\n", indent);
  fprintf(f, "%s{\n", indent);
  fprintf(f, "%s  return end_cbs_ret;\n", indent);
  fprintf(f, "%s}\n", indent);
}

void parsergen_dump_parser(struct ParserGen *gen, FILE *f)
{
  size_t i, j, X, x;
  size_t curidx = 0;
  int special_needed;
  yale_uint_t c;
  dump_chead(f, gen->parsername, gen->nofastpath, gen->max_cb_stack_size, gen->cbcnt);
  dump_collected(f, gen->parsername, &gen->bufs);
  for (i = 0; i < gen->pick_thoses_cnt; i++)
  {
    dump_one(f, gen->parsername, &gen->pick_thoses[i], &gen->numbershash, parsergen_alloc_fn, gen);
  }
  fprintf(f, "const parser_uint_t %s_num_terminals;\n", gen->parsername);
  fprintf(f, "ssize_t(*%s_callbacks[])(const char*, size_t, int, struct %s_parserctx*) = {\n", gen->parsername, gen->parsername);
  for (i = 0; i < gen->cbcnt; i++)
  {
    fprintf(f, "%s, ", gen->cbs[i].name);
  }
  fprints(f, "NULL};\n");
  fprintf(f, "struct %s_parserstatetblentry {\n", gen->parsername);
  fprints(f, "  const struct state *re;\n");
  fprintf(f, "  //const parser_uint_t rhs[%d];\n", gen->tokencnt);
  fprintf(f, "  const struct %s_callbacks cb2[%d];\n", gen->parsername, gen->tokencnt);
  fprintf(f, "  const struct %s_callbacks bytes_cb2;\n", gen->parsername);
  fprints(f, "  const uint8_t special_flags;\n");
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
  for (X = gen->tokencnt; X < gen->tokencnt + gen->nonterminalcnt; X++)
  {
    for (c = 0; c < gen->nonterminal_conds[X].condcnt; c++)
    {
      yale_uint_t cond = gen->nonterminal_conds[X].conds[c].cond;
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
            //numbers_sets_emit(f, &gen->numbershash, &e->cbs, parsergen_alloc_fn, gen);
            break;
          }
        }
      }
      for (x = 0; x < gen->tokencnt; x++)
      {
        uint32_t hashval = lookuptbl_hash(X, x);
        struct yale_hash_list_node *node;
        YALE_HASH_TABLE_FOR_EACH_POSSIBLE(&gen->Thash, node, hashval)
        {
          struct LookupTblEntry *e =
            YALE_CONTAINER_OF(node, struct LookupTblEntry, node);
          if (e->nonterminal == X && e->terminal == x &&
              e->cond == cond)
          {
            //numbers_sets_emit(f, &gen->numbershash, &e->cbs, parsergen_alloc_fn, gen);
            break;
          }
        }
      }
    }
  }
  fprintf(f, "const struct %s_parserstatetblentry %s_parserstatetblentries[] = {\n", gen->parsername, gen->parsername);
  for (X = gen->tokencnt; X < gen->tokencnt + gen->nonterminalcnt; X++)
  {
    for (c = 0; c < gen->nonterminal_conds[X].condcnt; c++)
    {
      yale_uint_t cond = gen->nonterminal_conds[X].conds[c].cond;
      gen->nonterminal_conds[X].conds[c].statetblidx = curidx++;
      int is_bytes = 0, is_re = 0;
      //yale_uint_t bytes_cb = YALE_UINT_MAX_LEGAL;
      struct bitset bytes_cbs = {};
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
            //bytes_cb = e->cb;
            bytes_cbs = e->cbs;
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
        printf("Error: state accepts both regexp and bytes tokens\n");
        printf("Can't know which one to choose\n");
        exit(1);
      }
      fprints(f, "{\n");
      if (is_bytes)
      {
        size_t cbi;
        fprintf(f, ".special_flags = YALE_SPECIAL_FLAG_BYTES, ");
        fprintf(f, ".bytes_cb2 = {\n");
        fprintf(f, ".cbsmask = {\n");
        fprintf(f, ".elems = {\n");
        for (cbi = 0; cbi < ((gen->cbcnt+63)/64); cbi++)
        {
          fprintf(f, "0x%llx,\n", (unsigned long long)bytes_cbs.bitset[cbi]);
        }
        fprintf(f, "},},},\n");
      }
      else
      {
        int emit_cb = 1;
        if (gen->shortcutting && gen->nonterminal_conds[X].is_shortcut)
        {
          fprintf(f, ".special_flags = YALE_SPECIAL_FLAG_SHORTCUT, ");
          emit_cb = 0;
        }
        else
        {
          fprintf(f, ".special_flags = 0, ");
        }
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
        if (emit_cb)
        {
          fprints(f, ".cb2 = {\n");
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
                size_t cbi;
                fprintf(f, "{.cbsmask = { .elems = {");
                for (cbi = 0; cbi < ((gen->cbcnt+63)/64); cbi++)
                {
                  fprintf(f, "0x%llx,\n", (unsigned long long)e->cbs.bitset[cbi]);
                }
                fprintf(f, "}, }, }, ");
                found = 1;
                break;
              }
            }
            if (!found)
            {
              fprintf(f, "{.cbsmask = {}}, ");
            }
          }
          fprints(f, "},\n");
        }
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
          //check_cb(gen->pick_thoses[it->value].ds, 0, it->value);
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
  fprintf(f, "                const char *blkoff, size_t szoff, int eofindicator, parser_uint_t *state,\n"
             "                const struct %s_callbacks *cb2, parser_uint_t cb1)//, void *baton)\n"
             "{\n"
             "  if (pctx->saved_token != PARSER_UINT_MAX)\n"
             "  {\n"
             "    *state = pctx->saved_token;\n"
             "    pctx->saved_token = PARSER_UINT_MAX;\n"
             "    return 0;\n"
             "  }\n", gen->parsername);
  if (gen->max_cb_stack_size)
  {
    fprintf(f, "  return %s_feed_statemachine(&pctx->rctx, restates, blkoff, szoff, eofindicator, state, %s_callbacks, cb2, cb1, pctx->cbstack, pctx->cbstacksz);//, baton);\n", gen->parsername, gen->parsername);
  }
  else
  {
    fprintf(f, "  return %s_feed_statemachine(&pctx->rctx, restates, blkoff, szoff, eofindicator, state, %s_callbacks, cb2, cb1);//, baton);\n", gen->parsername, gen->parsername);
  }
  fprints(f, "}\n");
  fprintf(f, "static ssize_t end_cbs(struct %s_parserctx *pctx, const void *blk)\n", gen->parsername);
  fprints(f, "{\n");
  fprintf(f, "  struct %s_cbset endmask = {}, mismask = {};\n", gen->parsername);
  fprintf(f, "  %s_bitcopy(&endmask, &pctx->rctx.confirm_status);\n", gen->parsername);
  if_bitnz_call_cbsflg(f, "  ", gen->parsername, "endmask", "YALE_FLAG_END");
  fprintf(f, "  %s_bitcopy(&mismask, &pctx->rctx.start_status);\n", gen->parsername);
  fprintf(f, "  %s_bitandnot(&mismask, &pctx->rctx.confirm_status);\n", gen->parsername);
  if_bitnz_call_cbsflg(f, "  ", gen->parsername, "mismask", "YALE_FLAG_MAJOR_MISTAKE");
  fprintf(f, "  return -EAGAIN;\n");
  fprints(f, "}\n");
  fprints(f, "\n"
             "#define EXTRA_SANITY\n"
             "\n");
  fprintf(f, "ssize_t %s_parse_block(struct %s_parserctx *pctx, const char *blk, size_t sz, int eofindicator)//, void *baton)\n", gen->parsername, gen->parsername);
  fprintf(f, "{\n"
             "  size_t off = 0;\n"
             "  ssize_t ret;\n"
             "  parser_uint_t curstateoff;\n"
             "  parser_uint_t curstate;\n"
             "  parser_uint_t curcb;\n"
             "  parser_uint_t state;\n"
             "  parser_uint_t ruleid;\n"
             "  parser_uint_t i; // FIXME is 8 bits enough?\n"
             "  parser_uint_t cb1;\n"
             "  const struct state *restates;\n"
             "  const struct rule *rule;\n"
             "  const struct %s_callbacks *cb2;\n", gen->parsername);
  fprintf(f, "  ssize_t (*cb1f)(const char *, size_t, int, struct %s_parserctx*);\n", gen->parsername);
  fprints(f, "\n"
             "  for (;;)\n"
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
             "        if (unlikely(eofindicator))\n"
             "        {\n");
  end_cbs(f, "          ");
  fprints(f, "        }\n"
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
             "    curstate = pctx->stack[pctx->stacksz - 1].rhs;\n"
             "    curcb = pctx->stack[pctx->stacksz - 1].cb;\n");
  if (gen->bytes_size_type == NULL || strcmp(gen->bytes_size_type, "void") != 0)
  {
    fprints(f, "    if (curstate == PARSER_UINT_MAX - 1)\n");
    fprints(f, "    {\n");
    fprintf(f, "      struct %s_cbset cbmask = {}, endmask = {}, mismask = {};\n", gen->parsername);
    fprintf(f, "      ssize_t cbr;\n");
    if (gen->max_cb_stack_size)
    {
      fprintf(f, "      size_t cbidx;\n");
    }
    //fprintf(f, "      uint16_t bitoff;\n");
    fprints(f, "      parser_uint_t bytes_cb = curcb;\n");
    fprintf(f, "      ret = pctx->bytes_sz < (sz-off) ? pctx->bytes_sz : (sz-off);\n");
#if 0
    fprints(f, "      if (bytes_cb != PARSER_UINT_MAX)\n");
    fprints(f, "      {\n");
    fprints(f, "        cbmask |= 1ULL<<bytes_cb;\n");
    fprints(f, "      }\n");
#endif
    fprintf(f, "      %s_bitmaybeset(&cbmask, bytes_cb);\n", gen->parsername);
    if (gen->max_cb_stack_size)
    {
      fprintf(f, "      for (cbidx = 0; cbidx < pctx->cbstacksz; cbidx++)\n");
      fprintf(f, "      {\n");
      fprintf(f, "        %s_bitset(&cbmask, pctx->cbstack[cbidx]);\n", gen->parsername);
      fprintf(f, "      }\n");
    }
    fprintf(f, "      if (%s_bitnz(&cbmask))\n", gen->parsername);
    fprintf(f, "      {\n");
    fprintf(f, "        cbr = %s_call_cbs1(pctx, &cbmask, blk+off, ret, %s_callbacks);\n", gen->parsername, gen->parsername);
    fprintf(f, "        if (cbr != -EAGAIN)\n");
    fprintf(f, "        {\n");
    fprintf(f, "          return cbr;\n");
    fprintf(f, "        }\n");
    fprintf(f, "      }\n");
    fprintf(f, "      %s_bitcopy(&endmask, &pctx->rctx.confirm_status);\n", gen->parsername);
    fprintf(f, "      %s_bitandnot(&endmask, &cbmask);\n", gen->parsername);
    fprintf(f, "      if (%s_bitnz(&endmask))\n", gen->parsername);
    fprintf(f, "      {\n");
    fprintf(f, "        cbr = %s_call_cbsflg(pctx, &endmask, blk+off, 0, YALE_FLAG_END, %s_callbacks);\n", gen->parsername, gen->parsername);
    fprintf(f, "        if (cbr != -EAGAIN)\n");
    fprintf(f, "        {\n");
    fprintf(f, "          return cbr;\n");
    fprintf(f, "        }\n");
    fprintf(f, "      }\n");
    fprintf(f, "      %s_bitcopy(&endmask, &pctx->rctx.start_status);\n", gen->parsername);
    fprintf(f, "      %s_bitandnot(&endmask, &cbmask);\n", gen->parsername);
    fprintf(f, "      %s_bitandnot(&endmask, &pctx->rctx.confirm_status);\n", gen->parsername);
    fprintf(f, "      if (%s_bitnz(&mismask))\n", gen->parsername);
    fprintf(f, "      {\n");
    fprintf(f, "        cbr = %s_call_cbsflg(pctx, &mismask, blk+off, 0, YALE_FLAG_MAJOR_MISTAKE, %s_callbacks);\n", gen->parsername, gen->parsername);
    fprintf(f, "        if (cbr != -EAGAIN)\n");
    fprintf(f, "        {\n");
    fprintf(f, "          return cbr;\n");
    fprintf(f, "        }\n");
    fprintf(f, "      }\n");
    fprintf(f, "      %s_bitor(&pctx->rctx.start_status, &cbmask);\n", gen->parsername);
    fprintf(f, "      %s_bitor(&pctx->rctx.confirm_status, &cbmask);\n", gen->parsername);
    fprintf(f, "      %s_bitandnot(&pctx->rctx.start_status, &endmask);\n", gen->parsername);
    fprintf(f, "      %s_bitandnot(&pctx->rctx.confirm_status, &endmask);\n", gen->parsername);
    fprintf(f, "      %s_bitandnot(&pctx->rctx.lastack_status, &endmask);\n", gen->parsername);
    fprintf(f, "      %s_bitandnot(&pctx->rctx.start_status, &mismask);\n", gen->parsername);
    fprintf(f, "      %s_bitandnot(&pctx->rctx.lastack_status, &mismask);\n", gen->parsername);
    fprintf(f, "      pctx->bytes_sz -= ret;\n");
    fprintf(f, "      off += ret;\n");
    fprintf(f, "      if (pctx->bytes_sz)\n");
    fprintf(f, "      {\n");
    fprintf(f, "        //off = sz;\n");
    fprints(f, "        if (unlikely(eofindicator))\n"
               "        {\n");
    end_cbs(f, "          ");
    fprints(f, "        }\n");
    fprintf(f, "        return -EAGAIN;\n");
    fprintf(f, "      }\n");
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
  fprints(f, "      cb1 = curcb;\n");
  fprintf(f, "      ret = %s_get_saved_token(pctx, restates, blk+off, sz-off, eofindicator, &state, NULL, cb1);//, baton);\n", gen->parsername);
  fprints(f, "      if (ret == -EAGAIN)\n"
             "      {\n"
             "        //off = sz;\n"
             "        if (unlikely(eofindicator))\n"
             "        {\n");
  end_cbs(f, "          ");
  fprints(f, "        }\n"
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
  special_needed = 0;
  if (gen->bytes_size_type == NULL || strcmp(gen->bytes_size_type, "void") != 0)
  {
    special_needed = 1;
  }
  if (gen->shortcutting)
  {
    special_needed = 1;
  }
  if (special_needed)
  {
    fprintf(f, "      uint8_t special_flags;\n");
  }
  fprintf(f, "      if (pctx->curstateoff == PARSER_UINT_MAX)\n");
  fprintf(f, "      {\n");
  fprintf(f, "        switch (curstate)\n");
  fprintf(f, "        {\n");
  for (X = gen->tokencnt; X < gen->tokencnt + gen->nonterminalcnt; X++)
  {
    fprintf(f, "          case %d:\n", (int)X);
    for (c = 0; c < gen->nonterminal_conds[X].condcnt; c++)
    {
      if (gen->nonterminal_conds[X].conds[c].cond == YALE_UINT_MAX_LEGAL)
      {
        fprintf(f, "            pctx->curstateoff = %d;\n", (int)gen->nonterminal_conds[X].conds[c].statetblidx);
      }
    }
    for (c = 0; c < gen->nonterminal_conds[X].condcnt; c++)
    {
      if (gen->nonterminal_conds[X].conds[c].cond != YALE_UINT_MAX_LEGAL)
      {
        fprintf(f, "            if (%s)\n", gen->conds[gen->nonterminal_conds[X].conds[c].cond]);
        fprintf(f, "            {\n");
        fprintf(f, "              pctx->curstateoff = %d;\n", (int)gen->nonterminal_conds[X].conds[c].statetblidx);
        fprintf(f, "            }\n");
      }
    }
    fprintf(f, "            break;\n");
  }
  fprintf(f, "          default:\n");
  fprintf(f, "            abort();\n");
  fprintf(f, "        }\n");
  fprintf(f, "      }\n");
  fprintf(f, "      curstateoff = pctx->curstateoff;\n");
  //fprintf(f, "      curstateoff = curstate - %s_num_terminals;\n", gen->parsername);
  fprintf(f, "      if (curstateoff == PARSER_UINT_MAX)\n");
  fprintf(f, "      {\n");
  fprintf(f, "        return -EINVAL;\n");
  fprintf(f, "      }\n");
  fprintf(f, "      restates = %s_parserstatetblentries[curstateoff].re;\n", gen->parsername);
  fprintf(f, "      cb2 = %s_parserstatetblentries[curstateoff].cb2;\n", gen->parsername);
  if (special_needed)
  {
    fprintf(f, "      special_flags = %s_parserstatetblentries[curstateoff].special_flags;\n", gen->parsername);
    fprintf(f, "      if (unlikely(special_flags))\n");
    fprintf(f, "      {\n");
    fprintf(f, "      if (special_flags & YALE_SPECIAL_FLAG_BYTES)\n");
    fprintf(f, "      {\n");
    if (gen->bytes_size_type == NULL || strcmp(gen->bytes_size_type, "void") != 0)
    {
      //fprintf(f, "        parser_uint_t bytes_cb = %s_parserstatetblentries[curstateoff].bytes_cb;\n", gen->parsername);
      fprintf(f, "        struct %s_cbset cbmask = {}, endmask = {}, mismask = {};\n", gen->parsername);
      fprintf(f, "        ssize_t cbr;\n");
      if (gen->max_cb_stack_size)
      {
        fprintf(f, "        size_t cbidx;\n");
      }
      //fprintf(f, "        uint16_t bitoff;\n");
      fprintf(f, "        const struct %s_callbacks *bytes_cb2 = &%s_parserstatetblentries[curstateoff].bytes_cb2;\n", gen->parsername, gen->parsername);
      fprintf(f, "        ret = pctx->bytes_sz < (sz-off) ? pctx->bytes_sz : (sz-off);\n");
#if 0
      fprintf(f, "        if (curcb != PARSER_UINT_MAX)\n");
      fprintf(f, "        {\n");
      fprintf(f, "          cbmask |= 1ULL<<curcb;\n");
      fprintf(f, "        }\n");
#endif
      fprintf(f, "        %s_bitmaybeset(&cbmask, curcb);\n", gen->parsername);
      fprintf(f, "        %s_bitor(&cbmask, &bytes_cb2->cbsmask);\n", gen->parsername);
      //fprintf(f, "        cbmask |= bytes_cb2->cbsmask;\n");
      if (gen->max_cb_stack_size)
      {
        fprintf(f, "        for (cbidx = 0; cbidx < pctx->cbstacksz; cbidx++)\n");
        fprintf(f, "        {\n");
        fprintf(f, "          %s_bitset(&cbmask, pctx->cbstack[cbidx]);\n", gen->parsername);
        fprintf(f, "        }\n");
      }
      fprintf(f, "        if (%s_bitnz(&cbmask))\n", gen->parsername);
      fprintf(f, "        {\n");
      fprintf(f, "          cbr = %s_call_cbs1(pctx, &cbmask, blk+off, ret, %s_callbacks);\n", gen->parsername, gen->parsername);
      fprintf(f, "          if (cbr != -EAGAIN)\n");
      fprintf(f, "          {\n");
      fprintf(f, "            return cbr;\n");
      fprintf(f, "          }\n");
      fprintf(f, "        }\n");
      fprintf(f, "        %s_bitcopy(&endmask, &pctx->rctx.confirm_status);\n", gen->parsername);
      fprintf(f, "        %s_bitandnot(&endmask, &cbmask);\n", gen->parsername);
      fprintf(f, "        if (%s_bitnz(&endmask))\n", gen->parsername);
      fprintf(f, "        {\n");
      fprintf(f, "          cbr = %s_call_cbsflg(pctx, &endmask, blk+off, 0, YALE_FLAG_END, %s_callbacks);\n", gen->parsername, gen->parsername);
      fprintf(f, "          if (cbr != -EAGAIN)\n");
      fprintf(f, "          {\n");
      fprintf(f, "            return cbr;\n");
      fprintf(f, "          }\n");
      fprintf(f, "        }\n");
      fprintf(f, "        %s_bitcopy(&endmask, &pctx->rctx.start_status);\n", gen->parsername);
      fprintf(f, "        %s_bitandnot(&endmask, &cbmask);\n", gen->parsername);
      fprintf(f, "        %s_bitandnot(&endmask, &pctx->rctx.confirm_status);\n", gen->parsername);
      fprintf(f, "        if (%s_bitnz(&mismask))\n", gen->parsername);
      fprintf(f, "        {\n");
      fprintf(f, "          cbr = %s_call_cbsflg(pctx, &mismask, blk+off, 0, YALE_FLAG_MAJOR_MISTAKE, %s_callbacks);\n", gen->parsername, gen->parsername);
      fprintf(f, "          if (cbr != -EAGAIN)\n");
      fprintf(f, "          {\n");
      fprintf(f, "            return cbr;\n");
      fprintf(f, "          }\n");
      fprintf(f, "        }\n");
      fprintf(f, "        %s_bitor(&pctx->rctx.start_status, &cbmask);\n", gen->parsername);
      fprintf(f, "        %s_bitor(&pctx->rctx.confirm_status, &cbmask);\n", gen->parsername);
      fprintf(f, "        %s_bitandnot(&pctx->rctx.start_status, &endmask);\n", gen->parsername);
      fprintf(f, "        %s_bitandnot(&pctx->rctx.confirm_status, &endmask);\n", gen->parsername);
      fprintf(f, "        %s_bitandnot(&pctx->rctx.lastack_status, &endmask);\n", gen->parsername);
      fprintf(f, "        %s_bitandnot(&pctx->rctx.start_status, &mismask);\n", gen->parsername);
      fprintf(f, "        %s_bitandnot(&pctx->rctx.lastack_status, &mismask);\n", gen->parsername);
      fprintf(f, "        pctx->bytes_sz -= ret;\n");
      fprintf(f, "        off += ret;\n");
      fprintf(f, "        if (pctx->bytes_sz)\n");
      fprintf(f, "        {\n");
      fprintf(f, "          //off = sz;\n");
      fprints(f, "          if (unlikely(eofindicator))\n"
                 "          {\n");
      end_cbs(f, "            ");
      fprints(f, "          }\n");
      fprintf(f, "          return -EAGAIN;\n");
      fprintf(f, "        }\n");
      fprintf(f, "        state = PARSER_UINT_MAX-1;\n");
    }
    else
    {
      fprintf(f, "        abort();\n");
    }
    fprintf(f, "      }\n");
    if (gen->shortcutting)
    {
      fprintf(f, "      else if (special_flags & YALE_SPECIAL_FLAG_SHORTCUT)\n");
      fprintf(f, "      {\n");
      fprintf(f, "        state = PARSER_UINT_MAX;\n");
      fprintf(f, "      }\n");
    }
    fprintf(f, "      else\n");
    fprintf(f, "      {\n");
    fprintf(f, "        abort();\n");
    fprintf(f, "      }\n");
    fprintf(f, "      }\n");
    fprintf(f, "      else\n");
  }
  fprintf(f, "      {\n");
  fprintf(f, "        ret = %s_get_saved_token(pctx, restates, blk+off, sz-off, eofindicator, &state, cb2, curcb);//, baton);\n", gen->parsername);
  fprints(f, "        if (ret == -EAGAIN)\n"
             "        {\n"
             "          //off = sz;\n"
             "          if (unlikely(eofindicator))\n"
             "          {\n");
  end_cbs(f, "            ");
  fprints(f, "          }\n"
             "          return -EAGAIN;\n"
             "        }\n"
             "        else if (ret < 0 || state == PARSER_UINT_MAX)\n"
             "        {\n"
             "          //fprintf(stderr, \"Parser error: tokenizer error, curstate=%%d, token=%%d\\n\", (int)curstate, (int)state);\n");
  fprints(f, "          //exit(1);\n"
             "          if (ret == 0)\n"
             "          {\n"
             "            if (unlikely(eofindicator))\n"
             "            {\n");
  end_cbs(f, "              ");
  fprints(f, "            }\n"
             "            return 0;\n"
             "          }\n"
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
    if (gen->shortcutting && gen->nonterminal_conds[X].is_shortcut)
    {
      fprintf(f, "          ruleid=%d;\n", (int)gen->nonterminal_conds[X].shortcut_rule);
    }
    else
    {
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
        yale_uint_t statetblidx = gen->nonterminal_conds[X].conds[c].statetblidx;
        if (cond != YALE_UINT_MAX_LEGAL)
        {
          fprintf(f, "        if (curstateoff == %d)\n", (int)statetblidx);
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
             "      if ((size_t)pctx->stacksz + rule->rhssz + (curcb != PARSER_UINT_MAX) > sizeof(pctx->stack)/sizeof(struct ruleentry))\n");
  fprints(f, "      {\n"
             "        abort();\n"
             "      }\n"
             "      i = 0;\n"
             "      pctx->curstateoff = PARSER_UINT_MAX;\n"
             "      if (curcb != PARSER_UINT_MAX)\n"
             "      {\n");
  if (gen->max_cb_stack_size)
  {
    fprints(f, "        if ((size_t)pctx->cbstacksz + 1 > sizeof(pctx->cbstack)/sizeof(*pctx->cbstack))\n"
               "        {\n"
               "          abort();\n"
               "        }\n"
               "        pctx->stack[pctx->stacksz].rhs = PARSER_UINT_MAX;\n"
               "        pctx->stack[pctx->stacksz].cb = PARSER_UINT_MAX;\n"
               "        pctx->cbstack[pctx->cbstacksz++] = curcb;\n"
               "        pctx->stacksz++;\n");
  }
  else
  {
    fprints(f, "        abort();\n");
  }
  fprints(f, "      }\n"
             "#if 0\n"
             "      while (i + 4 <= rule->rhssz)\n"
             "      {\n"
             "        pctx->stack[pctx->stacksz++] = rule->rhs[i+0];\n"
             "        pctx->stack[pctx->stacksz++] = rule->rhs[i+1];\n"
             "        pctx->stack[pctx->stacksz++] = rule->rhs[i+2];\n");
  fprints(f, "        pctx->stack[pctx->stacksz++] = rule->rhs[i+3];\n"
             "        i += 4;\n"
             "      }\n"
             "#endif\n"
             "      for (; i < rule->rhssz; i++)\n"
             "      {\n"
             "        pctx->stack[pctx->stacksz++] = rule->rhs[i];\n"
             "      }\n");
  fprintf(f, "      if (state != PARSER_UINT_MAX && rule->rhssz && (pctx->stack[pctx->stacksz-1].rhs < %s_num_terminals || pctx->stack[pctx->stacksz-1].rhs == PARSER_UINT_MAX-1))\n", gen->parsername);
  fprints(f, "      {\n"
             "        pctx->stacksz--; // Has to be correct token so let's process immediately\n"
             "      }\n"
             "      else\n"
             "      {\n"
             "        pctx->saved_token = state;\n"
             "      }\n"
             "    }\n"
             "    else // if (curstate == PARSER_UINT_MAX)\n"
             "    {\n"
             "      if (curcb == PARSER_UINT_MAX)\n"
             "      {\n");
  if (gen->max_cb_stack_size)
  {
    fprints(f, "        pctx->stacksz--;\n"
               "        if (pctx->cbstacksz == 0)\n"
               "        {\n"
               "          abort();\n"
               "        }\n"
               "        pctx->cbstacksz--;\n");
  }
  else
  {
    fprints(f, "        abort();\n");
  }
  fprints(f, "      }\n"
             "      else\n"
             "      {\n"
             "        ssize_t cbr;\n");
  fprintf(f, "        cb1f = %s_callbacks[curcb];\n", gen->parsername);
  fprints(f, "        cbr = cb1f(NULL, 0, YALE_FLAG_ACTION, pctx);//, baton);\n"
             "        if (cbr != 0 && cbr != -EAGAIN && cbr != -EWOULDBLOCK)\n"
             "        {\n"
             "          return cbr;\n"
             "        }\n"
             "        pctx->stacksz--;\n"
             "      }\n"
             "    }\n"
             "    if (off >= sz && pctx->saved_token == PARSER_UINT_MAX)\n"
             "    {\n"
             "      break;\n"
             "    }\n"
             "  }\n"
             "  if (unlikely(eofindicator))\n"
             "  {\n");
  end_cbs(f, "    ");
  fprints(f, "  }\n"
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
  const char *cbbitmasktype = NULL;
  fprints(f, "#include \"yalecommon.h\"\n");
  if (gen->cbcnt <= 8)
  {
    cbbitmasktype = "uint8_t";
  }
  else if (gen->cbcnt <= 16)
  {
    cbbitmasktype = "uint16_t";
  }
  else if (gen->cbcnt <= 32)
  {
    cbbitmasktype = "uint32_t";
  }
  else if (gen->cbcnt <= 64)
  {
    cbbitmasktype = "uint64_t";
  }
  else if (gen->cbcnt <= 255)
  {
    cbbitmasktype = "uint64_t";
  }
  else
  {
    printf("Too many callbacks, maximum is 255\n");
    exit(1);
  }
  dump_headers(f, gen->parsername, gen->max_bt, gen->max_cb_stack_size, cbbitmasktype, ((gen->cbcnt+63)/64));
  fprintf(f, "struct %s_parserctx {\n", gen->parsername);
  if (gen->bytes_size_type == NULL || strcmp(gen->bytes_size_type, "void") != 0)
  {
    fprintf(f, "  %s bytes_sz;\n", gen->bytes_size_type ? gen->bytes_size_type : "uint64_t");
  }
  fprints(f, "  parser_uint_t stacksz;\n");
  if (gen->max_cb_stack_size)
  {
    fprints(f, "  parser_uint_t cbstacksz;\n");
  }
  fprints(f, "  parser_uint_t saved_token;\n");
  fprints(f, "  parser_uint_t curstateoff;\n");
  fprintf(f, "  struct ruleentry stack[%d];\n", gen->max_stack_size);
  if (gen->max_cb_stack_size)
  {
    fprintf(f, "  parser_uint_t cbstack[%d];\n", gen->max_cb_stack_size);
  }
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
  fprints(f, "  pctx->curstateoff = PARSER_UINT_MAX;\n");
  if (gen->bytes_size_type == NULL || strcmp(gen->bytes_size_type, "void") != 0)
  {
    fprints(f, "  pctx->bytes_sz = 0;\n");
  }
  if (gen->max_cb_stack_size)
  {
    fprints(f, "  pctx->cbstacksz = 0;\n");
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
  fprintf(f, "ssize_t %s_parse_block(struct %s_parserctx *pctx, const char *blk, size_t sz, int eofindicator);//, void *baton);\n", gen->parsername, gen->parsername);
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

yale_uint_t parsergen_add_token(struct ParserGen *gen, char *re, size_t resz, int prio, int casei)
{
  if (gen->tokens_finalized)
  {
    abort();
  }
  // last valid token YALE_UINT_MAX_LEGAL-3
  // token YALE_UINT_MAX_LEGAL-2 is "pop from cb stack"
  // token YALE_UINT_MAX_LEGAL-1 is bytes
  // token YALE_UINT_MAX_LEGAL is action
  if (gen->tokencnt >= YALE_UINT_MAX_LEGAL - 2)
  {
    printf("Error: too many tokens, can't add token\n");
    exit(1);
  }
  gen->re_by_idx[gen->tokencnt].iov_base = memdup(re, resz);
  gen->re_by_idx[gen->tokencnt].iov_len = resz;
  gen->priorities[gen->tokencnt] = prio;
  gen->caseis[gen->tokencnt] = casei;
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
  // last valid nonterminal YALE_UINT_MAX_LEGAL-3
  // token YALE_UINT_MAX_LEGAL-2 is "pop from cb stack"
  // token YALE_UINT_MAX_LEGAL-1 is bytes
  // token YALE_UINT_MAX_LEGAL is action
  if (gen->tokencnt + gen->nonterminalcnt >= YALE_UINT_MAX_LEGAL - 2)
  {
    printf("Error: too many tokens+nonterminals, can't add nonterminal\n");
    exit(1);
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
        if (gen->rules[i].rhs[j].cb == YALE_UINT_MAX_LEGAL)
        {
          printf("Error: action without callback\n");
          exit(1);
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
  if (cbcnt > 255)
  {
    printf("Too many callbacks: current maximum is 255\n");
    exit(1);
  }
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

void parsergen_shortcutting(struct ParserGen *gen)
{
  gen->shortcutting = 1;
}
