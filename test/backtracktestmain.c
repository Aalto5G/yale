#include <stdio.h>
#include "backtracktestcparser.h"

int main(int argc, char **argv)
{
  char input1[] = "abcdegi";
  char input2[] = "abcdefbcdegi";
  char input3[] = "abcdeghcdegi";
  char input4[] = "abcdefbcdeghcdegi";
  struct backtracktest_parserctx pctx = {};
  ssize_t consumed;
  size_t i;

  backtracktest_parserctx_init(&pctx);
  for (i = 0; i < sizeof(input1)-1; i++)
  {
    consumed = backtracktest_parse_block(&pctx, &input1[i], 1, i == sizeof(input1) - 2);
    if (consumed != 1 && consumed != -EAGAIN)
    {
      printf("Fail i = %zu\n", i);
      abort();
    }
    printf("Succeed i = %zu\n", i);
  }
  backtracktest_parserctx_init(&pctx);
  for (i = 0; i < sizeof(input2)-1; i++)
  {
    consumed = backtracktest_parse_block(&pctx, &input2[i], 1, i == sizeof(input2) - 2);
    if (consumed != 1 && consumed != -EAGAIN)
    {
      printf("Fail i = %zu\n", i);
      abort();
    }
    printf("Succeed i = %zu\n", i);
  }
  backtracktest_parserctx_init(&pctx);
  for (i = 0; i < sizeof(input3)-1; i++)
  {
    consumed = backtracktest_parse_block(&pctx, &input3[i], 1, i == sizeof(input3) - 2);
    if (consumed != 1 && consumed != -EAGAIN)
    {
      printf("Fail i = %zu\n", i);
      abort();
    }
    printf("Succeed i = %zu\n", i);
  }
  backtracktest_parserctx_init(&pctx);
  for (i = 0; i < sizeof(input4)-1; i++)
  {
    consumed = backtracktest_parse_block(&pctx, &input4[i], 1, i == sizeof(input4) - 2);
    if (consumed != 1 && consumed != -EAGAIN)
    {
      printf("Fail i = %zu\n", i);
      abort();
    }
    printf("Succeed i = %zu\n", i);
  }
  return 0;
}
