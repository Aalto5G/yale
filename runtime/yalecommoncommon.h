#ifndef _YALECOMMONCOMMON_H_
#define _YALECOMMONCOMMON_H_

#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

enum yale_special_flags {
  YALE_SPECIAL_FLAG_BYTES = (1<<0),
  YALE_SPECIAL_FLAG_SHORTCUT = (1<<1),
};

enum yale_flags {
  YALE_FLAG_START = (1<<0),
  YALE_FLAG_END = (1<<1),
  YALE_FLAG_ACTION = (1<<2),
  YALE_FLAG_MAJOR_MISTAKE = (1<<3),
};

#if _POSIX_VERSION >= 202405L
 // glibc incorrectly doesn't make this visible with _POSIX_C_SOURCE
 // RFE does this also require _XOPEN_SOURCE to be set to at least 800?
 #ifndef __GLIBC__
  #undef YALE_HAS_FFSLL
  #define YALE_HAS_FFSLL 1
 #endif
#endif

#if _POSIX_VERSION >= 200112L
  #if _XOPEN_VERSION >= 600
    #ifdef _XOPEN_SOURCE
      #if _XOPEN_SOURCE >= 600
        #undef YALE_HAS_FFS
        #define YALE_HAS_FFS 1
      #endif
    #endif
  #endif
#endif

#ifdef __GLIBC__
  #if __GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 12)
   #if _XOPEN_SOURCE >= 700 || !(_POSIX_C_SOURCE >= 200809L)
    #undef YALE_HAS_FFS
    #define YALE_HAS_FFS 1
   #endif
   #ifdef _DEFAULT_SOURCE
    #if __GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 20)
     #undef YALE_HAS_FFS
     #define YALE_HAS_FFS 1
    #endif
   #endif
   #ifdef _BSD_SOURCE
    #if __GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ <= 19)
     #undef YALE_HAS_FFS
     #define YALE_HAS_FFS 1
    #endif
   #endif
  #endif
  #if __GLIBC__  > 2 || (__GLIBC__  == 2 && __GLIBC_MINOR__  >= 27)
   #ifdef _DEFAULT_SOURCE
    #undef YALE_HAS_FFSLL
    #define YALE_HAS_FFSLL 1
   #endif
  #endif
  #ifdef _GNU_SOURCE // this is ancient but required _GNU_SOURCE before 2.27
    #undef YALE_HAS_FFSLL
    #define YALE_HAS_FFSLL 1
  #endif
#endif

#ifdef __FreeBSD__
  #if __FreeBSD_version >= 800000
    #undef YALE_HAS_FFSLL
    #define YALE_HAS_FFSLL 1
  #endif
#endif

#ifdef __has_builtin
  #if __has_builtin (__builtin_ffsll)
    #undef YALE_HAS_BFFSLL
    #define YALE_HAS_BFFSLL 1
  #endif
#endif

#ifdef __GNUC__
  #if __GNUC__ >= 4
    #undef YALE_HAS_BFFSLL
    #define YALE_HAS_BFFSLL 1
  #endif
#endif

#ifdef YALE_HAS_FFS
#include <strings.h>
static inline int yale_ffsu32(uint32_t v)
{
  return ffs((int)v);
}
static inline int yale_ffsu16(uint16_t v)
{
  return ffs((int)v);
}
static inline int yale_ffsu8(uint8_t v)
{
  return ffs((int)v);
}
#else
static inline int yale_ffsu32(uint32_t v)
{
  int r;
  static const int MultiplyDeBruijnBitPosition[32] =
  {
    0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
    31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
  };
  r = MultiplyDeBruijnBitPosition[((uint32_t)((v & -v) * 0x077CB531U)) >> 27];
  return v?(r+1):0;
}
static inline int yale_ffsu16(uint16_t v)
{
  int r;
  static const int MultiplyDeBruijnBitPosition[16] =
  {
    0,1,2,5,3,9,6,11,15,4,8,10,14,7,13,12
  };
  r = MultiplyDeBruijnBitPosition[((uint16_t)((v & -v) * 0x09af)) >> 12];
  return v?(r+1):0;
}
static inline int yale_ffsu8(uint8_t v)
{
  int r;
  static const int MultiplyDeBruijnBitPosition[8] =
  {
    0,1,6,2,7,5,4,3
  };
  r = MultiplyDeBruijnBitPosition[((uint8_t)((v & -v) * 0x1d)) >> 5];
  return v?(r+1):0;
}
#endif

#ifdef YALE_HAS_FFSLL
#include <strings.h>
#endif

static inline int yale_ffsu64(uint64_t x)
{
#ifdef YALE_HAS_BFFSLL
  return __builtin_ffsll((long long)x);
#else
#ifdef YALE_HAS_FFSLL
  return ffsll((long long)x);
#else
#if 0
  int res = yale_ffsu32((uint32_t)(x&0xFFFFFFFFU));
  if (res)
  {
    return res;
  }
  res = yale_ffsu32((uint32_t)(x>>32));
  if (res)
  {
    return res+32;
  }
  return 0;
#else
  int r;
  static const int MultiplyDeBruijnBitPosition[64] =
  {
    0,
    1,  2, 53,  3,  7, 54, 27, 4, 38, 41,  8, 34, 55, 48, 28,
    62,  5, 39, 46, 44, 42, 22,  9, 24, 35, 59, 56, 49, 18, 29, 11,
    63, 52,  6, 26, 37, 40, 33, 47, 61, 45, 43, 21, 23, 58, 17, 10,
    51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19, 15, 30, 14, 13, 12
  };
  r = MultiplyDeBruijnBitPosition[((uint64_t)((v & -v) * 0x022fdd63cc95386dULL)) >> 58];
  return v?(r+1):0;
#endif
#endif
#endif
}

#if SIZE_MAX == UINT16_MAX
typedef int16_t yale_ssize_t;
#elif SIZE_MAX == UINT32_MAX
typedef int32_t yale_ssize_t;
#elif SIZE_MAX == UINT64_MAX
typedef int64_t yale_ssize_t;
#else
typedef ptrdiff_t yale_ssize_t;
#endif

#endif
