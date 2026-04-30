#ifndef _HTTPCOMMON_H_
#define _HTTPCOMMON_H_

#include <stddef.h>
#include "yalecommon2.h"

//void print(const char *buf, size_t siz, void *btn);
//void printsp(const char *buf, size_t siz, void *btn);
struct http_parserctx;
yale_ssize_t print(const char *buf, size_t siz, int start, struct http_parserctx *btn);
yale_ssize_t printsp(const char *buf, size_t siz, int start, struct http_parserctx *btn);

#endif
