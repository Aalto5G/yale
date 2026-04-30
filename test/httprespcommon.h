#ifndef _HTTPRESPCOMMON_H_
#define _HTTPRESPCOMMON_H_

#include <stddef.h>
#include "yalecommon2.h"

struct httpresp_parserctx;
yale_ssize_t sztxt(const char *buf, size_t siz, int start, struct httpresp_parserctx *btn);
yale_ssize_t invalidate(const char *buf, size_t siz, int start, struct httpresp_parserctx *btn);

#endif
