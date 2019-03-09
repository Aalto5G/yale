#include "fooparsercparser.h"
#include "foocommon.h"

ssize_t barcb(const char *buf, size_t siz, int start, struct fooparser_parserctx *btn)
{
  const char *ubuf = buf;
  size_t i;
  putchar('<');
  for (i = 0; i < siz; i++)
  {
    putchar(ubuf[i]);
  }
  putchar('>');
  if (start & YALE_FLAG_END)
  {
    putchar('-');
  }
  if (start & YALE_FLAG_MAJOR_MISTAKE)
  {
    putchar('!');
  }
  putchar('\n');
  return -EAGAIN;
}

int main(int argc, char **argv)
{
  ssize_t consumed;
  size_t i;
  struct fooparser_parserctx pctx = {};
  char stream[] = "foobarfoobar";

  for (i = 0; i < /* 1000 * 1000 */ 1 ; i++)
  {
    fooparser_parserctx_init(&pctx);
    consumed = fooparser_parse_block(&pctx, stream, sizeof(stream)-1, 1);
    printf("Consumed: %zd\n", consumed);
    if (consumed != sizeof(stream)-1 && consumed != -EAGAIN)
    {
      abort();
    }
  }

  return 0;
}
