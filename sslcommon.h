#ifndef _SSLCOMMON_H_
#define _SSLCOMMON_H_

#include <stddef.h>

struct ssl1_parserctx;
void szbe1(const char *buf, size_t siz, int start, struct ssl1_parserctx *btn);
void feed1(const char *buf, size_t siz, int start, struct ssl1_parserctx *btn);

struct ssl2_parserctx;
void szbe2(const char *buf, size_t siz, int start, struct ssl2_parserctx *btn);
void feed2(const char *buf, size_t siz, int start, struct ssl2_parserctx *btn);

struct ssl3_parserctx;
void szbe3(const char *buf, size_t siz, int start, struct ssl3_parserctx *btn);
void feed3(const char *buf, size_t siz, int start, struct ssl3_parserctx *btn);

struct ssl4_parserctx;
void szbe4(const char *buf, size_t siz, int start, struct ssl4_parserctx *btn);
void feed4(const char *buf, size_t siz, int start, struct ssl4_parserctx *btn);

struct ssl5_parserctx;
void szbe5(const char *buf, size_t siz, int start, struct ssl5_parserctx *btn);
void feed5(const char *buf, size_t siz, int start, struct ssl5_parserctx *btn);

struct ssl6_parserctx;
void szbe6(const char *buf, size_t siz, int start, struct ssl6_parserctx *btn);
void print6(const char *buf, size_t siz, int start, struct ssl6_parserctx *btn);

void szset32_3(const char *buf, size_t siz, int start, struct ssl3_parserctx *btn);

#endif
