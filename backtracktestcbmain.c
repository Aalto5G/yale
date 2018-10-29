#include <stdio.h>
#include "backtracktestcbcparser.h"
#include "backtracktestcbcommon.h"

#define DO_PRINT

#ifdef DO_PRINT
static inline void myPutchar(char ch)
{
  if (ch >= 0x20 && ch <= 0x7E)
  {
    putchar(ch);
  }
  else
  {
    printf("\\x%.2x", (unsigned)(unsigned char)ch);
  }
}
#endif

ssize_t print(const char *buf, size_t siz, int start, struct backtracktestcb_parserctx *btn)
{
#ifdef DO_PRINT
  const char *ubuf = buf;
  size_t i;

  // We don't want empty starts
  if ((start & YALE_FLAG_START) && siz == 0)
  {
    abort();
  }

  if (start & YALE_FLAG_START)
  {
    putchar('<');
  }
  else
  {
    putchar('[');
  }
  for (i = 0; i < siz; i++)
  {
    myPutchar(ubuf[i]);
  }
  if (start & YALE_FLAG_START)
  {
    putchar('>');
  }
  else
  {
    putchar(']');
  }
  if (start & YALE_FLAG_MAJOR_MISTAKE)
  {
    putchar('!');
  }
  if (start & YALE_FLAG_END)
  {
    putchar('-');
  }
  putchar('\n');
#endif
  return -EAGAIN;
}

ssize_t cb1(const char *buf, size_t siz, int start, struct backtracktestcb_parserctx *btn)
{
  printf("1: ");
  return print(buf, siz, start, btn);
}
ssize_t cb2(const char *buf, size_t siz, int start, struct backtracktestcb_parserctx *btn)
{
  printf("2: ");
  return print(buf, siz, start, btn);
}

int main(int argc, char **argv)
{
  char input1[] = "abcdf" "abde";
  struct backtracktestcb_parserctx pctx = {};
  ssize_t consumed;
  size_t i;

  backtracktestcb_parserctx_init(&pctx);
  for (i = 0; i < sizeof(input1)-1; i++)
  {
    consumed = backtracktestcb_parse_block(&pctx, &input1[i], 1);
    if (consumed != 1 && consumed != -EAGAIN)
    {
      printf("Fail i = %zu\n", i);
      abort();
    }
    printf("Succeed i = %zu\n", i);
  }
  return 0;
}
