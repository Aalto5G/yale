#define _GNU_SOURCE
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <ctype.h>
#include "regex.h"

struct nfa_node ns[YALE_UINT_MAX_LEGAL];
struct dfa_node ds[YALE_UINT_MAX_LEGAL];
struct transitionbufs bufs;

static void *malloc_fn(void *ud, size_t sz)
{
  return malloc(sz);
}

int main(int argc, char **argv)
{
  size_t i;
#if 0
  size_t j, k;
  yale_uint_t dscnt;
  yale_uint_t ncnt;
  struct re *re;
  const char *res[3] = {"ab","abcd","abce"};
  yale_uint_t pick_those[3] = {0,1,2};
#endif
  const char re0[] = "[Hh][Oo][Ss][Tt]";
  const char re1[] = "\r?\n";
  const char re2[] = " ";
  const char re3[] = " HTTP/[0-9]+[.][0-9]+\r?\n";
  const char re4[] = ":[ \t]*";
  const char re5[] = "[-!#$%&'*+.^_`|~0-9A-Za-z]+";
  const char re6[] = "[\t\x20-\x7E\x80-\xFF]*";
  const char re7[] = "[]:/?#@!$&'()*+,;=0-9A-Za-z._~%[-]+";
  const char re8[] = "\r?\n[\t ]+";
#if 0
  const char *http_res[] = {
    "[Hh][Oo][Ss][Tt]" ,
    "\r?\n" ,
    " " ,
    " HTTP/[0-9]+[.][0-9]+\r?\n" ,
    ":[ \t]*" ,
    "[-!#$%&'*+.^_`|~0-9A-Za-z]+" ,
    "[\t\x20-\x7E\x80-\xFF]*" ,
    "[]:/?#@!$&'()*+,;=0-9A-Za-z._~%[-]+" ,
    "\r?\n[\t ]+" ,
  };
#endif
  struct iovec http_res[] = {
    {.iov_base = (void*)re0, .iov_len = sizeof(re0)-1, },
    {.iov_base = (void*)re1, .iov_len = sizeof(re1)-1, },
    {.iov_base = (void*)re2, .iov_len = sizeof(re2)-1, },
    {.iov_base = (void*)re3, .iov_len = sizeof(re3)-1, },
    {.iov_base = (void*)re4, .iov_len = sizeof(re4)-1, },
    {.iov_base = (void*)re5, .iov_len = sizeof(re5)-1, },
    {.iov_base = (void*)re6, .iov_len = sizeof(re6)-1, },
    {.iov_base = (void*)re7, .iov_len = sizeof(re7)-1, },
    {.iov_base = (void*)re8, .iov_len = sizeof(re8)-1, },
  };
  int priorities[] = {
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
  };
  int caseis[] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
  };
  yale_uint_t pick_those0[] = {0};
  yale_uint_t pick_those1[] = {0,1,5};
  yale_uint_t pick_those2[] = {0,5};
  yale_uint_t pick_those3[] = {1};
  yale_uint_t pick_those4[] = {1,8};
  yale_uint_t pick_those5[] = {2};
  yale_uint_t pick_those6[] = {3};
  yale_uint_t pick_those7[] = {4};
  yale_uint_t pick_those8[] = {5};
  yale_uint_t pick_those9[] = {6};
  yale_uint_t pick_those10[] = {7};
  yale_uint_t pick_those11[] = {8};
  struct pick_those_struct pick_thoses[] = {
    {.pick_those=pick_those0, .len=sizeof(pick_those0)/sizeof(*pick_those0)},
    {.pick_those=pick_those1, .len=sizeof(pick_those1)/sizeof(*pick_those1)},
    {.pick_those=pick_those2, .len=sizeof(pick_those2)/sizeof(*pick_those2)},
    {.pick_those=pick_those3, .len=sizeof(pick_those3)/sizeof(*pick_those3)},
    {.pick_those=pick_those4, .len=sizeof(pick_those4)/sizeof(*pick_those4)},
    {.pick_those=pick_those5, .len=sizeof(pick_those5)/sizeof(*pick_those5)},
    {.pick_those=pick_those6, .len=sizeof(pick_those6)/sizeof(*pick_those6)},
    {.pick_those=pick_those7, .len=sizeof(pick_those7)/sizeof(*pick_those7)},
    {.pick_those=pick_those8, .len=sizeof(pick_those8)/sizeof(*pick_those8)},
    {.pick_those=pick_those9, .len=sizeof(pick_those9)/sizeof(*pick_those9)},
    {.pick_those=pick_those10, .len=sizeof(pick_those10)/sizeof(*pick_those10)},
    {.pick_those=pick_those11, .len=sizeof(pick_those11)/sizeof(*pick_those11)},
  };
  ssize_t maxbt = 0;
  FILE *f;

  //yale_uint_t transitions[256] = {};
  //perf_trans(transitions, &bufs);

#if 0
  for (i = 0; i < YALE_UINT_MAX_LEGAL; i++)
  {
    dfa_init_empty(&ds[i]);
  }

  nfa_init(&ns[0], 0, YALE_UINT_MAX_LEGAL);
  nfa_init(&ns[1], 0, 0);
  nfa_init(&ns[2], 0, 1);
  nfa_init(&ns[3], 0, 2);
  nfa_init(&ns[4], 0, 0);
  nfa_init(&ns[5], 1, 1);
  nfa_init(&ns[6], 0, 2);
  nfa_init(&ns[7], 0, 0);
  nfa_init(&ns[8], 0, 2);
  nfa_init(&ns[9], 1, 0);
  nfa_init(&ns[10], 1, 2);

  nfa_connect(&ns[0], 'a', 1);
  nfa_connect(&ns[1], 'b', 4);
  nfa_connect(&ns[4], 'c', 7);
  nfa_connect(&ns[7], 'e', 9);

  nfa_connect(&ns[0], 'a', 2);
  nfa_connect(&ns[2], 'b', 5);

  nfa_connect(&ns[0], 'a', 3);
  nfa_connect(&ns[3], 'b', 6);
  nfa_connect(&ns[6], 'c', 8);
  nfa_connect(&ns[8], 'd', 10);

  dscnt = nfa2dfa(ns, ds, 0);
  printf("DFA state count %d\n", (int)dscnt);
  printf("\n\n\n\n");
  dfaviz(ds, dscnt);
  printf("\n\n\n\n");

  for (i = 0; i < YALE_UINT_MAX_LEGAL; i++)
  {
    dfa_init_empty(&ds[i]);
  }

  ncnt = 0;
  const char *relit = "ab|abcd|abce";
  size_t remainderstart;
  re = parse_re(relit, strlen(relit), &remainderstart);
  gennfa_main(re, ns, &ncnt, 0);
  free_re(re);
  printf("NFA state count %d\n", (int)ncnt);
  printf("\n\n\n\n");
  nfaviz(ns, ncnt);
  printf("\n\n\n\n");
  dscnt = nfa2dfa(ns, ds, 0);
  printf("DFA state count %d\n", (int)dscnt);
  printf("\n\n\n\n");
  dfaviz(ds, dscnt);
  printf("\n\n\n\n");

  for (i = 0; i < YALE_UINT_MAX_LEGAL; i++)
  {
    dfa_init_empty(&ds[i]);
  }

  ncnt = 0;
  re = parse_res(res, pick_those, 3);
  gennfa_alternmulti(re, ns, &ncnt);
  free_re(re);
  printf("NFA state count %d\n", (int)ncnt);
  printf("\n\n\n\n");
  nfaviz(ns, ncnt);
  printf("\n\n\n\n");
  dscnt = nfa2dfa(ns, ds, 0);
  printf("DFA state count %d\n", (int)dscnt);
  printf("\n\n\n\n");
  dfaviz(ds, dscnt);
  printf("\n\n\n\n");
#endif

#if 0
  for (k = 0; k < 100; k++)
  {
    for (j = 0; j < sizeof(pick_thoses)/sizeof(*pick_thoses); j++)
    //for (j = 0; j < 1; j++)
    {
      for (i = 0; i < YALE_UINT_MAX_LEGAL; i++)
      {
        dfa_init_empty(&ds[i]);
      }
  
      printf("Pick those %d\n", (int)j);

      ncnt = 0;
      re = parse_res(http_res, pick_thoses[j].pick_those, pick_thoses[j].len);
      gennfa_alternmulti(re, ns, &ncnt);
      free_re(re);
      printf("NFA state count %d\n", (int)ncnt);
      //printf("\n\n\n\n");
      //nfaviz(ns, ncnt);
      //printf("\n\n\n\n");
      dscnt = nfa2dfa(ns, ds, 0);
      printf("DFA state count %d\n", (int)dscnt);
      set_accepting(ds, 0, priorities);
      //printf("\n\n\n\n");
      //dfaviz(ds, dscnt);
      //printf("\n\n\n\n");
    }
  }
#endif

  size_t outiter;
  for (outiter = 0; outiter < 1000; outiter++)
  {
    for (i = 0; i < sizeof(pick_thoses)/sizeof(*pick_thoses); i++)
    {
      pick(ns, ds, http_res, &pick_thoses[i], priorities, caseis);
    }
    collect(pick_thoses, sizeof(pick_thoses)/sizeof(*pick_thoses), &bufs, malloc_fn, NULL);
    for (i = 0; i < sizeof(pick_thoses)/sizeof(*pick_thoses); i++)
    {
      ssize_t curbt;
      curbt = maximal_backtrack(pick_thoses[i].ds, 0, 250);
      if (curbt < 0)
      {
        abort(); // FIXME error handling
      }
      if (maxbt < curbt)
      {
        maxbt = curbt;
      }
    }
  
    printf("Max backtrack %zd\n", maxbt);
    f = fopen("httpparser2.h", "w");
    fprintf(f, "#ifndef _HTTPPARSER_H_\n");
    fprintf(f, "#define _HTTPPARSER_H_\n");
    fprintf(f, "#include \"yalecommon.h\"\n");
    dump_headers(f, "http", maxbt, 0, "uint8_t", 1);
    fprintf(f, "#endif\n");
    fclose(f);
    f = fopen("httpparser2.c", "w");
    fprintf(f, "#include \"httpparser2.h\"\n");
    dump_chead(f, "http", 0, 0);
    dump_collected(f, "http", &bufs);
    for (i = 0; i < sizeof(pick_thoses)/sizeof(*pick_thoses); i++)
    {
#if 0
      dump_one(f, "http", &pick_thoses[i]); // Doesn't work anymore, needs gen
#endif
    }
    fclose(f);
  }
}
