#include <stdio.h>
#include "recursivecbcparser.h"
#include "recursivecbcommon.h"

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

ssize_t print(const char *buf, size_t siz, int start, struct recursivecb_parserctx *btn)
{
#ifdef DO_PRINT
  const char *ubuf = buf;
  size_t i;
  putchar('<');
  for (i = 0; i < siz; i++)
  {
    myPutchar(ubuf[i]);
  }
  putchar('>');
  putchar('\n');
#endif
  return -EAGAIN;
}

ssize_t f1(const char *buf, size_t siz, int start, struct recursivecb_parserctx *btn)
{
  printf("1: ");
  return print(buf, siz, start, btn);
}
ssize_t f2(const char *buf, size_t siz, int start, struct recursivecb_parserctx *btn)
{
  printf("2: ");
  return print(buf, siz, start, btn);
}
ssize_t f3(const char *buf, size_t siz, int start, struct recursivecb_parserctx *btn)
{
  printf("3: ");
  return print(buf, siz, start, btn);
}
ssize_t f4(const char *buf, size_t siz, int start, struct recursivecb_parserctx *btn)
{
  printf("4: ");
  return print(buf, siz, start, btn);
}

int main(int argc, char **argv)
{
  char input[] = "deef";
  struct recursivecb_parserctx pctx = {};
  ssize_t consumed;

  recursivecb_parserctx_init(&pctx);
  consumed = recursivecb_parse_block(&pctx, input, sizeof(input)-1);
  if (consumed != sizeof(input)-1)
  {
    abort();
  }
  return 0;
}
