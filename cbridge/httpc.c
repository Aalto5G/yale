#include "httppycparser.h"
#include "httppycommon.h"
#include "httpc.h"

void
http_cctx_init(struct http_cctx *cctx)
{
    cctx->sz = 0;
    cctx->ok = 0;
    httppy_parserctx_init(&cctx->pctx);
}

ssize_t
http_cctx_feed(struct http_cctx *cctx, const char *dat, size_t sz, int eof)
{
  return httppy_parse_block(&cctx->pctx, dat, sz, eof);
}

ssize_t store(const char *buf, size_t siz, int start, struct httppy_parserctx *btn)
{
  struct http_cctx *cctx = CONTAINER_OF(btn, struct http_cctx, pctx);
  if (cctx->sz + siz > sizeof(cctx->buf) - 1)
  {
    siz = sizeof(cctx->buf) - cctx->sz - 1;
  }
  memcpy(cctx->buf+cctx->sz, buf, siz);
  cctx->sz += siz;
  cctx->buf[cctx->sz] = '\0';
  if (start & YALE_FLAG_MAJOR_MISTAKE)
  {
    abort();
  }
  if (start & YALE_FLAG_END)
  {
    if (cctx->sz > 0 && cctx->buf[cctx->sz - 1] == '\n')
    {
      cctx->sz--;
      cctx->buf[cctx->sz] = '\0';
    }
    if (cctx->sz > 0 && cctx->buf[cctx->sz - 1] == '\r')
    {
      cctx->sz--;
      cctx->buf[cctx->sz] = '\0';
    }
    cctx->ok = 1;
    return -EPIPE;
  }
  return -EAGAIN;
}
