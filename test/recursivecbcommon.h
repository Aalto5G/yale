#ifndef _RECURSIVECBCOMMON_H_
#define _RECURSIVECBCOMMON_H_

#include <stddef.h>
#include "yalecommon2.h"

struct recursivecb_parserctx;
yale_ssize_t f1(const char *buf, size_t siz, int start, struct recursivecb_parserctx *btn);
yale_ssize_t f2(const char *buf, size_t siz, int start, struct recursivecb_parserctx *btn);
yale_ssize_t f3(const char *buf, size_t siz, int start, struct recursivecb_parserctx *btn);
yale_ssize_t f4(const char *buf, size_t siz, int start, struct recursivecb_parserctx *btn);

#endif
