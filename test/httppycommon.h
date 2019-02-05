#ifndef _HTTPPYCOMMON_H_
#define _HTTPPYCOMMON_H_

#include <stddef.h>
#include <unistd.h>

struct httppy_parserctx;
ssize_t store(const char *buf, size_t siz, int start, struct httppy_parserctx *btn);

#endif
