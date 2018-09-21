#include "httpcommon.h"
#include <stdio.h>

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

#undef DO_PRINT

void print(const char *buf, size_t siz, void *btn)
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
}
void printsp(const char *buf, size_t siz, void *btn)
{
#ifdef DO_PRINT
  putchar('<');
  putchar(' ');
  putchar('>');
  putchar('\n');
#endif
}
