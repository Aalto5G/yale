#include "httppycparser.h"
#include "httppycommon.h"

struct http_pyctx {
  struct httppy_parserctx pctx;
  char buf[1024];
  size_t sz;
  int ok;
};


ssize_t store(const char *buf, size_t siz, int start, struct httppy_parserctx *btn)
{
  struct http_pyctx *pyctx = CONTAINER_OF(btn, struct http_pyctx, pctx);
  if (pyctx->sz + siz > sizeof(pyctx->buf) - 1)
  {
    siz = sizeof(pyctx->buf) - pyctx->sz - 1;
  }
  memcpy(pyctx->buf+pyctx->sz, buf, siz);
  pyctx->sz += siz;
  pyctx->buf[pyctx->sz] = '\0';
  if (start & YALE_FLAG_MAJOR_MISTAKE)
  {
    abort();
  }
  //printf("buf %s end\n", pyctx->buf);
  if (start & YALE_FLAG_END)
  {
#if 0
    if (pyctx->sz > 0 && pyctx->buf[pyctx->sz - 1] == '\n')
    {
      pyctx->sz--;
      pyctx->buf[pyctx->sz] = '\0';
    }
    if (pyctx->sz > 0 && pyctx->buf[pyctx->sz - 1] == '\r')
    {
      pyctx->sz--;
      pyctx->buf[pyctx->sz] = '\0';
    }
#endif
    pyctx->ok = 1;
    return -EPIPE;
  }
  return -EAGAIN;
}

const struct http_pyctx pyctx0 = {};

int main(int argc, char **argv)
{
  struct http_pyctx pyctx = pyctx0;
  const char blkall[] = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
  const char blk1[] = "GET / HTTP/1.1\r\n";
  const char blk2[] = "Host: localhost\r\n";
  const char blk3[] = "Foo: bar\r\n";
  const char blkcrlf[] = "\r\n";
  int dummybegin, dummyend, i;
  ssize_t ret;
  httppy_parserctx_init(&pyctx.pctx);
  ret = httppy_parse_block(&pyctx.pctx, blkall, sizeof(blkall)-1, 0);
  printf("%zd\n", (ssize_t)ret);
  printf("BEGIN\n%s\nEND ok %d\n", pyctx.buf, pyctx.ok);
  for (dummybegin = 0; dummybegin < 3; dummybegin++)
  {
    for (dummyend = 0; dummyend < 3; dummyend++)
    {
      printf("--------\n");
      printf("%d %d\n", dummybegin, dummyend);
      pyctx = pyctx0;
      httppy_parserctx_init(&pyctx.pctx);
      httppy_parse_block(&pyctx.pctx, blk1, sizeof(blk1)-1, 0);
      printf("%zd\n", (ssize_t)ret);
      printf("BEGIN\n%s\nEND ok %d\n", pyctx.buf, pyctx.ok);
      for (i = 0; i < dummybegin; i++)
      {
        httppy_parse_block(&pyctx.pctx, blk3, sizeof(blk3)-1, 0);
        printf("%zd\n", (ssize_t)ret);
        printf("BEGIN\n%s\nEND ok %d\n", pyctx.buf, pyctx.ok);
      }
      httppy_parse_block(&pyctx.pctx, blk2, sizeof(blk2)-1, 0);
      printf("%zd\n", (ssize_t)ret);
      printf("BEGIN\n%s\nEND ok %d\n", pyctx.buf, pyctx.ok);
      for (i = 0; i < dummyend; i++)
      {
        httppy_parse_block(&pyctx.pctx, blk3, sizeof(blk3)-1, 0);
        printf("%zd\n", (ssize_t)ret);
        printf("BEGIN\n%s\nEND ok %d\n", pyctx.buf, pyctx.ok);
      }
      httppy_parse_block(&pyctx.pctx, blkcrlf, sizeof(blkcrlf)-1, 0);
      printf("%zd\n", (ssize_t)ret);
      printf("BEGIN\n%s\nEND ok %d\n", pyctx.buf, pyctx.ok);
    }
  }
}
