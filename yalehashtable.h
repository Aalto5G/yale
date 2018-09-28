#ifndef _YALEHASHTABLE_H_
#define _YALEHASHTABLE_H_

#include "yalehashlist.h"
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

typedef uint32_t (*hash_fn)(struct yale_hash_list_node *node, void *userdata);

struct yale_hash_table {
  struct yale_hash_list_head *buckets;
  size_t bucketcnt;
  hash_fn fn; 
  void *fn_userdata;
  size_t itemcnt;
};

static inline size_t next_highest_power_of_2(size_t x)
{
  x--;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
#if SIZE_MAX > (4U*1024U*1024U*1024U)
  x |= x >> 32;
#endif
  x++;
  return x;
}

static inline int yale_hash_table_init_impl(
  struct yale_hash_table *table, size_t bucketcnt, hash_fn fn, void *fn_userdata)
{
  size_t i;
  table->fn = fn;
  table->fn_userdata = fn_userdata;
  table->itemcnt = 0;
  table->bucketcnt = next_highest_power_of_2(bucketcnt);
  table->buckets = malloc(sizeof(*table->buckets)*table->bucketcnt);
  if (table->buckets == NULL)
  {
    table->fn = NULL;
    table->fn_userdata = NULL;
    table->bucketcnt = 0;
    return -ENOMEM;
  }
  for (i = 0; i < table->bucketcnt; i++)
  {
    yale_hash_list_head_init(&table->buckets[i]);
  }
  return 0;
}

static inline int yale_hash_table_init(
  struct yale_hash_table *table, size_t bucketcnt, hash_fn fn, void *fn_userdata)
{
  return yale_hash_table_init_impl(table, bucketcnt, fn, fn_userdata);
}

static inline void yale_hash_table_free(struct yale_hash_table *table)
{
  if (table->itemcnt)
  {
    abort();
  }
  free(table->buckets);
  table->buckets = NULL;
  table->bucketcnt = 0;
  table->fn = NULL;
  table->fn_userdata = NULL;
  table->itemcnt = 0;
}

static inline void yale_hash_table_delete(
  struct yale_hash_table *table, struct yale_hash_list_node *node)
{
  yale_hash_list_delete(node);
  if (table->itemcnt == 0)
  {
    abort();
  }
  table->itemcnt--;
}

static inline void yale_hash_table_add_nogrow(
  struct yale_hash_table *table, struct yale_hash_list_node *node, uint32_t hashval)
{
  yale_hash_list_add_head(node, &table->buckets[hashval & (table->bucketcnt - 1)]);
  table->itemcnt++;
}

#define YALE_HASH_TABLE_FOR_EACH(table, bucket, x) \
  for ((bucket) = 0, x = NULL; x == NULL && (bucket) < (table)->bucketcnt; (bucket)++) \
    YALE_HASH_LIST_FOR_EACH(x, &(table)->buckets[bucket])

#define YALE_HASH_TABLE_FOR_EACH_SAFE(table, bucket, x, n) \
  for ((bucket) = 0, x = NULL; x == NULL && (bucket) < (table)->bucketcnt; (bucket)++) \
    YALE_HASH_LIST_FOR_EACH_SAFE(x, n, &(table)->buckets[bucket])

#define YALE_HASH_TABLE_FOR_EACH_POSSIBLE(table, x, hashval) \
  YALE_HASH_LIST_FOR_EACH(x, &((table)->buckets[hashval & ((table)->bucketcnt - 1)]))

#define YALE_HASH_TABLE_FOR_EACH_POSSIBLE_SAFE(table, x, n, hashval) \
  YALE_HASH_LIST_FOR_EACH_SAFE(x, n, &((table)->buckets[hashval & ((table)->bucketcnt - 1)]))

#endif
