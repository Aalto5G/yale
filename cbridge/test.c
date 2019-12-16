#include <stdio.h>
#include "httpc.h"

int main(int argc, char **argv)
{
  struct http_cctx ctx;
  http_cctx_init(&ctx);
  const char *str = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
  http_cctx_feed(&ctx, str, strlen(str), 0);
  if (!ctx.ok)
  {
    abort();
  }
  printf("%s\n", ctx.buf);
  return 0;
}
