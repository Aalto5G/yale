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

static inline void dump_string(const char *str)
{
  size_t len = strlen(str);
  size_t i;
  putchar('"');
  for (i = 0; i < len; i++)
  {
    if (str[i] == '"' || str[i] == '\\')
    {
      printf("\\");
      putchar(str[i]);
      continue;
    }
    if (str[i] == '\n')
    {
      printf("\\n");
      continue;
    }
    if (str[i] == '\t')
    {
      printf("\\t");
      continue;
    }
    if (str[i] == '\r')
    {
      printf("\\r");
      continue;
    }
    if (isalpha(str[i]) || isdigit(str[i]) || ispunct(str[i]) || str[i] == ' ')
    {
      putchar(str[i]);
      continue;
    }
    printf("\\x");
    printf("%.2x", (unsigned char)str[i]);
  }
  putchar('"');
}

static inline void dump_python(struct yale *yale)
{
  uint8_t i;
  char *upparsername = strdup(yale->parsername);
  size_t len = strlen(upparsername);
  for (i = 0; i < len; i++)
  {
    upparsername[i] = toupper((unsigned char)upparsername[i]);
  }
  printf("import parser\n");
  printf("import sys\n\n");
  printf("d = {}\n");
  printf("p = parser.ParserGen(\"%s\")\n\n", yale->parsername);
  for (i = 0; i < yale->tokencnt; i++)
  {
    struct token *tk = &yale->tokens[i];
    char *tkname = yale->ns[tk->nsitem].name;
    printf("d[\"%s\"] = p.add_token(", tkname);
    dump_string(tk->re);
    printf(", priority=%d)\n", tk->priority);
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
  printf("\np.start_state(d[\"%s\"])\n", yale->ns[yale->startns].name);
  printf("\n");
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
  printf("])\n\n");
  printf("p.gen_parser()\n");
  printf("if sys.argv[1] == \"h\":\n");
  printf("  with open('%sparser.h', 'w') as fd:\n", yale->parsername);
  printf("    print(\"#ifndef _%sPARSER_H_\", file=fd)\n", upparsername);
  printf("    print(\"#define _%sPARSER_H_\", file=fd)\n", upparsername);
  printf("    p.print_headers(fd)\n");
  printf("    print(\"#endif\", file=fd)\n");
  printf("elif sys.argv[1] == \"c\":\n");
  printf("  with open('%sparser.c', 'w') as fd:\n", yale->parsername);
  printf("    print(\"#include \\\"httpcommon.h\\\"\", file=fd)\n"); // FIXME
  printf("    print(\"#include \\\"%sparser.h\\\"\", file=fd)\n", yale->parsername);
  printf("    p.print_parser(fd)\n");
  free(upparsername);
}

#endif
