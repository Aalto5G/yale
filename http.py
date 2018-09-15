hosttoken = 0
crlf = 1
onespace = 2
httpname = 3
slash = 4
digit = 5
colon = 6
optspace = 7
httptoken = 8
httpfield = 9
period = 10
uri = 11
foldstart = 12

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

headerField = 100
version = 101
requestLine = 102
requestHdrs = 103
requestWithHeaders = 104

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
  return x >= 0 and x < 100

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
      print "Conflict %d %d" % (A,a,)
      raise Exception("error")
    elif len(T[A][a]) == 1:
      Tt[A][a] = set(T[A][a]).pop()
      print "Non-conflict %d %d: %s" % (A,a,Tt[A][a])

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
      print "Removing terminal %d" % (sym,)
      if stack[-1] != sym:
        raise Exception("parse error")
      revreq.pop()
      stack.pop()
      continue
    print "Getting action %d %d" % (stack[-1], sym,)
    action = Tt[stack[-1]][sym]
    if action == None:
      raise Exception("parse error")
    rule = rules[action]
    rhs = rule[1]
    stack.pop()
    for val in reversed(rhs):
      stack.append(val)

max_stacks = {}
maxrecurse = 16384
changed = True
for a in terminals:
  max_stacks[a] = 1
while changed:
  changed = False
  for rule in rules:
    lhs = rule[0]
    rhs = list(rule[1])
    cursz = len(rhs)
    while len(rhs) > 0:
      tmpsz = len(rhs) - 1 + max_stacks.setdefault(rhs[0], 0)
      del rhs[0]
      if tmpsz > cursz:
        cursz = tmpsz
    if cursz > maxrecurse:
      raise Exception("language seems infinitely recursive")
    if lhs not in max_stacks or max_stacks[lhs] < cursz:
      changed = True
      max_stacks[lhs] = cursz
max_stack_size = 1 + max_stacks[requestWithHeaders]

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
