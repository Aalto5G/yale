#ifndef _YALE_H_
#define _YALE_H_

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "yaleuint.h"

struct escaped_string {
  size_t sz;
  char *str;
};

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
  uint8_t i:1;
  yale_uint_t nsitem;
  struct escaped_string re;
};

struct ruleitem {
  uint8_t is_action:1;
  uint8_t is_bytes:1;
  yale_uint_t value;
  yale_uint_t cb;
};

struct namespaceitem {
  char *name;
  uint8_t is_token:1;
  uint8_t is_lhs:1;
  yale_uint_t val;
};

struct cb {
  char *name;
};

struct rule {
  yale_uint_t lhs;
  yale_uint_t cond;
  struct ruleitem rhs[YALE_UINT_MAX_LEGAL];
  yale_uint_t itemcnt;
  struct ruleitem rhsnoact[YALE_UINT_MAX_LEGAL];
  yale_uint_t noactcnt;
};

struct yale {
  struct CSnippet cs;
  struct CSnippet hs;
  struct CSnippet si;
  struct CSnippet ii;
  struct token tokens[YALE_UINT_MAX_LEGAL];
  struct namespaceitem ns[YALE_UINT_MAX_LEGAL];
  struct cb cbs[YALE_UINT_MAX_LEGAL];
  struct rule rules[YALE_UINT_MAX_LEGAL];
  yale_uint_t tokencnt;
  yale_uint_t nscnt;
  yale_uint_t cbcnt;
  yale_uint_t rulecnt;
  char *parsername;
  char *bytessizetype;
  char *conds[YALE_UINT_MAX_LEGAL];
  yale_uint_t condcnt;
  yale_uint_t startns;
  uint8_t startns_present;
  uint8_t nofastpath;
  uint8_t shortcutting;
};

static inline void yale_free(struct yale *yale)
{
  yale_uint_t i;
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
    free(yale->tokens[i].re.str);
    yale->tokens[i].re.str = NULL;
    yale->tokens[i].nsitem = 0;
    yale->tokens[i].priority = 0;
  }
  yale->tokencnt = 0;
  yale->cbcnt = 0;
  yale->rulecnt = 0;
  yale->nscnt = 0;
  free(yale->cs.data);
  yale->cs.data = NULL;
  free(yale->hs.data);
  yale->hs.data = NULL;
  free(yale->si.data);
  yale->si.data = NULL;
  free(yale->ii.data);
  yale->ii.data = NULL;
  free(yale->parsername);
  yale->parsername = NULL;
  free(yale->bytessizetype);
  yale->bytessizetype = NULL;
  memset(yale, 0, sizeof(*yale));
}

static inline int check_actions(struct yale *yale)
{
  yale_uint_t i, j;
  for (i = 0; i < yale->rulecnt; i++)
  {
    for (j = 0; j < yale->rules[i].itemcnt; j++)
    {
      yale_uint_t value = yale->rules[i].rhs[j].value;
      yale_uint_t cb = yale->rules[i].rhs[j].cb;
      yale_uint_t act = yale->rules[i].rhs[j].is_action;
      yale_uint_t byt = yale->rules[i].rhs[j].is_bytes;
      if (!act && !byt && !yale->ns[value].is_token && cb != YALE_UINT_MAX_LEGAL)
      {
        return -EINVAL;
      }
    }
  }
  return 0;
}

static inline void check_python(struct yale *yale)
{
  yale_uint_t i;
  if (!yale->startns_present)
  {
    fprintf(stderr, "Error 1\n");
    exit(1);
  }
  for (i = 0; i < yale->nscnt; i++)
  {
    struct namespaceitem *nsit = &yale->ns[i];
    if (nsit->is_token)
    {
      if (nsit->is_lhs)
      {
        fprintf(stderr, "Error 2\n");
        exit(1);
      }
      continue;
    }
    if (!nsit->is_lhs)
    {
      fprintf(stderr, "Error 3\n");
      exit(1);
    }
  }
}

static inline void dump_string_len(FILE *f, const char *str, size_t len)
{
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

static inline void dump_string(FILE *f, const char *str)
{
  size_t len = strlen(str);
  dump_string_len(f, str, len);
}

static inline void dump_python(FILE *f, struct yale *yale)
{
  yale_uint_t i;
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
    dump_string_len(f, tk->re.str, tk->re.sz);
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
        fprintf(stderr, "Error 4\n");
        exit(1);
      }
      continue;
    }
    if (!nsit->is_lhs)
    {
      fprintf(stderr, "Error 5\n");
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
    yale_uint_t j;
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
      else if (it->cb != YALE_UINT_MAX_LEGAL)
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
