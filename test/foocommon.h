#ifndef _FOOCOMMON_H_
#define _FOOCOMMON_H_

#include <stddef.h>
#include "yalecommon2.h"

struct fooparser_parserctx;
yale_ssize_t barcb(const char *buf, size_t siz, int start, struct fooparser_parserctx *btn);

#endif
