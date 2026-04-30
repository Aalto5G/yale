#ifndef _SSLCOMMON_H_
#define _SSLCOMMON_H_

#include <stddef.h>
#include "yalecommon2.h"

struct ssl1_parserctx;
yale_ssize_t szbe1(const char *buf, size_t siz, int start, struct ssl1_parserctx *btn);
yale_ssize_t feed1(const char *buf, size_t siz, int start, struct ssl1_parserctx *btn);

struct ssl2_parserctx;
yale_ssize_t szbe2(const char *buf, size_t siz, int start, struct ssl2_parserctx *btn);
yale_ssize_t feed2(const char *buf, size_t siz, int start, struct ssl2_parserctx *btn);

struct ssl3_parserctx;
yale_ssize_t szbe3(const char *buf, size_t siz, int start, struct ssl3_parserctx *btn);
yale_ssize_t feed3(const char *buf, size_t siz, int start, struct ssl3_parserctx *btn);

struct ssl4_parserctx;
yale_ssize_t szbe4(const char *buf, size_t siz, int start, struct ssl4_parserctx *btn);
yale_ssize_t feed4(const char *buf, size_t siz, int start, struct ssl4_parserctx *btn);

struct ssl5_parserctx;
yale_ssize_t szbe5(const char *buf, size_t siz, int start, struct ssl5_parserctx *btn);
yale_ssize_t feed5(const char *buf, size_t siz, int start, struct ssl5_parserctx *btn);

struct ssl6_parserctx;
yale_ssize_t szbe6(const char *buf, size_t siz, int start, struct ssl6_parserctx *btn);
yale_ssize_t print6(const char *buf, size_t siz, int start, struct ssl6_parserctx *btn);

yale_ssize_t szset32_3(const char *buf, size_t siz, int start, struct ssl3_parserctx *btn);

#endif
