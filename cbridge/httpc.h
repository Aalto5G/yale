#ifndef _HTTPC_H_
#define _HTTPC_H_

#include "httppycparser.h"

struct http_cctx {
  struct httppy_parserctx pctx;
  char buf[1024];
  size_t sz;
  int ok;
};


void
http_cctx_init(struct http_cctx *cctx);

ssize_t
http_cctx_feed(struct http_cctx *cctx, const char *dat, size_t sz, int eof);

#endif
