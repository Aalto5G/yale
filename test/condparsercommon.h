#ifndef _CONDPARSERCOMMON_H_
#define _CONDPARSERCOMMON_H_

#include <stddef.h>
#include "yalecommon2.h"

struct condparser_parserctx;
yale_ssize_t setzero(const char *buf, size_t siz, int start, struct condparser_parserctx *btn);
yale_ssize_t setone(const char *buf, size_t siz, int start, struct condparser_parserctx *btn);

#endif
