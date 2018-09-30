#ifndef _LENPREFIXCOMMON_H_
#define _LENPREFIXCOMMON_H_

#include <stddef.h>

struct lenprefix_parserctx;
void print(const char *buf, size_t siz, int start, struct lenprefix_parserctx *btn);
void szbe(const char *buf, size_t siz, int start, struct lenprefix_parserctx *btn);

#endif
