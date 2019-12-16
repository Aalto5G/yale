#include "ssl1cparser.h"
#include "sslcommon.h"
#include "sslc.h"

void
ssl_cctx_init(struct ssl_cctx *cctx)
{
    cctx->sz = 0;
    cctx->ok = 0;
    ssl1_parserctx_init(&cctx->pctx);
}

ssize_t
ssl_cctx_feed(struct ssl_cctx *cctx, const char *dat, size_t sz, int eof)
{
    return ssl1_parse_block(&cctx->pctx, dat, sz, eof);
}

ssize_t szbe1(const char *buf, size_t siz, int flags, struct ssl1_parserctx *pctx)
{
  size_t i;
  if (flags & YALE_FLAG_START)
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
ssize_t feed1(const char *buf, size_t siz, int flags, struct ssl1_parserctx *pctx)
{
  return ssl2_parse_block(&pctx->ssl2, buf, siz, 0); // FIXME eofindicator
}

ssize_t szbe2(const char *buf, size_t siz, int flags, struct ssl2_parserctx *pctx)
{
  size_t i;
  if (flags & YALE_FLAG_START)
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
ssize_t feed2(const char *buf, size_t siz, int flags, struct ssl2_parserctx *pctx)
{
  ssize_t result;
  if (flags & YALE_FLAG_START)
  {
    ssl3_parserctx_init(&pctx->ssl3);
  }
  result = ssl3_parse_block(&pctx->ssl3, buf, siz, !!(flags & YALE_FLAG_END)); // FIXME eofindicator
  if (result != -EAGAIN && result != -EWOULDBLOCK && result != (ssize_t)siz)
  {
    return result;
  }
  if (flags & YALE_FLAG_END)
  {
    if (pctx->ssl3.stacksz > 0)
    {
      return -EINVAL;
    }
  }
  return result;
}

ssize_t szset32_3(const char *buf, size_t siz, int flags, struct ssl3_parserctx *pctx)
{
  pctx->bytes_sz = 32;
  return -EAGAIN;
}

ssize_t szbe3(const char *buf, size_t siz, int flags, struct ssl3_parserctx *pctx)
{
  size_t i;
  if (flags & YALE_FLAG_START)
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
ssize_t feed3(const char *buf, size_t siz, int flags, struct ssl3_parserctx *pctx)
{
  ssize_t result;
  if (flags & YALE_FLAG_START)
  {
    ssl4_parserctx_init(&pctx->ssl4);
  }
  result = ssl4_parse_block(&pctx->ssl4, buf, siz, !!(flags & YALE_FLAG_END));
  if (result != -EAGAIN && result != -EWOULDBLOCK && result != (ssize_t)siz)
  {
    return result;
  }
  if (flags & YALE_FLAG_END)
  {
    if (pctx->ssl4.stacksz != 1)
    {
      return -EINVAL;
    }
  }
  return result;
}

ssize_t szbe4(const char *buf, size_t siz, int flags, struct ssl4_parserctx *pctx)
{
  size_t i;
  if (flags & YALE_FLAG_START)
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
ssize_t feed4(const char *buf, size_t siz, int flags, struct ssl4_parserctx *pctx)
{
  ssize_t result;
  if (flags & YALE_FLAG_START)
  {
    ssl5_parserctx_init(&pctx->ssl5);
  }
  result = ssl5_parse_block(&pctx->ssl5, buf, siz, !!(flags & YALE_FLAG_END));
  if (result != -EAGAIN && result != -EWOULDBLOCK && result != (ssize_t)siz)
  {
    return result;
  }
  if (flags & YALE_FLAG_END)
  {
    if (pctx->ssl5.stacksz > 0)
    {
      return -EINVAL;
    }
  }
  return result;
}

ssize_t szbe5(const char *buf, size_t siz, int flags, struct ssl5_parserctx *pctx)
{
  size_t i;
  if (flags & YALE_FLAG_START)
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
ssize_t feed5(const char *buf, size_t siz, int flags, struct ssl5_parserctx *pctx)
{
  ssize_t result;
  if (flags & YALE_FLAG_START)
  {
    ssl6_parserctx_init(&pctx->ssl6);
  }
  result = ssl6_parse_block(&pctx->ssl6, buf, siz, !!(flags & YALE_FLAG_END));
  if (result != -EAGAIN && result != -EWOULDBLOCK && result != (ssize_t)siz)
  {
    return result;
  }
  if (flags & YALE_FLAG_END)
  {
    if (pctx->ssl6.stacksz != 1)
    {
      return -EINVAL;
    }
  }
  return result;
}

ssize_t szbe6(const char *buf, size_t siz, int flags, struct ssl6_parserctx *pctx)
{
  size_t i;
  if (flags & YALE_FLAG_START)
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
  


ssize_t print6(const char *buf, size_t siz, int start, struct ssl6_parserctx *btn)
{
  struct ssl5_parserctx *ctx5 = CONTAINER_OF(btn, struct ssl5_parserctx, ssl6);
  struct ssl4_parserctx *ctx4 = CONTAINER_OF(ctx5, struct ssl4_parserctx, ssl5);
  struct ssl3_parserctx *ctx3 = CONTAINER_OF(ctx4, struct ssl3_parserctx, ssl4);
  struct ssl2_parserctx *ctx2 = CONTAINER_OF(ctx3, struct ssl2_parserctx, ssl3);
  struct ssl1_parserctx *ctx1 = CONTAINER_OF(ctx2, struct ssl1_parserctx, ssl2);
  struct ssl_cctx *cctx = CONTAINER_OF(ctx1, struct ssl_cctx, pctx);
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
    cctx->ok = 1;
    return -EPIPE;
  }
  return -EAGAIN;
}
