#define _GNU_SOURCE
#include "yale.h"
#include "yyutils.h"
#include <stdio.h>
#include <libgen.h>
#include <unistd.h>
#include "parser.h"
#include "../git.h"

struct ParserGen gen;
struct yale yale = YALE_EMPTY;

static void usage(const char *argv0)
{
  fprintf(stderr, "This is YaLe version %s\n", gitversion);
  fprintf(stderr, "Usage: %s [-o outdir] file.txt [c|h|b|p|t]\n", argv0);
  exit(1);
}

int main(int argc, char **argv)
{
  FILE *f;
  size_t i, iter;
  char cnamebuf[1024] = {0};
  char hnamebuf[1024] = {0};
  char hincbuf[1024] = {0};
  char hdefbuf[1024] = {0};
  size_t len;
  int c = 0;
  int h = 0;
  size_t iters = 1;
  int test = 0;
  char *dir = NULL;
  int opt;

  while ((opt = getopt(argc, argv, "o:")) != -1)
  {
    switch (opt)
    {
      case 'o':
        dir = optarg;
        break;
      default:
        usage(argv[0]);
        break;
    }
  }

  if (argc != optind + 2)
  {
    usage(argv[0]);
  }
  if (strcmp(argv[optind+1], "c") == 0)
  {
    c = 1;
  }
  else if (strcmp(argv[optind+1], "h") == 0)
  {
    h = 1;
  }
  else if (strcmp(argv[optind+1], "b") == 0)
  {
    c = 1;
    h = 1;
  }
  else if (strcmp(argv[optind+1], "t") == 0)
  {
    test = 1;
    c = 1;
    h = 1;
  }
  else if (strcmp(argv[optind+1], "p") == 0)
  {
    iters = 1000;
    c = 1;
    h = 1;
  }
  else
  {
    usage(argv[0]);
  }

  f = fopen(argv[optind+0], "r");
  if (f == NULL)
  {
    fprintf(stderr, "Can't open input file %s\n", argv[optind+0]);
    exit(1);
  }
  yaleyydoparse(f, &yale);
  fclose(f);
#if 0
  if (check_actions(&yale) != 0)
  {
    printf("Fail action\n");
    exit(1);
  }
#endif

  if (dir == NULL)
  {
    dir = dirname(strdup(argv[optind+0]));
  }

  snprintf(cnamebuf, sizeof(cnamebuf), "%s/%s%s", dir, yale.parsername, "cparser.c");
  snprintf(hnamebuf, sizeof(hnamebuf), "%s/%s%s", dir, yale.parsername, "cparser.h");
  snprintf(hincbuf, sizeof(hincbuf), "%s%s", yale.parsername, "cparser.h");
  if (iters > 1 || test)
  {
    snprintf(cnamebuf, sizeof(cnamebuf), "/dev/null");
    snprintf(hnamebuf, sizeof(hnamebuf), "/dev/null");
  }
  snprintf(hdefbuf, sizeof(hdefbuf), "_%sCPARSER_H_", yale.parsername);
  len = strlen(hdefbuf);
  for (i = 0; i < len; i++)
  {
    hdefbuf[i] = toupper((unsigned char)hdefbuf[i]);
  }

  for (iter = 0; iter < iters; iter++)
  {
    parsergen_init(&gen, yale.parsername);
    if (yale.bytessizetype)
    {
      parsergen_set_bytessizetype(&gen, yale.bytessizetype);
    }
    for (i = 0; i < yale.tokencnt; i++)
    {
      yale.ns[yale.tokens[i].nsitem].val =
        parsergen_add_token(&gen, yale.tokens[i].re.str, yale.tokens[i].re.sz, yale.tokens[i].priority,
                            yale.tokens[i].i, yale.ns[yale.tokens[i].nsitem].name); // FIXME '\0'
    }
    parsergen_finalize_tokens(&gen);
    for (i = 0; i < yale.nscnt; i++)
    {
      struct namespaceitem *nsit = &yale.ns[i];
      if (nsit->is_token)
      {
        if (nsit->is_lhs)
        {
          fprintf(stderr, "Error: token %s both terminal and nonterminal\n", nsit->name);
          exit(1);
        }
        continue;
      }
      if (!nsit->is_lhs)
      {
        fprintf(stderr, "Error: token %s neither terminal nor nonterminal\n", nsit->name);
        exit(1);
      }
      nsit->val = parsergen_add_nonterminal(&gen, yale.ns[i].name);
      yale.nonterminals[nsit->val].nsitem = i;
    }
    if (yale.si.data != NULL)
    {
      parsergen_state_include(&gen, yale.si.data);
    }
    if (yale.ii.data != NULL)
    {
      parsergen_init_include(&gen, yale.ii.data);
    }
    if (yale.ei.data != NULL)
    {
      parsergen_empty_include(&gen, yale.ei.data);
    }
    if (!yale.startns_present)
    {
      abort();
    }
    parsergen_set_start_state(&gen, yale.ns[yale.startns].val);
    parsergen_set_cb(&gen, yale.cbs, yale.cbcnt);
    parsergen_set_conds(&gen, yale.conds, yale.condcnt);
    parsergen_set_rules(&gen, yale.rules, yale.rulecnt, yale.ns);
    if (yale.nofastpath)
    {
      parsergen_nofastpath(&gen);
    }
    if (yale.shortcutting)
    {
      parsergen_shortcutting(&gen);
    }
    gen_parser(&gen, &yale);
    if (h)
    {
      f = fopen(hnamebuf, "w");
      fprintf(f, "#ifndef %s\n", hdefbuf);
      fprintf(f, "#define %s\n", hdefbuf);
      if (yale.hs.data)
      {
        fprintf(f, "%s", yale.hs.data);
      }
      parsergen_dump_headers(&gen, f);
      fprintf(f, "#endif\n");
      fclose(f);
    }
    if (c)
    {
      f = fopen(cnamebuf, "w");
      fprintf(f, "#define _GNU_SOURCE\n");
      if (yale.cs.data)
      {
        fprintf(f, "%s", yale.cs.data);
      }
      fprintf(f, "#include \"%s\"\n", hincbuf);
      parsergen_dump_parser(&gen, &yale, f);
      fclose(f);
    }
    parsergen_free(&gen);
    memset(&gen, 0, sizeof(gen));
  }
  yale_free(&yale);
  return 0;
}
