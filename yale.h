#ifndef _YALE_H_
#define _YALE_H_

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

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
  uint8_t val;
};

struct cb {
  char *name;
};

struct rule {
  uint8_t lhs;
  struct ruleitem rhs[255];
  uint8_t itemcnt;
  struct ruleitem rhsnoact[255];
  uint8_t noactcnt;
};

struct yale {
  struct CSnippet cs;
  struct CSnippet si;
  struct token tokens[255];
  struct namespaceitem ns[255];
  struct cb cbs[255];
  struct rule rules[255];
  uint8_t tokencnt;
  uint8_t nscnt;
  uint8_t cbcnt;
  uint8_t rulecnt;
  char *parsername;
  uint8_t startns;
  uint8_t startns_present;
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
  free(yale->si.data);
  yale->si.data = NULL;
  free(yale->parsername);
  yale->parsername = NULL;
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

static inline void check_python(struct yale *yale)
{
  uint8_t i;
  if (!yale->startns_present)
  {
    fprintf(stderr, "Error\n");
    exit(1);
  }
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
  }
}

static inline void dump_string(FILE *f, const char *str)
{
  size_t len = strlen(str);
  size_t i;
  putc('"', f);
  for (i = 0; i < len; i++)
  {
    if (str[i] == '"' || str[i] == '\\')
    {
      fprintf(f, "\\");
      putc(str[i], f);
      continue;
    }
    if (str[i] == '\n')
    {
      fprintf(f, "\\n");
      continue;
    }
    if (str[i] == '\t')
    {
      fprintf(f, "\\t");
      continue;
    }
    if (str[i] == '\r')
    {
      fprintf(f, "\\r");
      continue;
    }
    if (isalpha(str[i]) || isdigit(str[i]) || ispunct(str[i]) || str[i] == ' ')
    {
      putc(str[i], f);
      continue;
    }
    fprintf(f, "\\x");
    fprintf(f, "%.2x", (unsigned char)str[i]);
  }
  putc('"', f);
}

static inline void dump_python(FILE *f, struct yale *yale)
{
  uint8_t i;
  char *upparsername = strdup(yale->parsername);
  size_t len = strlen(upparsername);
  for (i = 0; i < len; i++)
  {
    upparsername[i] = toupper((unsigned char)upparsername[i]);
  }
  fprintf(f, "from __future__ import print_function\n");
  fprintf(f, "import parser\n");
  fprintf(f, "import sys\n\n");
  fprintf(f, "d = {}\n");
  fprintf(f, "p = parser.ParserGen(\"%s\")\n\n", yale->parsername);
  for (i = 0; i < yale->tokencnt; i++)
  {
    struct token *tk = &yale->tokens[i];
    char *tkname = yale->ns[tk->nsitem].name;
    fprintf(f, "d[\"%s\"] = p.add_token(", tkname);
    dump_string(f, tk->re);
    fprintf(f, ", priority=%d)\n", tk->priority);
  }
  fprintf(f, "p.finalize_tokens()\n\n");
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
    fprintf(f, "d[\"%s\"] = p.add_nonterminal()\n", nsit->name);
  }
  fprintf(f, "\np.start_state(d[\"%s\"])\n", yale->ns[yale->startns].name);
  fprintf(f, "\n");
  fprintf(f, "p.set_rules([\n");
  for (i = 0; i < yale->rulecnt; i++)
  {
    struct rule *rl = &yale->rules[i];
    uint8_t j;
    fprintf(f, "  ");
    fprintf(f, "(");
    fprintf(f, "d[\"%s\"], ", yale->ns[rl->lhs].name);
    fprintf(f, "[");
    for (j = 0; j < rl->itemcnt; j++)
    {
      struct ruleitem *it = &rl->rhs[j];
      if (it->is_action)
      {
        fprintf(f, "p.action(\"%s\"), ", yale->cbs[it->cb].name);
      }
      else if (it->cb != 255)
      {
        fprintf(f, "p.wrapCB(d[\"%s\"], \"%s\"), ", yale->ns[it->value].name, yale->cbs[it->cb].name);
      }
      else
      {
        fprintf(f, "d[\"%s\"], ", yale->ns[it->value].name);
      }
    }
    fprintf(f, "]");
    fprintf(f, "),\n");
  }
  fprintf(f, "])\n\n");
  if (yale->si.data)
  {
    fprintf(f, "p.state_include(");
    dump_string(f, yale->si.data);
    fprintf(f, ")\n");
  }
  fprintf(f, "p.gen_parser()\n");
  fprintf(f, "if sys.argv[1] == \"h\":\n");
  fprintf(f, "  with open('%sparser.h', 'w') as fd:\n", yale->parsername);
  fprintf(f, "    print(\"#ifndef _%sPARSER_H_\", file=fd)\n", upparsername);
  fprintf(f, "    print(\"#define _%sPARSER_H_\", file=fd)\n", upparsername);
  fprintf(f, "    p.print_headers(fd)\n");
  fprintf(f, "    print(\"#endif\", file=fd)\n");
  fprintf(f, "elif sys.argv[1] == \"c\":\n");
  fprintf(f, "  with open('%sparser.c', 'w') as fd:\n", yale->parsername);
  if (yale->cs.data)
  {
    fprintf(f, "    print(");
    dump_string(f, yale->cs.data);
    fprintf(f, ", file=fd)\n");
  }
  fprintf(f, "    print(\"#include \\\"%sparser.h\\\"\", file=fd)\n", yale->parsername);
  fprintf(f, "    p.print_parser(fd)\n");
  free(upparsername);
}

#endif
