#include <stdio.h>
#include "backtracktestcparser.h"

int main(int argc, char **argv)
{
  char input[] = "abcdegi";
  struct backtracktest_parserctx pctx = {};
  ssize_t consumed;
  size_t i;

  backtracktest_parserctx_init(&pctx);
  for (i = 0; i < sizeof(input)-1; i++)
  {
    consumed = backtracktest_parse_block(&pctx, &input[i], 1);
    if (consumed != sizeof(input)-1 && consumed != -EAGAIN)
    {
      printf("Fail i = %zu\n", i),
      abort();
    }
  }
  return 0;
}
