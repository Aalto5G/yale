# -*- coding: iso-8859-15 -*-
from __future__ import division
import collections
import random
import re
import cStringIO
import time


class dfanode(object):
  def __init__(self,accepting=False,tainted=False,acceptidset=frozenset([])):
    self.d = {}
    self.accepting = accepting
    self.default = None
    self.tainted = tainted
    self.acceptidset = acceptidset
  def connect(self,ch,node):
    assert ch not in self.d
    self.d[ch] = node
  def connect_default(self,node):
    assert self.default == None
    self.default = node
  def execute(self,str):
    if not str:
      return self.accepting
    elif str[0] in self.d:
      return self.d[str[0]].execute(str[1:])
    elif self.default:
      return self.default.execute(str[1:])
    else:
      return False

tainted = object()

class nfanode(object):
  def __init__(self,accepting=False,taintid=None):
    self.d = {}
    self.accepting = accepting
    self.defaults = set()
    self.taintid = taintid
  def connect(self,ch,node):
    self.d.setdefault(ch,set()).add(node)
  def connect_default(self,node):
    self.defaults.add(node)

def fsmviz(begin,deterministic=False):
  d = {}
  q = collections.deque()
  result = cStringIO.StringIO()
  result.write("digraph fsm {\n")
  set_ids(begin)
  def add_node(node2):
    if not node2 in d:
      n = node2.id
      tainted = False
      if deterministic:
        tainted = node2.tainted
      acceptid = ""
      if "acceptid" in node2.__dict__:
        acceptid = "{%d}" % (node2.acceptid,)
      result.write("n%d [label=\"%d%s%s%s\"];\n" % (n,n,(node2.accepting and "+" or ""),(node2.tainted and "*" or ""),acceptid))
      d[node2] = n
      q.append(node2)
  add_node(begin)
  while q:
    node = q.popleft()
    for ch,nodes in node.d.items():
      if deterministic:
        nodes = [nodes]
      for node2 in nodes:
        add_node(node2)
        if ch:
          result.write("n%d -> n%d [label=\"%s\"];\n" % (d[node],d[node2],repr(repr(ch)[1:-1])[1:-1]))
        else:
          result.write("n%d -> n%d [label=\"%s\", fontname=Symbol];\n" % (d[node],d[node2],"e"))
    if deterministic:
      defaults = node.default and [node.default] or []
    else:
      defaults = node.defaults
    for node2 in defaults:
      add_node(node2)
      result.write("n%d -> n%d [label=\"%s\"];\n" % (d[node],d[node2],""))
  result.write("}\n")
  result.seek(0)
  return result.read()

def epsilonclosure(nodes):
  closure = set(nodes)
  q = collections.deque(nodes)
  while q:
    n = q.popleft()
    for n2 in n.d.get("",set()):
      if n2 not in closure:
        closure.add(n2)
        q.append(n2)
  taintidset = set()
  acceptidset = set()
  for item in closure:
    if item.taintid != None:
      taintidset.add(item.taintid)
      if item.accepting:
        acceptidset.add(item.taintid)
  return (frozenset(closure), len(taintidset)>1, frozenset(acceptidset))

def nfa2dfa(begin):
  dfabegin,tainted,acceptidset = epsilonclosure(set([begin]))
  d = {}
  d[dfabegin] = dfanode(True in (x.accepting for x in dfabegin), tainted=tainted, acceptidset=acceptidset)
  q = collections.deque([dfabegin])
  while q:
    nns = q.popleft()
    d2 = {}
    defaults = set()
    # lisätään kaikki tilasiirrot d2:een ja defaultsiin
    for nn in nns:
      defaults.update(nn.defaults)
      for ch,nns2 in nn.d.items():
        d2.setdefault(ch,set()).update(nns2)
    defaultsec,tainted,acceptidset = epsilonclosure(defaults)
    if defaultsec:
      if defaultsec not in d:
        d[defaultsec] = dfanode(True in (x.accepting for x in defaultsec), tainted=tainted, acceptidset=acceptidset)
        q.append(defaultsec)
      d[nns].connect_default(d[defaultsec])
    for ch,nns2 in d2.items():
      if ch:
        nns2.update(defaults)
        ec,tainted,acceptidset = epsilonclosure(nns2) # XXX: hidas!
        if ec not in d:
          d[ec] = dfanode(True in (x.accepting for x in ec), tainted=tainted, acceptidset=acceptidset)
          q.append(ec)
        d[nns].connect(ch,d[ec])
  return d[dfabegin]

class regexp(object):
  def nfa(self, taintid=None):
    begin,end = nfanode(),nfanode(True)
    self.gennfa(begin,end,taintid=taintid)
    return begin

class wildcard(regexp):
  def gennfa(self,begin,end,taintid=None):
    begin.connect_default(end)

class emptystr(regexp):
  def gennfa(self,begin,end,taintid=None):
    begin.connect("",end)

class literals(regexp):
  def __init__(self,s):
    self.s = s
  def gennfa(self,begin,end,taintid=None):
    for ch in self.s:
      begin.connect(ch,end)

class concat(regexp):
  def __init__(self,re1,re2):
    self.re1 = re1
    self.re2 = re2
  def gennfa(self,begin,end,taintid=None):
    middle = nfanode(taintid=taintid)
    self.re1.gennfa(begin,middle,taintid=taintid)
    self.re2.gennfa(middle,end,taintid=taintid)

class altern(regexp):
  def __init__(self,re1,re2):
    self.re1 = re1
    self.re2 = re2
  def gennfa(self,begin,end, taintid=None):
    self.re1.gennfa(begin,end,taintid=taintid)
    self.re2.gennfa(begin,end,taintid=taintid)

class alternmulti(regexp):
  def __init__(self,*res):
    self.res = res
  def nfa(self):
    begin = nfanode()
    taintid = 0
    for re in self.res:
      end = nfanode(True, taintid = taintid)
      taintid += 1
      re.gennfa(begin, end, taintid=taintid)
    return begin

class star(regexp):
  def __init__(self,re):
    self.re = re
  def gennfa(self,begin,end, taintid=None):
    begin1,end1 = nfanode(taintid=taintid),nfanode(taintid=taintid)
    self.re.gennfa(begin1,end1,taintid=taintid)
    begin.connect("",begin1)
    begin.connect("",end)
    end1.connect("",begin1)
    end1.connect("",end)

def parse_re(s):
  branch,remainder = parse_branch(s)
  if remainder and remainder[0] == "|":
    re,remainder2 = parse_re(remainder[1:])
    return altern(branch,re),remainder2
  else:
    return branch,remainder

def parse_branch(s):
  piece,remainder = parse_piece(s)
  if remainder and remainder[0] not in ")|":
    branch,remainder2 = parse_branch(remainder)
    return concat(piece,branch),remainder2
  else:
    return piece,remainder

def parse_piece(s):
  atom,remainder = parse_atom(s)
  if remainder:
    if remainder[0] == "*":
      return star(atom),remainder[1:]
    elif remainder[0] == "+":
      return concat(atom,star(atom)),remainder[1:]
    elif remainder[0] == "?":
      return altern(emptystr(),atom),remainder[1:]
    # TODO: bound
  return atom,remainder

def parse_atom(s):
  if not s or s[0] in ")|":
    return emptystr(),s
  elif s[0] == '[':
    return parse_bracketexpr(s[1:])
  elif s[0] == '.':
    return wildcard(),s[1:]
  elif s[0] == "(":
    re,remainder = parse_re(s[1:])
    assert remainder[0] == ")"
    return re,remainder[1:]
  else:
    return literals(s[0]),s[1:]

def parse_bracketexpr(s,recursive=False):
  assert s
  # TODO: -, ^
  l = s[1:].split("]",1)
  assert len(l) == 2
  return literals(s[0]+l[0]),l[1]


def re_compile(s):
  re,remainder = parse_re(s)
  assert remainder == ""
  #return re
  #return re.nfa()
  return re

def re_compilemulti(*ss):
  res = []
  for s in ss:
    res.append(re_compile(s))
  return alternmulti(*res)


def test_re(expr,n):
  dfa = nfa2dfa(re_compile(expr).nfa())
  compiled = re.compile("^(%s)$"%expr)
  for dummy in range(n):
    l = random.randrange(10)
    s = ''.join("abcdef"[random.randrange(6)] for dummy2 in range(l))
    if dfa.execute(s) != bool(compiled.match(s)):
      print expr, s, dfa.execute(s), bool(compiled.match(s))

#for n in range(100):
#  re_compile("(a|b)*|(a|f)c*(d|e)")
#  re_compile("(a|b)+|e?(a|f)c*(d|e).")
#  #re.compile("^((a|b)*|(a|f)c*(d|e))$")
#  #re.compile("^((a|b)+|e?(a|f)c*(d|e).)$")

#test_re("[ab]*|[af]c*[de]",50000)
#test_re("(a|b)+|e?(a|f)c*(d|e).",50000)


def state_backtrack(state):
  tovisit = [(state, 0)]
  visited = set([])
  max_backtrack = 0
  assert state.accepting
  while tovisit:
    queued,bt = tovisit.pop()
    #if queued in visited: # XXX harmful?
    #  continue
    if bt > max_backtrack:
      max_backtrack = bt
    visited.add(queued)
    for ch,node in queued.d.items():
      if node.accepting:
        continue
      if node not in visited:
        tovisit.append((node, bt+1))
      else:
        return -1
    if queued.default != None:
      if not queued.default.accepting:
        if queued.default not in visited:
          tovisit.append((queued.default, bt+1))
        else:
          return -1
  return max_backtrack

def maximal_backtrack(state):
  tovisit = [state]
  visited = set([])
  max_backtrack = 0
  while tovisit:
    queued = tovisit.pop()
    if queued in visited:
      continue
    visited.add(queued)
    if queued.accepting:
      state_bt = state_backtrack(queued)
      if state_bt < 0:
        return -1
      if state_bt > max_backtrack:
        max_backtrack = state_bt
    for ch,node in queued.d.items():
      if node not in visited:
        tovisit.append(node)
    if queued.default != None:
      if queued.default not in visited:
        tovisit.append(queued.default)
  return max_backtrack

def set_accepting(state, prios):
  tovisit = [state]
  visited = set([])
  while tovisit:
    queued = tovisit.pop()
    if queued in visited:
      continue
    visited.add(queued)
    if queued.accepting:
      sortlist = sorted([(prios[x],x) for x in queued.acceptidset])
      if len(sortlist) > 1 and sortlist[-1][0] == sortlist[-2][0]:
        assert False # priority conflict
      queued.acceptid = sortlist[-1][1]
    for ch,node in queued.d.items():
      if node not in visited:
        tovisit.append(node)
    if queued.default != None:
      if queued.default not in visited:
        tovisit.append(queued.default)

def set_ids(state):
  if "id" in state.__dict__:
    return
  tovisit = [state]
  visited = set([])
  def idgen(n):
    while True:
      yield n
      n+=1
  next_id = idgen(0)
  while tovisit:
    queued = tovisit.pop()
    if queued in visited:
      continue
    visited.add(queued)
    queued.id = next_id.next()
    for ch,node in queued.d.items():
      if node not in visited:
        tovisit.append(node)
    if queued.default != None:
      if queued.default not in visited:
        tovisit.append(queued.default)

dfahost = nfa2dfa(re_compilemulti("[Hh][Oo][Ss][Tt]","\r\n","[#09AHOSTZahostz]+","[ \t]+").nfa())
set_accepting(dfahost, [1,0,0,0])
#set_ids(dfahost)
print fsmviz(dfahost,True)
raise SystemExit()

dfaproblematic = nfa2dfa(re_compilemulti("ab","abcd","abce").nfa())
#print maximal_backtrack(dfaproblematic)
#raise SystemExit(1)

dfaproblematic2 = nfa2dfa(re_compilemulti("ab","abc*d","abc*e").nfa())
#print maximal_backtrack(dfaproblematic2)
#raise SystemExit(1)

print fsmviz(nfa2dfa(re_compilemulti("ab","ac","de").nfa()),True)
raise SystemExit()


print fsmviz(nfa2dfa(re_compilemulti("[Hh][Oo][Ss][Tt]","\r\n","[#09AHOSTZahostz]+","[ \t]+").nfa()),True)

#print fsmviz(re_compilemulti("ab","abcd","abce").nfa(),False)
#print fsmviz(nfa2dfa(re_compilemulti("ab","abcd","abce").nfa()),True)
#print fsmviz(nfa2dfa(re_compilemulti("....a","Host").nfa()),True)
#print fsmviz(nfa2dfa(re_compilemulti(".*ab",".*abcd",".*abce").nfa()),True)
#print fsmviz(nfa2dfa(re_compile("(ab|abcd|abce)*").nfa()),True)
raise SystemExit(1)

#print fsmviz(re_compile("ab|abcd|abce").nfa(),False)
#print fsmviz(nfa2dfa(re_compile("ab|abcd|abce").nfa()),True)

#print fsmviz(nfa2dfa(re_compile("[ab]*|[af]c*[de]").nfa()),True)
#print fsmviz(nfa2dfa(re_compile("(a|b)+|e?(a|f)c*(d|e).").nfa()),True)
#print fsmviz(re_compile("[ab]*|[^af]c*[de]").nfa())
#print fsmviz(nfa2dfa(re_compile("[^af]").nfa()),True)


dfa = nfa2dfa(re_compile("[ab]*|[af]c*[de].").nfa())
print dfa.execute("accef")
