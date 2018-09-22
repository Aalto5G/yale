#ifndef _YALE_H_
#define _YALE_H_

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct CSnippet {
  char *data;
  size_t len;
  size_t capacity;
};

static inline void csadd(struct CSnippet *cs, char ch)
{
  if (cs->len + 2 >= cs->capacity)
  {
    size_t new_capacity = cs->capacity * 2 + 2;
    cs->data = realloc(cs->data, new_capacity);
    cs->capacity = new_capacity;
  }
  cs->data[cs->len] = ch;
  cs->data[cs->len + 1] = '\0';
  cs->len++;
}

static inline void csaddstr(struct CSnippet *cs, char *str)
{
  size_t len = strlen(str);
  if (cs->len + len + 1 >= cs->capacity)
  {
    size_t new_capacity = cs->capacity * 2 + 2;
    if (new_capacity < cs->len + len + 1)
    {
      new_capacity = cs->len + len + 1;
    }
    cs->data = realloc(cs->data, new_capacity);
    cs->capacity = new_capacity;
  }
  memcpy(cs->data + cs->len, str, len);
  cs->len += len;
  cs->data[cs->len] = '\0';
}

struct token {
  int priority;
  uint8_t nsitem;
  char *re;
};

struct ruleitem {
  uint8_t is_action:1;
  uint8_t value;
  uint8_t cb;
};

struct namespaceitem {
  char *name;
  uint8_t is_token:1;
  uint8_t is_lhs:1;
};

struct cb {
  char *name;
};

struct rule {
  uint8_t lhs;
  struct ruleitem rhs[255];
  uint8_t itemcnt;
};

struct yale {
  struct CSnippet cs;
  struct token tokens[255];
  struct namespaceitem ns[255];
  struct cb cbs[255];
  struct rule rules[255];
  uint8_t tokencnt;
  uint8_t nscnt;
  uint8_t cbcnt;
  uint8_t rulecnt;
};

static inline void yale_free(struct yale *yale)
{
  uint8_t i;
  for (i = 0; i < yale->cbcnt; i++)
  {
    free(yale->cbs[i].name);
    yale->cbs[i].name = NULL;
  }
  for (i = 0; i < yale->nscnt; i++)
  {
    free(yale->ns[i].name);
    yale->ns[i].name = NULL;
    yale->ns[i].is_token = 0;
    yale->ns[i].is_lhs = 0;
  }
  for (i = 0; i < yale->tokencnt; i++)
  {
    free(yale->tokens[i].re);
    yale->tokens[i].re = NULL;
    yale->tokens[i].nsitem = 0;
    yale->tokens[i].priority = 0;
  }
  yale->tokencnt = 0;
  yale->cbcnt = 0;
  yale->rulecnt = 0;
  yale->nscnt = 0;
  free(yale->cs.data);
  yale->cs.data = NULL;
  memset(yale, 0, sizeof(*yale));
}

static inline int check_actions(struct yale *yale)
{
  uint8_t i, j;
  for (i = 0; i < yale->rulecnt; i++)
  {
    for (j = 0; j < yale->rules[i].itemcnt; j++)
    {
      uint8_t value = yale->rules[i].rhs[j].value;
      uint8_t cb = yale->rules[i].rhs[j].cb;
      uint8_t act = yale->rules[i].rhs[j].is_action;
      if (!act && !yale->ns[value].is_token && cb != 255)
      {
        return -EINVAL;
      }
    }
  }
  return 0;
}

static inline void dump_python(struct yale *yale)
{
  uint8_t i;
  printf("import parser\n");
  printf("import sys\n\n");
  printf("d = {}\n");
  printf("p = parser.ParserGen(\"http\")\n\n"); // FIXME parser name
  for (i = 0; i < yale->tokencnt; i++)
  {
    struct token *tk = &yale->tokens[i];
    char *tkname = yale->ns[tk->nsitem].name;
    printf("d[\"%s\"] = p.add_token(\"\")\n", tkname); // FIXME re
  }
  printf("p.finalize_tokens()\n\n");
  for (i = 0; i < yale->nscnt; i++)
  {
    struct namespaceitem *nsit = &yale->ns[i];
    if (nsit->is_token)
    {
      if (nsit->is_lhs)
      {
        fprintf(stderr, "Error\n");
        exit(1);
      }
      continue;
    }
    if (!nsit->is_lhs)
    {
      fprintf(stderr, "Error\n");
      exit(1);
    }
    printf("d[\"%s\"] = p.add_nonterminal()\n", nsit->name);
  }
  printf("\np.start_state(requestWithHeaders)\n"); // FIXME start state
  printf("p.set_rules([\n");
  for (i = 0; i < yale->rulecnt; i++)
  {
    struct rule *rl = &yale->rules[i];
    uint8_t j;
    printf("  ");
    printf("(");
    printf("d[\"%s\"], ", yale->ns[rl->lhs].name);
    printf("[");
    for (j = 0; j < rl->itemcnt; j++)
    {
      struct ruleitem *it = &rl->rhs[j];
      if (it->is_action)
      {
        printf("p.action(\"%s\"), ", yale->cbs[it->cb].name);
      }
      else if (it->cb != 255)
      {
        printf("p.wrapCB(d[\"%s\"], \"%s\"), ", yale->ns[it->value].name, yale->cbs[it->cb].name);
      }
      else
      {
        printf("d[\"%s\"], ", yale->ns[it->value].name);
      }
    }
    printf("]");
    printf("),\n");
  }
  printf("])\n");
  printf("p.gen_parser()\n");
}

#endif
