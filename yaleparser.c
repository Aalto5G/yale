#include "yale.h"
#include "yyutils.h"
#include <stdio.h>
#include "parser.h"

struct ParserGen gen;

int main(int argc, char **argv)
{
  FILE *f;
  struct yale yale = {};
  size_t i, iter;

  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s file.txt\n", argv[0]);
    exit(1);
  }

  f = fopen(argv[1], "r");
  if (f == NULL)
  {
    fprintf(stderr, "Can't open input file\n");
    exit(1);
  }
  yaleyydoparse(f, &yale);
  fclose(f);
  if (check_actions(&yale) != 0)
  {
    printf("Fail action\n");
    exit(1);
  }
  for (iter = 0; iter < 1000*30; iter++)
  {
    parsergen_init(&gen, yale.parsername);
    for (i = 0; i < yale.tokencnt; i++)
    {
      yale.ns[yale.tokens[i].nsitem].val =
        parsergen_add_token(&gen, yale.tokens[i].re, strlen(yale.tokens[i].re), yale.tokens[i].priority); // FIXME '\0'
    }
    parsergen_finalize_tokens(&gen);
    for (i = 0; i < yale.nscnt; i++)
    {
      struct namespaceitem *nsit = &yale.ns[i];
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
      nsit->val = parsergen_add_nonterminal(&gen);
    }
    parsergen_state_include(&gen, yale.si.data);
    if (!yale.startns_present)
    {
      abort();
    }
    parsergen_set_start_state(&gen, yale.ns[yale.startns].val);
    parsergen_set_cb(&gen, yale.cbs, yale.cbcnt);
    parsergen_set_rules(&gen, yale.rules, yale.rulecnt, yale.ns);
    gen_parser(&gen);
  }
  yale_free(&yale);
  return 0;
}
