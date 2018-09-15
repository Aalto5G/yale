# -*- coding: iso-8859-15 -*-
from __future__ import division
import collections
import random
import re
import cStringIO


class dfanode(object):
  def __init__(self,final=False):
    self.d = {}
    self.final = final
    self.default = None
  def connect(self,ch,node):
    assert ch not in self.d
    self.d[ch] = node
  def connect_default(self,node):
    assert self.default == None
    self.default = node
  def execute(self,str):
    if not str:
      return self.final
    elif str[0] in self.d:
      return self.d[str[0]].execute(str[1:])
    elif self.default:
      return self.default.execute(str[1:])
    else:
      return False

class nfanode(object):
  def __init__(self,final=False):
    self.d = {}
    self.final = final
    self.defaults = set()
  def connect(self,ch,node):
    self.d.setdefault(ch,set()).add(node)
  def connect_default(self,node):
    self.defaults.add(node)

def fsmviz(begin,deterministic=False):
  def idgen(n):
    while True:
      yield n
      n+=1
  next_id = idgen(0)
  d = {}
  q = collections.deque()
  result = cStringIO.StringIO()
  result.write("digraph fsm {\n")
  def add_node(node2):
    if not node2 in d:
      n = next_id.next()
      result.write("n%d [label=\"%d%s\"];\n" % (n,n,(node2.final and "+" or "")))
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
          result.write("n%d -> n%d [label=\"%s\"];\n" % (d[node],d[node2],ch))
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
  return frozenset(closure)

def nfa2dfa(begin):
  dfabegin = epsilonclosure(set([begin]))
  d = {}
  d[dfabegin] = dfanode(True in (x.final for x in dfabegin))
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
    defaultsec = epsilonclosure(defaults)
    if defaultsec:
      if defaultsec not in d:
        d[defaultsec] = dfanode(True in (x.final for x in defaultsec))
        q.append(defaultsec)
      d[nns].connect_default(d[defaultsec])
    for ch,nns2 in d2.items():
      if ch:
        nns2.update(defaults)
        ec = epsilonclosure(nns2) # XXX: hidas!
        if ec not in d:
          d[ec] = dfanode(True in (x.final for x in ec))
          q.append(ec)
        d[nns].connect(ch,d[ec])
  return d[dfabegin]

class regexp(object):
  def nfa(self):
    begin,end = nfanode(),nfanode(True)
    self.gennfa(begin,end)
    return begin

class wildcard(regexp):
  def gennfa(self,begin,end):
    begin.connect_default(end)

class emptystr(regexp):
  def gennfa(self,begin,end):
    begin.connect("",end)

class literals(regexp):
  def __init__(self,s):
    self.s = s
  def gennfa(self,begin,end):
    for ch in self.s:
      begin.connect(ch,end)

class concat(regexp):
  def __init__(self,re1,re2):
    self.re1 = re1
    self.re2 = re2
  def gennfa(self,begin,end):
    middle = nfanode()
    self.re1.gennfa(begin,middle)
    self.re2.gennfa(middle,end)

class altern(regexp):
  def __init__(self,re1,re2):
    self.re1 = re1
    self.re2 = re2
  def gennfa(self,begin,end):
    self.re1.gennfa(begin,end)
    self.re2.gennfa(begin,end)

class alternmulti(regexp):
  def __init__(self,*res):
    self.res = res
  def nfa(self):
    begin = nfanode()
    for re in self.res:
      end = nfanode(True)
      re.gennfa(begin, end)
    return begin

class star(regexp):
  def __init__(self,re):
    self.re = re
  def gennfa(self,begin,end):
    begin1,end1 = nfanode(),nfanode()
    self.re.gennfa(begin1,end1)
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

test_re("[ab]*|[af]c*[de]",50000)
test_re("(a|b)+|e?(a|f)c*(d|e).",50000)

#print fsmviz(re_compilemulti("ab","abcd","abce").nfa(),False)
#print fsmviz(nfa2dfa(re_compilemulti("ab","abcd","abce").nfa()),True)
print fsmviz(nfa2dfa(re_compilemulti("....a","Host").nfa()),True)
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
