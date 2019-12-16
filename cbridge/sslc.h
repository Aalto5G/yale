#ifndef _SSLC_H_
#define _SSLC_H_

#include "ssl1cparser.h"

struct ssl_cctx {
  struct ssl1_parserctx pctx;
  char buf[1024];
  size_t sz;
  int ok;
};

void
ssl_cctx_init(struct ssl_cctx *cctx);

ssize_t
ssl_cctx_feed(struct ssl_cctx *cctx, const char *dat, size_t sz, int eof);

#endif
