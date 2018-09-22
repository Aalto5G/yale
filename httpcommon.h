#ifndef _HTTPCOMMON_H_
#define _HTTPCOMMON_H_

#include <stddef.h>

//void print(const char *buf, size_t siz, void *btn);
//void printsp(const char *buf, size_t siz, void *btn);
struct http_parserctx;
void print(const char *buf, size_t siz, struct http_parserctx *btn);
void printsp(const char *buf, size_t siz, struct http_parserctx *btn);

#endif
