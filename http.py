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
foldstart_re = "[ \t]+"
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

nonterminals = [
  headerField,
  version,
  requestLine,
  requestHdrs,
  requestWithHeaders,
]

epsilon = -1
eof = -2

def isTerminal(x):
  return x >= 0 and x < num_terminals

S = requestWithHeaders

rules = [
  (headerField, [foldstart, httpfield]),
  (headerField, [hosttoken, colon, optspace, httpfield]),
  (headerField, [httptoken, colon, optspace, httpfield]),
  (version, [httpname, slash, digit, period, digit]),
  (requestLine, [httptoken, onespace, uri, onespace, version, crlf]),
  (requestHdrs, []),
  (requestHdrs, [headerField, crlf, requestHdrs]),
  (requestWithHeaders, [requestLine, requestHdrs, crlf]),
]

Fi = {}
for nonterminal in nonterminals:
  Fi[nonterminal] = set()

def firstset_func(rhs):
  global Fi
  if len(rhs) == 0:
    return set([epsilon])
  first=rhs[0]
  if isTerminal(first):
    return set([first])
  if epsilon not in Fi[first]:
    return Fi[first]
  else:
    return Fi[first].difference(set([epsilon])).union(firstset_func(rhs[1:]))
    

changed = True
while changed:
  changed = False
  for rule in rules:
    nonterminal = rule[0]
    rhs = rule[1]
    trhs = tuple(rhs)
    firstset = firstset_func(rhs)
    Fi.setdefault(trhs, set([]))
    if firstset.issubset(Fi[trhs]):
      continue
    Fi[trhs].update(firstset)
    changed = True
  for rule in rules:
    nonterminal = rule[0]
    rhs = rule[1]
    trhs = tuple(rhs)
    if Fi[trhs].issubset(Fi[nonterminal]):
      continue
    Fi[nonterminal].update(Fi[trhs])
    changed = True

Fo = {}
for nonterminal in nonterminals:
  Fo[nonterminal] = set()
Fo[S] = set([eof])

changed = True
while changed:
  changed = False
  for rule in rules:
    nonterminal = rule[0]
    rhs = rule[1]
    for idx in range(len(rhs)):
      rhsmid = rhs[idx]
      if isTerminal(rhsmid):
        continue
      rhsleft = rhs[:idx]
      rhsright = rhs[idx+1:]
      firstrhsright = firstset_func(rhsright)
      for terminal in terminals:
        if terminal in firstrhsright:
          if terminal not in Fo[rhsmid]:
            changed = True
            Fo[rhsmid].add(terminal)
      if epsilon in firstrhsright:
        if not Fo[nonterminal].issubset(Fo[rhsmid]):
          changed = True
          Fo[rhsmid].update(Fo[nonterminal])
      if len(rhsright) == 0:
        if not Fo[nonterminal].issubset(Fo[rhsmid]):
          changed = True
          Fo[rhsmid].update(Fo[nonterminal])

T = {}
for A in nonterminals:
  T[A] = {}
  for a in terminals:
    T[A][a] = set([])

for idx in range(len(rules)):
  rule = rules[idx]
  A = rule[0]
  w = rule[1]
  tw = tuple(w)
  for a in terminals:
    fi = Fi[tw]
    fo = Fo[A]
    if a in fi:
      T[A][a].add(idx)
    if epsilon in fi and a in fo:
      T[A][a].add(idx)

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
        rule = rules[Tt[A][a]]
        rhs = rule[1]
        newtuple = current[:-1] + tuple(reversed(rhs))
        if newtuple not in visiteds:
          visitqueue.append(newtuple)
  return maxvisited

max_stack_size = get_max_sz_dfs(Tt, rules, terminals, S)

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
    action = Tt[stack[-1]][sym]
    if action == None:
      raise Exception("parse error")
    rule = rules[action]
    rhs = rule[1]
    stack.pop()
    for val in reversed(rhs):
      stack.append(val)

myreq = [
  httptoken, onespace, uri, onespace,
    httpname, slash, digit, period, digit, crlf,
  httptoken, colon, optspace, httpfield, crlf,
  hosttoken, colon, optspace, httpfield, crlf,
  httptoken, colon, optspace, httpfield, crlf,
    foldstart, httpfield, crlf,
    foldstart, httpfield, crlf,
  httptoken, colon, optspace, httpfield, crlf,
    foldstart, httpfield, crlf,
  httptoken, colon, optspace, httpfield, crlf,
  crlf]
parse(myreq)

myreqshort = [
  httptoken, onespace, uri, onespace,
    httpname, slash, digit, period, digit, crlf,
  httptoken, colon, optspace, httpfield, crlf,
  hosttoken, colon, optspace, httpfield, crlf,
  httptoken, colon, optspace, httpfield, crlf,
    foldstart, httpfield, crlf,
    foldstart, httpfield, crlf,
  httptoken, colon, optspace, httpfield, crlf,
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
  httptoken, colon, optspace, httpfield, crlf,
    foldstart, httpfield, crlf,
    foldstart, httpfield, crlf,
  httptoken, colon, optspace, httpfield, crlf,
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
regex.dump_all(re_by_idx, list_of_reidx_sets, priorities)

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

  for (i = 0; i < 1000*1000; i++)
  {
    init_statemachine(&ctx);
    consumed = feed_statemachine(&ctx, states_8, input, strlen(input), &state);
    if (consumed != 999 || state != 8)
    {
      abort();
    }
    //printf("Consumed %zd state %d\\n", consumed, (int)state);
  }

  init_statemachine(&ctx);
  consumed = feed_statemachine(&ctx, states_0_1_8_12, input, strlen(input), &state);
  printf("Consumed %zd state %d\\n", consumed, (int)state);

  return 0;
}
"""
