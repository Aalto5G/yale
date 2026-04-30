#ifndef _HTTPPYCOMMON_H_
#define _HTTPPYCOMMON_H_

#include <stddef.h>
#include "yalecommon2.h"

struct httppy_parserctx;
yale_ssize_t store(const char *buf, size_t siz, int start, struct httppy_parserctx *btn);

#endif
