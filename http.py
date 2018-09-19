import regex

#hosttoken = 0
#crlf = 1
#onespace = 2
#httpname = 3
#slash = 4
#digit = 5
#colon = 6
#optspace = 7
#httptoken = 8
#httpfield = 9
#period = 10
#uri = 11
#foldstart = 12

re_by_idx = []
priorities = []

hosttoken = 0
hosttoken_re = "[Hh][Oo][Ss][Tt]"
re_by_idx.append(hosttoken_re)
priorities.append(1)
crlf = 1
crlf_re = "\r\n"
re_by_idx.append(crlf_re)
priorities.append(0)
onespace = 2
onespace_re = " "
re_by_idx.append(onespace_re)
priorities.append(0)
httpname = 3
httpname_re = "HTTP"
re_by_idx.append(httpname_re)
priorities.append(0)
slash = 4
slash_re = "/"
re_by_idx.append(slash_re)
priorities.append(0)
digit = 5
digit_re = "[0-9]"
re_by_idx.append(digit_re)
priorities.append(0)
colon = 6
colon_re = ":"
re_by_idx.append(colon_re)
priorities.append(0)
optspace = 7
optspace_re = "[ \t]*"
re_by_idx.append(optspace_re)
priorities.append(0)
httptoken = 8
httptoken_re = "[-!#$%&'*+.^_`|~0-9A-Za-z]+"
re_by_idx.append(httptoken_re)
priorities.append(0)
httpfield = 9
httpfield_re = "[\t\x20-\x7E\x80-\xFF]*"
re_by_idx.append(httpfield_re)
priorities.append(0)
period = 10
period_re = "."
re_by_idx.append(period_re)
priorities.append(0)
uri = 11
uri_re = "[]:/?#@!$&'()*+,;=0-9A-Za-z._~%[-]+"
re_by_idx.append(uri_re)
priorities.append(0)
foldstart = 12
foldstart_re = "\r\n[ \t]+"
re_by_idx.append(foldstart_re)
priorities.append(0)
num_terminals = 13

terminals = [
  hosttoken,
  crlf,
  onespace,
  httpname,
  slash,
  digit,
  colon,
  optspace,
  httptoken,
  httpfield,
  period,
  uri,
  foldstart,
]

headerField = num_terminals + 0
version = num_terminals + 1
requestLine = num_terminals + 2
requestHdrs = num_terminals + 3
requestWithHeaders = num_terminals + 4
httpFoldField = num_terminals + 5
httpFoldFieldEnd = num_terminals + 6
hostFoldField = num_terminals + 7
hostFoldFieldEnd = num_terminals + 8

nonterminals = [
  headerField,
  version,
  requestLine,
  requestHdrs,
  requestWithHeaders,
  httpFoldField,
  httpFoldFieldEnd,
  hostFoldField,
  hostFoldFieldEnd,
]

epsilon = -1
eof = -2

S = requestWithHeaders

class WrapCB(object):
  def __init__(self, token, cbname):
    assert isTerminal(token)
    self.token = token
    self.cbname = cbname

def isTerminal(x):
  if type(x) == WrapCB:
    return True
  return x >= 0 and x < num_terminals

def unwrap(x):
  if type(x) == list:
    return list(unwrap(y) for y in x)
  if type(x) == WrapCB:
    return x.token
  if type(x) == int:
    return x
  assert False



rules = [
  (hostFoldField, [WrapCB(httpfield, "print"), hostFoldFieldEnd]),
  (hostFoldFieldEnd, []),
  (hostFoldFieldEnd, [WrapCB(foldstart, "print"),
                      WrapCB(httpfield, "print"),
                      hostFoldFieldEnd]),
  (httpFoldField, [httpfield, httpFoldFieldEnd]),
  (httpFoldFieldEnd, []),
  (httpFoldFieldEnd, [foldstart, httpfield, httpFoldFieldEnd]),
  (headerField, [hosttoken, colon, optspace, hostFoldField]),
  (headerField, [httptoken, colon, optspace, httpFoldField]),
  (version, [httpname, slash, digit, period, digit]),
  (requestLine, [httptoken, onespace, uri, onespace, version, crlf]),
  (requestHdrs, []),
  (requestHdrs, [headerField, crlf, requestHdrs]),
  (requestWithHeaders, [requestLine, requestHdrs, crlf]),
]

Fi = {}
for nonterminal in nonterminals:
  Fi[nonterminal] = {}#set()

def firstset_update(a,b):
  for k,v in b.items():
    if k in a:
      a[k].update(v)
    else:
      a[k] = set([])
      a[k].update(v)

def firstset_singleton(x):
  if type(x) == WrapCB:
    return {x.token: set([x.cbname])}
  if type(x) == int:
    return {x: set()}
  assert False

def firstset_func(rhs):
  global Fi
  if len(rhs) == 0:
    return {epsilon: set()}
    #return set([epsilon])
  first=rhs[0]
  if isTerminal(first):
    return firstset_singleton(first)
    #return {first: set()}
    #return set([first])
  if epsilon not in Fi[first]:
    return Fi[first]
  else:
    #result = Fi[first][:]
    result = dict(Fi[first])
    del result[epsilon]
    firstset_update(result, firstset_func(rhs[1:]))
    #result.update(firstset_func(rhs[1:])) # FIXME
    return result
    #return Fi[first].difference(set([epsilon])).union(firstset_func(rhs[1:]))

def firstset_issubset(a,b):
  for k in a.keys():
    if k not in b:
      return False
  for k,v in a.items():
    if not v.issubset(b[k]):
      return False
  return True

changed = True
while changed:
  changed = False
  for rule in rules:
    nonterminal = rule[0]
    origrhs = rule[1]
    rhs = unwrap(origrhs)
    trhs = tuple(rhs)
    firstset = firstset_func(origrhs)
    Fi.setdefault(trhs, {})
    #if firstset.issubset(Fi[trhs]):
    if firstset_issubset(firstset, Fi[trhs]):
      continue
    firstset_update(Fi[trhs], firstset)
    #Fi[trhs].update(firstset)
    changed = True
  for rule in rules:
    nonterminal = rule[0]
    rhs = unwrap(rule[1])
    trhs = tuple(rhs)
    #if Fi[trhs].issubset(Fi[nonterminal]):
    if firstset_issubset(Fi[trhs], Fi[nonterminal]):
      continue
    firstset_update(Fi[nonterminal], Fi[trhs])
    #Fi[nonterminal].update(Fi[trhs])
    changed = True

Fo = {}
for nonterminal in nonterminals:
  Fo[nonterminal] = {}#set()
#Fo[S] = set([eof])
Fo[S] = {eof: set()}

changed = True
while changed:
  changed = False
  for rule in rules:
    nonterminal = rule[0]
    origrhs = rule[1]
    rhs = unwrap(origrhs)
    for idx in range(len(rhs)):
      origrhsmid = origrhs[idx]
      rhsmid = rhs[idx]
      if isTerminal(rhsmid):
        continue
      origrhsleft = origrhs[:idx]
      rhsleft = rhs[:idx]
      origrhsright = origrhs[idx+1:]
      rhsright = rhs[idx+1:]
      firstrhsright = firstset_func(origrhsright)
      for terminal in terminals:
        if terminal in firstrhsright:
          if terminal not in Fo[rhsmid]:
            changed = True
            firstset_update(Fo[rhsmid], {terminal: firstrhsright[terminal]})
            #Fo[rhsmid].add(terminal)
      if epsilon in firstrhsright:
        #if not Fo[nonterminal].issubset(Fo[rhsmid]):
        if not firstset_issubset(Fo[nonterminal],Fo[rhsmid]):
          changed = True
          #Fo[rhsmid].update(Fo[nonterminal])
          firstset_update(Fo[rhsmid], Fo[nonterminal])
      if len(rhsright) == 0:
        #if not Fo[nonterminal].issubset(Fo[rhsmid]):
        if not firstset_issubset(Fo[nonterminal],Fo[rhsmid]):
          changed = True
          #Fo[rhsmid].update(Fo[nonterminal])
          firstset_update(Fo[rhsmid], Fo[nonterminal])

T = {}
for A in nonterminals:
  T[A] = {}
  for a in terminals:
    T[A][a] = set([])

for idx in range(len(rules)):
  rule = rules[idx]
  A = rule[0]
  w = unwrap(rule[1])
  tw = tuple(w)
  for a in terminals:
    fi = Fi[tw]
    fo = Fo[A]
    if a in fi:
      if len(fi[a]) == 0:
        T[A][a].add((idx, None))
      for cb in fi[a]:
        T[A][a].add((idx, cb))
    if epsilon in fi and a in fo:
      if len(fo[a]) == 0:
        T[A][a].add((idx, None))
      for cb in fo[a]:
        T[A][a].add((idx, cb))

Tt = {}
for A in nonterminals:
  Tt[A] = {}
  for a in terminals:
    Tt[A][a] = None

for A in nonterminals:
  for a in terminals:
    if len(T[A][a]) > 1:
      raise Exception("Conflict %d %d" % (A,a,))
    elif len(T[A][a]) == 1:
      Tt[A][a] = set(T[A][a]).pop()
      #print "Non-conflict %d %d: %s" % (A,a,Tt[A][a])

def get_max_sz_dfs(Tt, rules, terminals, S, eof=-1, maxrecurse=16384):
  visiteds = set()
  maxvisited = 0
  # in case somebody uses without EOF symbol:
  initial = (eof is None) and (S,) or (eof, S)
  visitqueue = []
  visitqueue.append(initial)
  while visitqueue:
    current = visitqueue.pop()
    visiteds.add(current)
    if len(current) > maxrecurse:
      raise Exception("language seems infinitely recursive")
    if len(current) > maxvisited:
      maxvisited = len(current)
    last = current[-1]
    if last in terminals:
      if current[:-1] not in visiteds:
        visitqueue.append(current[:-1])
      continue
    if last == eof:
      continue
    A = last
    for a in terminals:
      if Tt[A][a] != None:
        rule = rules[Tt[A][a][0]]
        rhs = unwrap(rule[1])
        newtuple = current[:-1] + tuple(reversed(rhs))
        if newtuple not in visiteds:
          visitqueue.append(newtuple)
  return maxvisited

#print Tt
max_stack_size = get_max_sz_dfs(Tt, rules, terminals, S)
if max_stack_size > 255:
  assert False

def parse(req):
  revreq = list(reversed(req))
  stack = [eof, S]
  while True:
    if len(stack) > max_stack_size:
      raise Exception("stack overflow")
    if stack[-1] == eof and len(revreq) == 0:
      break
    if stack[-1] == eof or len(revreq) == 0:
      raise Exception("parse error")
    sym = revreq[-1]
    if isTerminal(stack[-1]):
      #print "Removing terminal %d" % (sym,)
      if stack[-1] != sym:
        raise Exception("parse error")
      revreq.pop()
      stack.pop()
      continue
    #print "Getting action %d %d" % (stack[-1], sym,)
    action = Tt[stack[-1]][sym][0]
    if action == None:
      raise Exception("parse error")
    rule = rules[action]
    rhs = unwrap(rule[1])
    stack.pop()
    for val in reversed(rhs):
      stack.append(val)

myreq = [
  httptoken, onespace, uri, onespace,
    httpname, slash, digit, period, digit, crlf,
  httptoken, colon, optspace, httpfield, crlf,
  hosttoken, colon, optspace, httpfield, crlf,
  httptoken, colon, optspace, httpfield,# crlf,
    foldstart, httpfield, #crlf,
    foldstart, httpfield, crlf,
  httptoken, colon, optspace, httpfield, #crlf,
    foldstart, httpfield, crlf,
  httptoken, colon, optspace, httpfield, crlf,
  crlf]
parse(myreq)

myreqshort = [
  httptoken, onespace, uri, onespace,
    httpname, slash, digit, period, digit, crlf,
  httptoken, colon, optspace, httpfield, crlf,
  hosttoken, colon, optspace, httpfield, crlf,
  httptoken, colon, optspace, httpfield, #crlf,
    foldstart, httpfield, #crlf,
    foldstart, httpfield, crlf,
  httptoken, colon, optspace, httpfield, #crlf,
    foldstart, httpfield, crlf,
  httptoken, colon, optspace, httpfield, crlf]
ok = False
try:
  parse(myreqshort)
except:
  ok = True
assert ok

myreqlong = [
  httptoken, onespace, uri, onespace,
    httpname, slash, digit, period, digit, crlf,
  httptoken, colon, optspace, httpfield, crlf,
  hosttoken, colon, optspace, httpfield, crlf,
  httptoken, colon, optspace, httpfield, #crlf,
    foldstart, httpfield, #crlf,
    foldstart, httpfield, crlf,
  httptoken, colon, optspace, httpfield, #crlf,
    foldstart, httpfield, crlf,
  httptoken, colon, optspace, httpfield, crlf,
  crlf,
  crlf]
ok = False
try:
  parse(myreqlong)
except:
  ok = True
assert ok

myreqtrivial = [
  httptoken, onespace, uri, onespace,
    httpname, slash, digit, period, digit, crlf,
  crlf]
parse(myreqtrivial)

# Strategy for tokenizer:
# uint64_t[] terminal index list for every nonterminal
# uint64_t[] terminal index list for every DFA state
# if bitwise AND is nonzero, continue

list_of_reidx_sets = set()
# Single token DFAs
list_of_reidx_sets.update([frozenset([x]) for x in range(num_terminals)])

for X in nonterminals:
  list_of_reidx_sets.add(frozenset([x for x in terminals if T[X][x]]))

regex.dump_headers(re_by_idx, list_of_reidx_sets)
print """
static inline void myPutchar(char ch)
{
  if (ch >= 0x20 && ch <= 0x7E)
  {
    putchar(ch);
  }
  else
  {
    printf("\\\\x%.2x", (unsigned)(uint8_t)ch);
  }
}
static void print(const char *buf, size_t siz, void *btn)
{
  const char *ubuf = buf;
  size_t i;
  putchar('<');
  for (i = 0; i < siz; i++)
  {
    myPutchar(ubuf[i]);
  }
  putchar('>');
  putchar('\\n');
}
"""
regex.dump_all(re_by_idx, list_of_reidx_sets, priorities)

print "const uint8_t num_terminals;"

print """
struct ruleentry {
  uint8_t rhs;
  void (*cb)(const char *, size_t, void*);
};

struct parserctx {
  uint8_t stacksz;
  struct ruleentry stack[%d]; // WAS: uint8_t stack[...];
  struct rectx rctx;
  uint8_t saved_token;
};

static inline void parserctx_init(struct parserctx *pctx)
{
  pctx->saved_token = 255;
  pctx->stacksz = 1;
  pctx->stack[0].rhs = %d;
  pctx->stack[0].cb = NULL;
  init_statemachine(&pctx->rctx);
}
""" % (max_stack_size, S, )

print """
struct rule {
  uint8_t lhs;
  uint8_t rhssz;
  //const uint8_t *rhs;
  const struct ruleentry *rhs;
};
"""
print """
struct parserstatetblentry {
  const struct state *re;
  const uint8_t rhs[%d];
  const void (*cb[%d])(const char *, size_t, void*);
};
""" % (len(terminals),len(terminals),)

print """
struct reentry {
  const struct state *re;
};
"""

print "const uint8_t num_terminals = %d;" % (len(terminals),)
print "const uint8_t start_state = %d;" % (S,)

print "const struct reentry reentries[] = {"

for x in sorted(terminals):
  print "{"
  name = str(x)
  print ".re = states_" + name + ","
  print "},"

print "};"

print "const struct parserstatetblentry parserstatetblentries[] = {"

for X in sorted(nonterminals):
  print "{"
  name = '_'.join(str(x) for x in sorted([x for x in terminals if T[X][x]]))
  print ".re = states_" + name + ","
  print ".rhs = {",
  for x in sorted(terminals):
    if Tt[X][x] == None:
      print 255,",",
    else:
      print Tt[X][x][0],",",
  print "},"
  print ".cb = {",
  for x in sorted(terminals):
    if Tt[X][x] == None or Tt[X][x][1] == None:
      print "NULL,",
    else:
      print Tt[X][x][1],",",
  print "},"
  print "},"

print "};"

for n in range(len(rules)):
  lhs,rhs = rules[n]
  print "const struct ruleentry rule_%d[] = {" % (n,)
  for rhsitem in reversed(rhs):
    print "{",
    if type(rhsitem) == WrapCB:
      print ".rhs =", rhsitem.token, ",", ".cb = ", rhsitem.cbname,
    else:
      print ".rhs =", rhsitem, ",", ".cb = NULL",
    print "}",",",
  print
  print "};"
  print "const uint8_t rule_%d_len = sizeof(rule_%d)/sizeof(struct ruleentry);" % (n,n,)
print "const struct rule rules[] = {"
for n in range(len(rules)):
  lhs,rhs = rules[n]
  print "{"
  print "  .lhs =", lhs, ","
  print "  .rhssz = sizeof(rule_%d)/sizeof(struct ruleentry)," % (n,)
  print "  .rhs = rule_%d," % (n,)
  print "},"
print "};"

print """

static inline ssize_t
get_saved_token(struct parserctx *pctx, const struct state *restates,
                char *blkoff, size_t szoff, uint8_t *state,
                void (*const*cbs)(const char *, size_t, void*),
                void (*cb1)(const char *, size_t, void*))
{
  if (pctx->saved_token != 255)
  {
    *state = pctx->saved_token;
    pctx->saved_token = 255;
    return 0;
  }
  return feed_statemachine(&pctx->rctx, restates, blkoff, szoff, state, cbs, cb1, NULL);
}

#undef EXTRA_SANITY

static __attribute__((unused)) ssize_t
parse_block(struct parserctx *pctx, char *blk, size_t sz)
{
  size_t off = 0;
  ssize_t ret;
  uint8_t curstate;
  while (off < sz || pctx->saved_token != 255)
  {
    if (pctx->stacksz == 0)
    {
      if (off >= sz && pctx->saved_token == 255)
      {
#ifdef EXTRA_SANITY
        if (off > sz)
        {
          abort();
        }
#endif
        return sz; // EOF
      }
      else
      {
#ifdef EXTRA_SANITY
        if (off > sz)
        {
          abort();
        }
#endif
        return -EBADMSG;
      }
    }
    curstate = pctx->stack[pctx->stacksz - 1].rhs;
    if (curstate < num_terminals)
    {
      uint8_t state;
      const struct state *restates = reentries[curstate].re;
      const void (*cb1)(const char *, size_t, void*);
      cb1 = pctx->stack[pctx->stacksz - 1].cb;
      ret = get_saved_token(pctx, restates, blk+off, sz-off, &state, NULL, cb1);
      if (ret == -EAGAIN)
      {
        off = sz;
        return -EAGAIN;
      }
      else if (ret < 0)
      {
        fprintf(stderr, "Parser error: tokenizer error, curstate=%d\\n", curstate);
        fprintf(stderr, "blk[off] = '%c'\\n", blk[off]);
        abort();
        exit(1);
      }
      else
      {
        off += ret;
#if 0
        if (off > sz)
        {
          abort();
        }
#endif
      }
      if (curstate != state)
      {
        fprintf(stderr, "Parser error: state mismatch %d %d\\n",
                        (int)curstate, (int)state);
        exit(1);
      }
      //printf("Got expected token %d\\n", (int)state);
      pctx->stacksz--;
    }
    else
    {
      uint8_t state;
      uint8_t curstateoff = curstate - num_terminals;
      uint8_t ruleid;
      size_t i;
      const struct rule *rule;
      const struct state *restates = parserstatetblentries[curstateoff].re;
      const void (*const*cbs)(const char *, size_t, void*);
      cbs = parserstatetblentries[curstateoff].cb;
      ret = get_saved_token(pctx, restates, blk+off, sz-off, &state, cbs, NULL);
      if (ret == -EAGAIN)
      {
        off = sz;
        return -EAGAIN;
      }
      else if (ret < 0 || state == 255)
      {
        fprintf(stderr, "Parser error: tokenizer error, curstate=%d, token=%d\\n", (int)curstate, (int)state);
        exit(1);
      }
      else
      {
        off += ret;
#if 0
        if (off > sz)
        {
          abort();
        }
#endif
      }
      //printf("Got token %d, curstate=%d\\n", (int)state, (int)curstate);
      ruleid = parserstatetblentries[curstateoff].rhs[state];
      rule = &rules[ruleid];
      pctx->stacksz--;
#if 0
      if (rule->lhs != curstate)
      {
        abort();
      }
#endif
      if (pctx->stacksz + rule->rhssz > sizeof(pctx->stack)/sizeof(struct ruleentry))
      {
        abort();
      }
      i = 0;
      while (i + 4 <= rule->rhssz)
      {
        pctx->stack[pctx->stacksz++] = rule->rhs[i+0];
        pctx->stack[pctx->stacksz++] = rule->rhs[i+1];
        pctx->stack[pctx->stacksz++] = rule->rhs[i+2];
        pctx->stack[pctx->stacksz++] = rule->rhs[i+3];
        i += 4;
      }
      for (; i < rule->rhssz; i++)
      {
        pctx->stack[pctx->stacksz++] = rule->rhs[i];
      }
      pctx->saved_token = state;
    }
  }
  if (pctx->stacksz == 0)
  {
    if (off >= sz && pctx->saved_token == 255)
    {
#ifdef EXTRA_SANITY
      if (off > sz)
      {
        abort();
      }
#endif
      return sz; // EOF
    }
  }
#ifdef EXTRA_SANITY
  if (off > sz)
  {
    abort();
  }
#endif
  return -EAGAIN;
}
"""

print """
int main(int argc, char **argv)
{
  char *input = "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ" // 500
                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ" 
                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHI "; // 1000

  ssize_t consumed;
  uint8_t state = 255;
  struct rectx ctx = {};
  size_t i;
  struct parserctx pctx = {};
  char *http = "GET / HTTP/1.1\\r\\nHost: localhost\\r\\n\\r\\n";
  char *httpa = "GET / HTTP/1.1\\r\\nHost: localhost\\r\\n\\r\\na";


  for (i = 0; i < /* 1000* */ 1000 * 0; i++)
  {
    init_statemachine(&ctx);
    consumed = feed_statemachine(&ctx, states_8, input, strlen(input), &state);
    if (consumed != 999 || state != 8)
    {
      abort();
    }
    //printf("Consumed %zd state %d\\n", consumed, (int)state);
  }

  parserctx_init(&pctx);
  consumed = parse_block(&pctx, http, strlen(http));
  printf("Consumed %zd stack %d\\n", consumed, (int)pctx.stacksz);

  parserctx_init(&pctx);
  consumed = parse_block(&pctx, httpa, strlen(httpa));
  printf("Consumed %zd stack %d\\n", consumed, (int)pctx.stacksz);

  return 0;
}
"""[:0]

print """
int main(int argc, char **argv)
{
  ssize_t consumed;
  uint8_t state = 255;
  size_t i;
  struct parserctx pctx = {};
  char http[] =
    "GET /foo/bar/baz/barf/quux.html HTTP/1.1\\r\\n"
    "Host: www.google.fi\\r\\n"
    "\twww.google2.fi\\r\\n"
    "User-Agent: Mozilla/5.0 (Linux; Android 7.0; SM-G930VC Build/NRD90M; wv) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/58.0.3029.83 Mobile Safari/537.36\\r\\n"
    "Accept: text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,image/jpeg,image/gif;q=0.2,*/*;q=0.1\\r\\n"
    "Accept-Language: en-us,en;q=0.5\\r\\n"
    "Accept-Encoding: gzip,deflate\\r\\n"
    "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\\r\\n"
    "Keep-Alive: 300\\r\\n"
    "Connection: keep-alive\\r\\n"
    "Referer: http://www.google.fi/quux/barf/baz/bar/foo.html\\r\\n"
    "Cookie: PHPSESSID=298zf09hf012fh2; csrftoken=u32t4o3tb3gg43; _gat=1;\\r\\n"
    "\\r\\n";

  for (i = 0; i < /*1000 * 1000*/ 1; i++)
  {
    parserctx_init(&pctx);
    consumed = parse_block(&pctx, http, sizeof(http)-1);
    if (consumed != sizeof(http)-1)
    {
      abort();
    }
  }

  return 0;
}
"""
