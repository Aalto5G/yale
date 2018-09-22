#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct bitset {
  uint64_t bitset[4];
};

struct dfa_node {
  uint8_t d[256];
  uint8_t default_tr;
  uint8_t tainted:1;
  uint8_t accepting:1;
  struct bitset acceptidset;
};

struct nfa_node {
  struct bitset d[256];
  struct bitset defaults;
  struct bitset epsilon;
  uint8_t accepting:1;
  uint8_t taintid;
};

void nfa_init(struct nfa_node *n, int accepting, int taintid)
{
  memset(n, 0, sizeof(*n));
  n->accepting = !!accepting;
  n->taintid = taintid;
}

void nfa_connect(struct nfa_node *n, char ch, uint8_t node2)
{
  uint8_t wordoff = node2/64;
  uint8_t bitoff = node2%64;
  n->d[(unsigned char)ch].bitset[wordoff] |= (1ULL<<bitoff);
}

void nfa_connect_epsilon(struct nfa_node *n, uint8_t node2)
{
  uint8_t wordoff = node2/64;
  uint8_t bitoff = node2%64;
  n->epsilon.bitset[wordoff] |= (1ULL<<bitoff);
}

void nfa_connect_default(struct nfa_node *n, uint8_t node2)
{
  uint8_t wordoff = node2/64;
  uint8_t bitoff = node2%64;
  n->defaults.bitset[wordoff] |= (1ULL<<bitoff);
}

#if 0
uint8_t ffs_bitset(const struct bitset *bs)
{
  size_t i;
  for (i = 0; i < 4; i++)
  {
    ffsll(bs->bitset[i]);
  }
}
#endif

void epsilonclosure(struct nfa_node *ns, struct bitset nodes,
                    struct bitset *closurep, int *tainted,
                    struct bitset *acceptidsetp)
{
  struct bitset closure = nodes;
  struct bitset taintidset = {};
  struct bitset acceptidset = {};
  uint8_t stack[256] = {};
  uint8_t nidx;
  size_t stacksz = 0;
  size_t i;
  uint16_t taintcnt = 0;
  for (i = 0; i < 256; i++)
  {
    uint8_t wordoff = i/64;
    uint8_t bitoff = i%64;
    if (nodes.bitset[wordoff] & (1ULL<<bitoff))
    {
      if (stacksz >= sizeof(stack)/sizeof(*stack))
      {
        abort();
      }
      stack[stacksz++] = i;
    }
  }
  if (stacksz > 0)
  {
    //printf("Epsilon closure stack size: %d\n", (int)stacksz);
  }
  while (stacksz > 0)
  {
    struct nfa_node *n;
    nidx = stack[--stacksz];
    n = &ns[nidx];
    for (i = 0; i < 256; i++)
    {
      uint8_t wordoff = i/64;
      uint8_t bitoff = i%64;
      if (n->epsilon.bitset[wordoff] & (1ULL<<bitoff))
      {
        if (!(closure.bitset[wordoff] & (1ULL<<bitoff)))
        {
          closure.bitset[wordoff] |= (1ULL<<bitoff);
          if (stacksz >= sizeof(stack)/sizeof(*stack))
          {
            abort();
          }
          stack[stacksz++] = i;
        }
      }
    }
  }
  for (i = 0; i < 256; i++)
  {
    uint8_t wordoff = i/64;
    uint8_t bitoff = i%64;
    if (closure.bitset[wordoff] & (1ULL<<bitoff))
    {
      struct nfa_node *n = &ns[i];
      if (n->taintid)
      {
        if (!(taintidset.bitset[wordoff] & (1ULL<<bitoff)))
        {
          taintidset.bitset[wordoff] |= (1ULL<<bitoff);
          taintcnt++;
        }
        if (n->accepting)
        {
          acceptidset.bitset[wordoff] |= (1ULL<<bitoff);
        }
      }
    }
  }
  *closurep = closure;
  *acceptidsetp = acceptidset;
  *tainted = (taintcnt > 1);
}

void dfa_init(struct dfa_node *n, int accepting, int tainted, struct bitset *acceptidset)
{
  uint16_t i;
  memset(n, 0, sizeof(*n));
  for (i = 0; i < 256; i++)
  {
    n->d[i] = 255;
  }
  n->default_tr = 255;
  n->accepting = !!accepting;
  n->tainted = !!tainted;
  n->acceptidset = *acceptidset;
}

void dfa_init_empty(struct dfa_node *n)
{
  struct bitset acceptidset = {};
  dfa_init(n, 0, 0, &acceptidset);
}

void dfa_connect(struct dfa_node *n, char ch, uint8_t node2)
{
  if (n->d[(unsigned char)ch] != 255 || node2 == 255)
  {
    printf("node2 %u\n", (unsigned)node2);
    printf("already connected %u\n", (unsigned)n->d[(unsigned char)ch]);
    abort();
  }
  n->d[(unsigned char)ch] = node2;
}

void dfa_connect_default(struct dfa_node *n, uint8_t node2)
{
  printf("connecting default\n");
  if (n->default_tr != 255 || node2 == 255)
  {
    printf("default conflict\n");
    printf("node2 %u\n", (unsigned)node2);
    printf("already connected %u\n", (unsigned)n->default_tr);
    abort();
  }
  n->default_tr = node2;
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

struct bitset_hash_item {
  struct bitset key;
  uint8_t dfanodeid;
};

struct bitset_hash {
  struct bitset_hash_item tbl[255];
  uint8_t tblsz;
};

void dfaviz(struct dfa_node *ds, uint8_t cnt)
{
  uint8_t i;
  uint16_t j;
  printf("digraph fsm {\n");
  for (i = 0; i < cnt; i++)
  {
    printf("n%d [label=\"%d%s%s\"];\n",
           (int)i, (int)i,
           ds[cnt].accepting ? "+" : "",
           ds[cnt].tainted ? "*" : "");
  }
  for (i = 0; i < cnt; i++)
  {
    for (j = 0; j < 256; j++)
    {
      if (ds[i].d[j] != 255)
      {
        printf("n%d -> n%d [label=\"%c\"];\n", i, ds[i].d[j], (char)j);
      }
    }
  }
  printf("}\n");
}

uint8_t nfa2dfa(struct nfa_node *ns, struct dfa_node *ds, uint8_t begin)
{
  struct bitset initial = {};
  struct bitset dfabegin = {};
  int tainted;
  struct bitset acceptidset = {};
  struct bitset defaults = {};
  const struct bitset defaults_empty = {};
  uint8_t wordoff = begin/64;
  uint8_t bitoff = begin%64;
  uint8_t curdfanode = 0;
  uint8_t dfanodeid, dfanodeid2;
  int accepting = 0;
  struct bitset queue[256];
  size_t queuesz;
  struct bitset_hash d = {};
  uint16_t i, j;

  initial.bitset[wordoff] |= (1ULL<<bitoff);

  epsilonclosure(ns, initial, &dfabegin, &tainted, &acceptidset);
  for (i = 0; i < 256; i++)
  {
    wordoff = i/64;
    bitoff = i%64;
    if (dfabegin.bitset[wordoff] & (1ULL<<bitoff))
    {
      if (ns[i].accepting)
      {
        accepting = 1;
        break;
      }
    }
  }

  d.tbl[d.tblsz].dfanodeid = curdfanode;
  memcpy(&d.tbl[d.tblsz++].key, &dfabegin, sizeof(dfabegin));
  dfa_init(&ds[curdfanode], accepting, tainted, &acceptidset);
  curdfanode++;

  queue[0] = dfabegin;
  queuesz = 1;

  while (queuesz)
  {
    struct bitset nns = queue[--queuesz];
    struct bitset d2[256] = {};
    struct bitset d2epsilon = {};
    struct bitset defaultsec;
    //printf("Iter\n");
    defaults = defaults_empty;
    for (i = 0; i < 256; i++) // for nn in nns
    {
      wordoff = i/64;
      bitoff = i%64;
      if (nns.bitset[wordoff] & (1ULL<<bitoff))
      {
        bitset_update(&defaults, &ns[i].defaults);
        //defaults.bitset[wordoff] |= (1ULL<<bitoff);
        for (j = 0; j < 256; j++) // for ch,nns2 in nn.d.items()
        {
          bitset_update(&d2[j], &ns[i].d[j]);
        }
        bitset_update(&d2epsilon, &ns[i].epsilon);
      }
    }
    epsilonclosure(ns, defaults, &defaultsec, &tainted, &acceptidset);
    if (!bitset_empty(&defaultsec))
    {
      dfanodeid = 255;
      for (i = 0; i < d.tblsz; i++)
      {
        if (bitset_equal(&d.tbl[i].key, &defaultsec))
        {
          dfanodeid = d.tbl[i].dfanodeid;
          break;
        }
      }
      if (dfanodeid == 255)
      {
        accepting = 0;
        for (i = 0; i < 256; i++)
        {
          wordoff = i/64;
          bitoff = i%64;
          if (defaultsec.bitset[wordoff] & (1ULL<<bitoff))
          {
            if (ns[i].accepting)
            {
              accepting = 1;
              break;
            }
          }
        }
        if (curdfanode >= 255)
        {
          abort();
        }
        d.tbl[d.tblsz].dfanodeid = curdfanode;
        memcpy(&d.tbl[d.tblsz++].key, &defaultsec, sizeof(defaultsec));
        dfa_init(&ds[curdfanode], accepting, tainted, &acceptidset);
        dfanodeid = curdfanode++;
        if (queuesz >= sizeof(queue)/sizeof(*queue))
        {
          abort();
        }
        queue[queuesz++] = defaultsec;
      }

      dfanodeid2 = 255;
      for (i = 0; i < d.tblsz; i++)
      {
        if (bitset_equal(&d.tbl[i].key, &nns))
        {
          dfanodeid2 = d.tbl[i].dfanodeid;
          break;
        }
      }
      if (dfanodeid2 == 255)
      {
        abort(); // FIXME fails
      }
      dfa_connect_default(&ds[dfanodeid2], dfanodeid);
      printf("Connected default\n");
    }
    for (i = 0; i < 256; i++)
    {
      struct bitset ec;
      if (bitset_empty(&d2[i]))
      {
        continue;
      }
      bitset_update(&d2[i], &defaults);
      epsilonclosure(ns, d2[i], &ec, &tainted, &acceptidset);

      dfanodeid = 255;
      for (j = 0; j < d.tblsz; j++)
      {
        if (bitset_equal(&d.tbl[j].key, &ec))
        {
          dfanodeid = d.tbl[j].dfanodeid;
          break;
        }
      }
      if (dfanodeid == 255)
      {
        accepting = 0;
        for (j = 0; j < 256; j++)
        {
          wordoff = j/64;
          bitoff = j%64;
          if (ec.bitset[wordoff] & (1ULL<<bitoff))
          {
            if (ns[j].accepting)
            {
              accepting = 1;
              break;
            }
          }
        }
        if (curdfanode >= 255)
        {
          abort();
        }
        d.tbl[d.tblsz].dfanodeid = curdfanode;
        memcpy(&d.tbl[d.tblsz++].key, &ec, sizeof(ec));
        dfa_init(&ds[curdfanode], accepting, tainted, &acceptidset);
        dfanodeid = curdfanode++;
        if (queuesz >= sizeof(queue)/sizeof(*queue))
        {
          abort();
        }
        queue[queuesz++] = ec;
      }

      dfanodeid2 = 255;
      for (j = 0; j < d.tblsz; j++)
      {
        if (bitset_equal(&d.tbl[j].key, &nns))
        {
          dfanodeid2 = d.tbl[j].dfanodeid;
          break;
        }
      }
      if (dfanodeid2 == 255)
      {
        abort();
      }
      dfa_connect(&ds[dfanodeid2], i, dfanodeid);
    }
  }
  return curdfanode;
}

int main(int argc, char **argv)
{
  struct nfa_node ns[11];
  struct dfa_node ds[255];
  size_t i;
  uint8_t dscnt;

  for (i = 0; i < 255; i++)
  {
    dfa_init_empty(&ds[i]);
  }

  nfa_init(&ns[0], 0, 255);
  nfa_init(&ns[1], 0, 0);
  nfa_init(&ns[2], 0, 1);
  nfa_init(&ns[3], 0, 2);
  nfa_init(&ns[4], 0, 0);
  nfa_init(&ns[5], 1, 1);
  nfa_init(&ns[6], 0, 2);
  nfa_init(&ns[7], 0, 0);
  nfa_init(&ns[8], 0, 2);
  nfa_init(&ns[9], 1, 0);
  nfa_init(&ns[10], 1, 2);

  nfa_connect(&ns[0], 'a', 1);
  nfa_connect(&ns[1], 'b', 4);
  nfa_connect(&ns[4], 'c', 7);
  nfa_connect(&ns[7], 'e', 9);

  nfa_connect(&ns[0], 'a', 2);
  nfa_connect(&ns[2], 'b', 5);

  nfa_connect(&ns[0], 'a', 3);
  nfa_connect(&ns[3], 'b', 6);
  nfa_connect(&ns[6], 'c', 8);
  nfa_connect(&ns[8], 'd', 10);

  dscnt = nfa2dfa(ns, ds, 0);
  printf("DFA state count %d\n", (int)dscnt);
  printf("\n\n\n\n");
  dfaviz(ds, dscnt);
}
