#ifndef _REPREFIXCOMMON_H_
#define _REPREFIXCOMMON_H_

#include <stddef.h>
#include <unistd.h>

struct reprefix_parserctx;
ssize_t print(const char *buf, size_t siz, int start, struct reprefix_parserctx *btn);
ssize_t printall(const char *buf, size_t siz, int start, struct reprefix_parserctx *btn);
ssize_t printcont(const char *buf, size_t siz, int start, struct reprefix_parserctx *btn);
ssize_t szbe(const char *buf, size_t siz, int start, struct reprefix_parserctx *btn);

#endif
