#include "yale.h"
#include "parser.h"
#include <sys/uio.h>

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
  gen->max_bt = 0;
  gen->stackconfigcnt = 0;
  memset(&gen->re_gen, 0, sizeof(gen->re_gen));
  memset(gen->T, 0xff, sizeof(gen->T));
  //memset(gen->Fo, 0, sizeof(gen->Fo)); // This is the overhead!
  gen->Ficnt = 0;
  gen->pick_thoses_cnt = 0;
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

void stackconfig_append(struct ParserGen *gen, const uint8_t *stack, uint8_t sz)
{
  size_t i;
  for (i = 0; i < gen->stackconfigcnt; i++)
  {
    if (gen->stackconfigs[i].sz != sz)
    {
      continue;
    }
    if (memcmp(gen->stackconfigs[i].stack, stack, sz*sizeof(uint8_t)) != 0)
    {
      continue;
    }
    //printf("Found %d!\n", (int)sz);
    break;
  }
  if (i == gen->stackconfigcnt)
  {
    if (gen->stackconfigcnt == sizeof(gen->stackconfigs)/sizeof(*gen->stackconfigs))
    {
      abort();
    }
    memcpy(gen->stackconfigs[i].stack, stack, sz*sizeof(uint8_t));
    gen->stackconfigs[i].sz = sz;
    gen->stackconfigcnt++;
    //printf("Not found %d!\n", (int)sz);
  }
}

int parsergen_is_terminal(struct ParserGen *gen, uint8_t x);

ssize_t max_stack_sz(struct ParserGen *gen)
{
  size_t maxsz = 1;
  size_t i, j;
  uint8_t stack[255];
  size_t sz;
  uint8_t a;
  gen->stackconfigcnt = 1;
  gen->stackconfigs[0].stack[0] = gen->start_state;
  gen->stackconfigs[0].sz = 1;
  //printf("Start state is %d, terminal? %d\n", gen->start_state, parsergen_is_terminal(gen, gen->start_state));
  for (i = 0; i < gen->stackconfigcnt; i++)
  {
    struct stackconfig *current = &gen->stackconfigs[i];
    if (current->sz > maxsz)
    {
      maxsz = current->sz;
    }
    if (current->sz > 0)
    {
      uint8_t last = current->stack[current->sz-1];
      if (parsergen_is_terminal(gen, last) || last == 255)
      {
        stackconfig_append(gen, current->stack, current->sz-1);
        continue;
      }
      for (a = 0; a < gen->tokencnt; a++)
      {
        uint8_t rule = gen->T[last][a].val;
        if (rule != 255)
        {
          //printf("Rule differs from 255\n");
          memcpy(stack, current->stack, current->sz-1);
          sz = current->sz - 1;
          if (sz + gen->rules[rule].itemcnt > 255)
          {
            return -1;
          }
          for (j = 0; j < gen->rules[rule].itemcnt; j++)
          {
            struct ruleitem *it =
              &gen->rules[rule].rhs[gen->rules[rule].itemcnt-j-1];
            if (it->is_action)
            {
              stack[sz++] = 255;
            }
            else
            {
              stack[sz++] = it->value;
            }
          }
          stackconfig_append(gen, stack, sz);
        }
      }
    }
  }
  return maxsz;
}

void gen_parser(struct ParserGen *gen)
{
  int changed;
  size_t i, j;

  memset(gen->Fo, 0, sizeof(gen->Fo[0])*(gen->tokencnt + gen->nonterminalcnt));

  for (i = gen->tokencnt; i < gen->tokencnt + gen->nonterminalcnt; i++)
  {
    gen->Fi[gen->Ficnt].rhssz = 1;
    gen->Fi[gen->Ficnt].rhs[0] = i;
    memset(&gen->Fi[gen->Ficnt].dict, 0, sizeof(gen->Fi[gen->Ficnt].dict));
    gen->Ficnt++;
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
        if (gen->rules[i].noactcnt == 0 || j == gen->rules[i].noactcnt - 1U)
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
    size_t len = 0;
    for (j = 0; j < gen->tokencnt; j++)
    {
      if (gen->T[i][j].val != 255)
      {
        if (len >= 255)
        {
          abort();
        }
        gen->pick_those[gen->pick_thoses_cnt][len++] = j;
      }
    }
    gen->pick_thoses[gen->pick_thoses_cnt].pick_those =
      gen->pick_those[gen->pick_thoses_cnt];
    gen->pick_thoses[gen->pick_thoses_cnt].len = len;
    gen->pick_thoses[gen->pick_thoses_cnt].ds = NULL;
    gen->pick_thoses[gen->pick_thoses_cnt].dscnt = 0;
    gen->pick_thoses_cnt++;
  }
  for (i = 0; i < gen->pick_thoses_cnt; i++)
  {
    pick(gen->ns, gen->ds, gen->re_by_idx, &gen->pick_thoses[i], gen->priorities);
  }
  collect(gen->pick_thoses, gen->pick_thoses_cnt, &gen->bufs);
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

void parsergen_dump_headers(struct ParserGen *gen, FILE *f)
{
  fprintf(f, "#include \"yalecommon.h\"\n");
  dump_headers(f, gen->parsername, gen->max_bt);
  fprintf(f, "struct %s_parserctx {\n", gen->parsername);
  fprintf(f, "  uint8_t stacksz;\n");
  fprintf(f, "  struct ruleentry stack[%d];\n", gen->max_stack_size);
  fprintf(f, "  struct %s_rectx rctx;\n", gen->parsername);
  fprintf(f, "  uint8_t saved_token;\n");
  if (gen->state_include_str)
  {
    fprintf(f, "  %s\n", gen->state_include_str);
  }
  fprintf(f, "};\n");
  fprintf(f, "\n");
  fprintf(f, "static inline void %s_parserctx_init(struct %s_parserctx *pctx)\n",
          gen->parsername, gen->parsername);
  fprintf(f, "{\n");
  fprintf(f, "  pctx->saved_token = 255;\n");
  fprintf(f, "  pctx->stacksz = 1;\n");
  fprintf(f, "  pctx->stack[0].rhs = %d;\n", gen->start_state);
  fprintf(f, "  pctx->stack[0].cb = 255;\n");
  fprintf(f, "  %s_init_statemachine(&pctx->rctx);\n", gen->parsername);
  fprintf(f, "}\n");
  fprintf(f, "\n");
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
  return gen->tokencnt + (gen->nonterminalcnt++);
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
