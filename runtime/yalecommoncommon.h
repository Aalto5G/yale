#ifndef _YALECOMMONCOMMON_H_
#define _YALECOMMONCOMMON_H_

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>

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
  #undef YALE_HAS_FFSLL
  #define YALE_HAS_FFSLL 1
#endif

#ifdef __GLIBC__
  #if __GLIBC__  > 2 || (__GLIBC__  == 2 && __GLIBC_MINOR__  >= 27)
    #undef YALE_HAS_FFSLL
    #define YALE_HAS_FFSLL 1
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

static inline int yale_ffsu64(uint64_t x)
{
#ifdef YALE_HAS_BFFSLL
  return __builtin_ffsll((long long)x);
#else
#ifdef YALE_HAS_FFSLL
  return ffsll((long long)x);
#else
  int res = ffs((int)(x&0xFFFFFFFFU));
  if (res)
  {
    return res;
  }
  res = ffs((int)(x>>32));
  if (res)
  {
    return res+32;
  }
  return 0;
#endif
#endif
}

#endif
