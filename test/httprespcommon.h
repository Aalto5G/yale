#ifndef _HTTPRESPCOMMON_H_
#define _HTTPRESPCOMMON_H_

#include <stddef.h>
#include <unistd.h>

struct httpresp_parserctx;
ssize_t sztxt(const char *buf, size_t siz, int start, struct httpresp_parserctx *btn);
ssize_t invalidate(const char *buf, size_t siz, int start, struct httpresp_parserctx *btn);

#endif
