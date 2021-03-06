#include "httpcparser.h"
#include "httpcommon.h"

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

ssize_t print(const char *buf, size_t siz, int start, struct http_parserctx *btn)
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
#endif
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
ssize_t printsp(const char *buf, size_t siz, int start, struct http_parserctx *btn)
{
#ifdef DO_PRINT
  putchar('<');
  putchar(' ');
  putchar('>');
  putchar('\n');
#endif
  return -EAGAIN;
}

int main(int argc, char **argv)
{
  ssize_t consumed;
  size_t i;
  struct http_parserctx pctx = {};
  char http[] =
    "GET /foo/bar/baz/barf/quux.html HTTP/1.1\r\n"
    "Host: www.google.fi\r\n"
    "\twww.google2.fi\r\n"
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

  if (argc != 1 && argc != 2)
  {
    printf("Usage: %s [file.dat]\n", argv[0]);
    exit(1);
  }
  if (argc == 2)
  {
    FILE *f = fopen(argv[1], "rb");
    char buf[65536];
    size_t bytes_read;
    char *term;
    if (f == NULL)
    {
      printf("Can't open %s\n", argv[1]);
      exit(1);
    }
    bytes_read = fread(buf, 1, sizeof(buf), f);
    if (bytes_read == sizeof(buf))
    {
      printf("Too large file: %s\n", argv[1]);
      exit(1);
    }
    buf[bytes_read] = '\0';
    fclose(f);
    term = strstr(buf, "\r\n\r\n");
    if (term != NULL)
    {
      bytes_read = term - buf + 4;
    }
    http_parserctx_init(&pctx);
    consumed = http_parse_block(&pctx, buf, bytes_read, 1);
    if (consumed < 0 || (size_t)consumed != bytes_read)
    {
      abort();
    }
    return 0;
  }

  for (i = 0; i < /* 1000 * 1000 */ 1 ; i++)
  {
    http_parserctx_init(&pctx);
    consumed = http_parse_block(&pctx, http, sizeof(http)-1, 1);
    printf("Consumed: %zd\n", consumed);
    if (consumed != sizeof(http)-1)
    {
      abort();
    }
  }

  return 0;
}
