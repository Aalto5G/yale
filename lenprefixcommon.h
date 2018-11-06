#ifndef _LENPREFIXCOMMON_H_
#define _LENPREFIXCOMMON_H_

#include <stddef.h>
#include <unistd.h>

struct lenprefix_parserctx;
ssize_t print(const char *buf, size_t siz, int start, struct lenprefix_parserctx *btn);
ssize_t printall(const char *buf, size_t siz, int start, struct lenprefix_parserctx *btn);
ssize_t printcont(const char *buf, size_t siz, int start, struct lenprefix_parserctx *btn);
ssize_t szbe(const char *buf, size_t siz, int start, struct lenprefix_parserctx *btn);

#endif
