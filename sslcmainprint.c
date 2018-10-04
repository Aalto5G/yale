#include "sslcommon.h"
#include <stddef.h>
#include "ssl1cparser.h"
#include <sys/time.h>

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
  return ssl2_parse_block(&pctx->ssl2, buf, siz);
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
  return ssl3_parse_block(&pctx->ssl3, buf, siz);
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
  if (flags & YALE_FLAG_START)
  {
    ssl4_parserctx_init(&pctx->ssl4);
  }
  return ssl4_parse_block(&pctx->ssl4, buf, siz);
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
  if (flags & YALE_FLAG_START)
  {
    ssl5_parserctx_init(&pctx->ssl5);
  }
  return ssl5_parse_block(&pctx->ssl5, buf, siz);
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
  if (flags & YALE_FLAG_START)
  {
    ssl6_parserctx_init(&pctx->ssl6);
  }
  return ssl6_parse_block(&pctx->ssl6, buf, siz);
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

ssize_t print6(const char *buf, size_t siz, int flags, struct ssl6_parserctx *pctx)
{
  const char *ubuf = buf;
  size_t i;
  if (flags & YALE_FLAG_START)
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
  if (flags & YALE_FLAG_START)
  {
    putchar('>');
  }
  else
  {
    putchar(']');
  }
  if (flags & YALE_FLAG_END)
  {
    putchar('-');
  }
  putchar('\n');
  return -EAGAIN;
}

ssize_t szset32_3(const char *buf, size_t siz, int flags, struct ssl3_parserctx *pctx)
{
  pctx->bytes_sz = 32;
  return -EAGAIN;
}

int main(int argc, char **argv)
{
  ssize_t consumed;
  size_t i;
  struct ssl1_parserctx pctx = {};
  char withsni[] = {
0x16,0x03,0x01,0x00,0xbd,0x01,0x00,0x00,0xb9,0x03,0x03,0x8a,0x80,0x22,0x0f,0x8d
,0x60,0x13,0x99,0x8b,0x4b,0xfa,0x96,0xba,0x7a,0xeb,0x81,0x60,0x80,0xe4,0xc7,0x9e
,0xd0,0x4e,0x18,0x4e,0xc5,0xd5,0x74,0x17,0x23,0xb1,0xa1,0x00,0x00,0x38,0xc0,0x2c
,0xc0,0x30,0x00,0x9f,0xcc,0xa9,0xcc,0xa8,0xcc,0xaa,0xc0,0x2b,0xc0,0x2f,0x00,0x9e
,0xc0,0x24,0xc0,0x28,0x00,0x6b,0xc0,0x23,0xc0,0x27,0x00,0x67,0xc0,0x0a,0xc0,0x14
,0x00,0x39,0xc0,0x09,0xc0,0x13,0x00,0x33,0x00,0x9d,0x00,0x9c,0x00,0x3d,0x00,0x3c
,0x00,0x35,0x00,0x2f,0x00,0xff,0x01,0x00,0x00,0x58,0x00,0x00,0x00,0x0e,0x00,0x0c
,0x00,0x00,0x09,0x6c,0x6f,0x63,0x61,0x6c,0x68,0x6f,0x73,0x74,0x00,0x0b,0x00,0x04
,0x03,0x00,0x01,0x02,0x00,0x0a,0x00,0x0a,0x00,0x08,0x00,0x1d,0x00,0x17,0x00,0x19
,0x00,0x18,0x00,0x23,0x00,0x00,0x00,0x16,0x00,0x00,0x00,0x17,0x00,0x00,0x00,0x0d
,0x00,0x20,0x00,0x1e,0x06,0x01,0x06,0x02,0x06,0x03,0x05,0x01,0x05,0x02,0x05,0x03
,0x04,0x01,0x04,0x02,0x04,0x03,0x03,0x01,0x03,0x02,0x03,0x03,0x02,0x01,0x02,0x02
,0x02,0x03
  };

  printf("sz: %zu\n", sizeof(pctx));


  for (i = 0; i < 1; i++)
  {
    ssl1_parserctx_init(&pctx);
    consumed = ssl1_parse_block(&pctx, withsni, sizeof(withsni));
    if (consumed != -EAGAIN && consumed != sizeof(withsni))
    {
      printf("Consumed %zd expected -EAGAIN/%d\n", consumed, (int)sizeof(withsni));
      abort();
    }
  }

  printf("----------------\n");

  ssl1_parserctx_init(&pctx);
  for (i = 0; i < sizeof(withsni); i++)
  {
    consumed = ssl1_parse_block(&pctx, withsni+i, 1);
    if (i == sizeof(withsni) - 1)
    {
      if (consumed != 1 && consumed != -EAGAIN)
      {
        printf("Consumed %zd expected -EAGAIN/1 i=%d\n", consumed, (int)i);
        abort();
      }
    }
    else if (consumed != -EAGAIN)
    {
      printf("Consumed %zd expected -EAGAIN i=%d\n", consumed, (int)i);
      abort();
    }
  }

  return 0;
}
