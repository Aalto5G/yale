#include "yale.h"

struct bitset {
  uint64_t bitset[4];
};

int bitset_empty(struct bitset *a)
{
  size_t i;
  for (i = 0; i < 4; i++)
  {
    if (a->bitset[i] != 0)
    {
      return 0;
    }
  }
  return 1;
}

uint8_t pick_rm_first(struct bitset *bs)
{
  size_t i;
  int j;
  int ffsres;
  for (i = 0; i < 4; i++)
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

void bitset_update(struct bitset *a, const struct bitset *b)
{
  size_t i;
  for (i = 0; i < 4; i++)
  {
    a->bitset[i] |= b->bitset[i];
  }
}
int bitset_equal(const struct bitset *a, const struct bitset *b)
{
  size_t i;
  for (i = 0; i < 4; i++)
  {
    if (a->bitset[i] != b->bitset[i])
    {
      return 0;
    }
  }
  return 1;
}

uint8_t bitset_issubset(const struct bitset *ba, const struct bitset *bb)
{
  size_t i;
  for (i = 0; i < 4; i++)
  {
    if (ba->bitset[0] & ~bb->bitset[0])
    {
      return 0;
    }
  }
  return 1;
}

uint8_t has_bitset(struct bitset *bs, uint8_t bit)
{
  uint8_t wordoff = bit/64;
  uint8_t bitoff = bit%64;
  return !!(bs->bitset[wordoff] & (1ULL<<bitoff));
}

void set_bitset(struct bitset *bs, uint8_t bit)
{
  uint8_t wordoff = bit/64;
  uint8_t bitoff = bit%64;
  bs->bitset[wordoff] |= (1ULL<<bitoff);
}

struct dict {
  struct bitset bitset[256];
  struct bitset has;
};

void firstset_update(struct dict *da, struct dict *db)
{
  size_t i;
  for (i = 0; i < 256; i++)
  {
    bitset_update(&da->bitset[i], &db->bitset[i]);
  }
}

void firstset_settoken(struct dict *da, struct ruleitem *ri)
{
  if (ri->is_action)
  {
    abort();
  }
  if (ri->cb != 255)
  {
    set_bitset(&da->bitset[ri->value], ri->cb);
  }
  set_bitset(&da->has, ri->value);
}

void firstset_setsingleton(struct dict *da, struct ruleitem *ri)
{
  memset(da, 0, sizeof(*da));
  firstset_settoken(da, ri);
}

int firstset_issubset(struct dict *da, struct dict *db)
{
  size_t i;
  for (i = 0; i < 256; /*i++*/)
  {
    uint8_t wordoff = i/64;
    uint8_t bitoff = i%64;
    if (da->has.bitset[wordoff] & (1ULL<<bitoff))
    {
      if (!(db->has.bitset[wordoff] & (1ULL<<bitoff)))
      {
        return 0;
      }
      if (!bitset_issubset(&da->bitset[i], &db->bitset[i]))
      {
        return 0;
      }
    }
    if (bitoff != 63)
    { 
      i = (wordoff*64) + ffsll(da->has.bitset[wordoff] & ~((1ULL<<(bitoff+1))-1)) - 1;
    }
    else
    {
      i++;
    }
  }
  return 1;
}
