#ifndef _BACKTRACKTESTCBCOMMON_H_
#define _BACKTRACKTESTCBCOMMON_H_

#include <stddef.h>
#include "yalecommon2.h"

struct backtracktestcb_parserctx;
yale_ssize_t cb1(const char *buf, size_t siz, int start, struct backtracktestcb_parserctx *btn);
yale_ssize_t cb2(const char *buf, size_t siz, int start, struct backtracktestcb_parserctx *btn);

#endif
