#ifndef _RECURSIVECBCOMMON_H_
#define _RECURSIVECBCOMMON_H_

#include <stddef.h>
#include <unistd.h>

struct recursivecb_parserctx;
ssize_t f1(const char *buf, size_t siz, int start, struct recursivecb_parserctx *btn);
ssize_t f2(const char *buf, size_t siz, int start, struct recursivecb_parserctx *btn);
ssize_t f3(const char *buf, size_t siz, int start, struct recursivecb_parserctx *btn);
ssize_t f4(const char *buf, size_t siz, int start, struct recursivecb_parserctx *btn);

#endif
