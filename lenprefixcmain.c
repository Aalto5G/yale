#include "lenprefixcparser.h"
#include "lenprefixcommon.h"
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

ssize_t print(const char *buf, size_t siz, int start, struct lenprefix_parserctx *pctx)
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
ssize_t printall(const char *buf, size_t siz, int start, struct lenprefix_parserctx *pctx)
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
ssize_t printcont(const char *buf, size_t siz, int start, struct lenprefix_parserctx *pctx)
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

ssize_t szbe(const char *buf, size_t siz, int start, struct lenprefix_parserctx *pctx)
{
  size_t i;
  if (start)
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
  struct lenprefix_parserctx pctx = {};
  char lenprefix[] =
    "\x00\x00\x00\x00"
    "\x00\x00\x00\x01G"
    "\x00\x00\x00\x02GH"
    "\x00\x00\x00\x03GHI"
    "\x00\x00\x00\x04GHIJ"
    "\x00\x00\x00\x05GHIJK"
    "\x00\x01\x00\x00"
    "\x00\x01\x00\x01g"
    "\x00\x01\x00\x02gh"
    "\x00\x01\x00\x03ghi"
    "\x00\x01\x00\x04ghij"
    "\x00\x01\x00\x05ghijk";

  for (i = 0; i < 1; i++)
  {
    lenprefix_parserctx_init(&pctx);
    consumed = lenprefix_parse_block(&pctx, lenprefix, sizeof(lenprefix)-1);
    if (consumed != -EAGAIN)
    {
      printf("Consumed %zd expected -EAGAIN\n", consumed);
      abort();
    }
  }

  printf("----------------\n");

  lenprefix_parserctx_init(&pctx);
  for (i = 0; i < sizeof(lenprefix)-1; i++)
  {
    consumed = lenprefix_parse_block(&pctx, lenprefix+i, 1);
    if (consumed != -EAGAIN)
    {
      printf("Consumed %zd expected -EAGAIN\n", consumed);
      abort();
    }
  }

  return 0;
}
