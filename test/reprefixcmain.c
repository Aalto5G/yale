#include "reprefixcparser.h"
#include "reprefixcommon.h"
#include <sys/time.h>

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

ssize_t print(const char *buf, size_t siz, int start, struct reprefix_parserctx *pctx)
{
  const char *ubuf = buf;
  size_t i;
  if (start)
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
  if (start)
  {
    putchar('>');
  }
  else
  {
    putchar(']');
  }
  putchar('\n');
  return -EAGAIN;
}
ssize_t printall(const char *buf, size_t siz, int start, struct reprefix_parserctx *pctx)
{
  const char *ubuf = buf;
  size_t i;
  if (start)
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
  if (start)
  {
    putchar('>');
  }
  else
  {
    putchar(']');
  }
  putchar('A');
  putchar('\n');
  return -EAGAIN;
}
ssize_t printcont(const char *buf, size_t siz, int start, struct reprefix_parserctx *pctx)
{
  const char *ubuf = buf;
  size_t i;
  if (start)
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
  if (start)
  {
    putchar('>');
  }
  else
  {
    putchar(']');
  }
  putchar('C');
  putchar('\n');
  return -EAGAIN;
}

ssize_t szbe(const char *buf, size_t siz, int start, struct reprefix_parserctx *pctx)
{
  size_t i;
  if (start & YALE_FLAG_START)
  {
    pctx->bytes_sz = 0;
  }
  for (i = 0; i < siz; i++)
  {
    pctx->bytes_sz <<= 8;
    pctx->bytes_sz |= (unsigned char)buf[i];
  }
  return -EAGAIN;
}

int main(int argc, char **argv)
{
  ssize_t consumed;
  size_t i;
  struct reprefix_parserctx pctx2 = {};
  struct reprefix_parserctx pctx = {};
  char reprefix[] =
    "\x00\x01\x00\x02GH";

  for (i = 0; i < 1; i++)
  {
    pctx = pctx2;
    reprefix_parserctx_init(&pctx);
    consumed = reprefix_parse_block(&pctx, reprefix, sizeof(reprefix)-1, 1);
    if (consumed != -EAGAIN && consumed != sizeof(reprefix)-1)
    {
      printf("Consumed %zd expected -EAGAIN\n", consumed);
      abort();
    }
  }

  printf("----------------\n");

  reprefix_parserctx_init(&pctx);
  for (i = 0; i < sizeof(reprefix)-1; i++)
  {
    consumed = reprefix_parse_block(&pctx, reprefix+i, 1, i == sizeof(reprefix) - 2);
    if (consumed != -EAGAIN && !(consumed == 1 && i == sizeof(reprefix) - 2))
    {
      printf("Consumed %zd expected -EAGAIN\n", consumed);
      abort();
    }
  }

  return 0;
}
