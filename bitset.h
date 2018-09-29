#ifndef _BITSET_H_
#define _BITSET_H_

#include "yale.h"
#include "yaleuint.h"
#include <sys/uio.h>
#include <strings.h>

struct bitset {
  uint64_t bitset[(YALE_UINT_MAX_LEGAL+1+63)/64];
};

struct charbitset {
  uint64_t bitset[4];
};

static inline int myffsll(long long int i)
{
  int res = ffsll(i);
  if (res == 0)
  {
    return 65;
  }
  else
  {
    return res;
  }
}

static inline int bitset_empty(struct bitset *a)
{
  size_t i;
  for (i = 0; i < sizeof(a->bitset)/sizeof(*a->bitset); i++)
  {
    if (a->bitset[i] != 0)
    {
      return 0;
    }
  }
  return 1;
}

static inline yale_uint_t pick_rm_first(struct bitset *bs)
{
  size_t i;
  int j;
  int ffsres;
  for (i = 0; i < sizeof(bs->bitset)/sizeof(*bs->bitset); i++)
  {
    ffsres = ffsll(bs->bitset[i]);
    if (ffsres)
    {
      j = ffsres - 1;
      bs->bitset[i] &= ~(1ULL<<j);
      return i*64 + j;
    }
  }
  abort();
}

static inline void bitset_update(struct bitset *a, const struct bitset *b)
{
  size_t i;
  for (i = 0; i < sizeof(a->bitset)/sizeof(*a->bitset); i++)
  {
    a->bitset[i] |= b->bitset[i];
  }
}
static inline int bitset_equal(const struct bitset *a, const struct bitset *b)
{
  size_t i;
  for (i = 0; i < sizeof(a->bitset)/sizeof(*a->bitset); i++)
  {
    if (a->bitset[i] != b->bitset[i])
    {
      return 0;
    }
  }
  return 1;
}

static inline int bitset_issubset(const struct bitset *ba, const struct bitset *bb)
{
  size_t i;
  for (i = 0; i < sizeof(ba->bitset)/sizeof(*ba->bitset); i++)
  {
    if (ba->bitset[0] & ~bb->bitset[0])
    {
      return 0;
    }
  }
  return 1;
}

static inline int has_bitset(struct bitset *bs, size_t bit)
{
  size_t wordoff = bit/64;
  size_t bitoff = bit%64;
  return !!(bs->bitset[wordoff] & (1ULL<<bitoff));
}

static inline void set_bitset(struct bitset *bs, size_t bit)
{
  size_t wordoff = bit/64;
  size_t bitoff = bit%64;
  bs->bitset[wordoff] |= (1ULL<<bitoff);
}

static inline void clr_bitset(struct bitset *bs, size_t bit)
{
  size_t wordoff = bit/64;
  size_t bitoff = bit%64;
  bs->bitset[wordoff] &= ~(1ULL<<bitoff);
}

#endif
