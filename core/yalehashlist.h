#ifndef _YALEHASHLIST_H_
#define _YALEHASHLIST_H_

#include <stddef.h>

struct yale_hash_list_node {
  struct yale_hash_list_node **pprev;
  struct yale_hash_list_node *next;
};

struct yale_hash_list_head {
  struct yale_hash_list_node *next;
};

static inline void yale_hash_list_head_init(struct yale_hash_list_head *head)
{
  head->next = NULL;
}

static inline void yale_hash_list_node_init(struct yale_hash_list_node *head)
{
  head->pprev = NULL;
  head->next = NULL;
}

static inline int yale_hash_list_is_empty(struct yale_hash_list_head *head)
{
  return !(head->next);
}

static inline void yale_hash_list_delete(struct yale_hash_list_node *node)
{
  *node->pprev = node->next;
  if (node->next)
  {
    node->next->pprev = node->pprev;
  }
  node->next = NULL;
  node->pprev = NULL;
}

static inline void yale_hash_list_add_head(
  struct yale_hash_list_node *node, struct yale_hash_list_head *head)
{
  node->next = head->next;
  node->pprev = &(head->next);
  if (head->next)
  {
    head->next->pprev = &(node->next);
  }
  head->next = node;
}

#define YALE_HASH_LIST_FOR_EACH(x, head) \
  for (x = (head)->next; x; x = x->next)

#define YALE_HASH_LIST_FOR_EACH_SAFE(x, n, head) \
  for (x = (head)->next; x && ({n = x->next; 1;}); x = n)

#endif
