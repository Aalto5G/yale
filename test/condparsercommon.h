#ifndef _CONDPARSERCOMMON_H_
#define _CONDPARSERCOMMON_H_

#include <stddef.h>
#include <unistd.h>

struct condparser_parserctx;
ssize_t setzero(const char *buf, size_t siz, int start, struct condparser_parserctx *btn);
ssize_t setone(const char *buf, size_t siz, int start, struct condparser_parserctx *btn);

#endif
