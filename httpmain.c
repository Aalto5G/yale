#include "httpparser.h"
#include <sys/time.h>

#undef DO_PRINT

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

void print(const char *buf, size_t siz, int start, void *btn)
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
void printsp(const char *buf, size_t siz, int start, void *btn)
{
#ifdef DO_PRINT
  putchar('<');
  putchar(' ');
  putchar('>');
  putchar('\n');
#endif
}

int main(int argc, char **argv)
{
  ssize_t consumed;
  size_t i;
  struct http_parserctx pctx = {};
  struct timeval tv1 = {}, tv2 = {};
  double us;
  char http[] =
    "GET /foo/bar/baz/barf/quux.html HTTP/1.1\r\n"
    "Host: www.google.fi\r\n"
    //"\twww.google2.fi\r\n"
    "User-Agent: Mozilla/5.0 (Linux; Android 7.0; SM-G930VC Build/NRD90M; wv) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/58.0.3029.83 Mobile Safari/537.36\r\n"
    "Accept: text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,image/jpeg,image/gif;q=0.2,*/*;q=0.1\r\n"
    "Accept-Language: en-us,en;q=0.5\r\n"
    "Accept-Encoding: gzip,deflate\r\n"
    "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
    "Keep-Alive: 300\r\n"
    "Connection: keep-alive\r\n"
    "Referer: http://www.google.fi/quux/barf/baz/bar/foo.html\r\n"
    "Cookie: PHPSESSID=298zf09hf012fh2; csrftoken=u32t4o3tb3gg43; _gat=1;\r\n"
    "\r\n";

  gettimeofday(&tv1, NULL);
  for (i = 0; i < 10 * 1000 * 1000 /* 1 */; i++)
  {
    http_parserctx_init(&pctx);
    consumed = http_parse_block(&pctx, http, sizeof(http)-1);
    if (consumed != sizeof(http)-1)
    {
      abort();
    }
  }
  gettimeofday(&tv2, NULL);
  us = (tv2.tv_sec - tv1.tv_sec)/1e1 + (tv2.tv_usec - tv1.tv_usec)/1e7;
  printf("%g us\n", us);
  printf("%g Gbps\n", (sizeof(http)-1)*8/us/1e3);

  return 0;
}
