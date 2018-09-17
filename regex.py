# -*- coding: iso-8859-15 -*-
from __future__ import division
import collections
import random
import re
import sys
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
      result.write("n%d [label=\"%d%s%s%s\"];\n" % (n,n,(node2.accepting and "+" or ""),(tainted and "*" or ""),acceptid))
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
    # add all state transitions to d2 and defaults
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
        ec,tainted,acceptidset = epsilonclosure(nns2) # XXX: slow!
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

def parse_bracketexpr(s):
  assert s
  l = s[1:].split("]",1)
  assert len(l) == 2
  inverse = False
  fullstr = s[0]+l[0]
  if s[0] == "^":
    inverse = True
    fullstr = l[0]
  last = None
  literalstr = ""
  idx = 0
  while idx < len(fullstr):
    first = fullstr[idx]
    idx += 1
    if first == "\\":
      first = fullstr[idx]
      idx += 1
      if first == "n":
        newlast = "\n"
      elif first == "r":
        newlast = "\r"
      elif first == "t":
        newlast = "\t"
      elif first == "x":
        newlast = chr(int(fullstr[idx:idx+2], 16))
        idx += 2
      else:
        assert False
      if last != None:
        literalstr += last
      last = newlast
    elif first == "-" and (last!=None) and idx < len(fullstr):
      first = fullstr[idx]
      idx += 1
      if first == "\\":
        first = fullstr[idx]
        idx += 1
        if first == "n":
          newlast = "\n"
        elif first == "r":
          newlast = "\r"
        elif first == "t":
          newlast = "\t"
        elif first == "x":
          newlast = chr(int(fullstr[idx:idx+2], 16))
          idx += 2
        else:
          assert False
      elif first == "-":
        assert False
      else:
        newlast = first
      literalstr += ''.join(chr(c) for c in range(ord(last), ord(newlast)+1))
      last = None
    else:
      newlast = first
      if last != None:
        literalstr += last
      last = newlast
  if last != None:
    literalstr += last
  if inverse:
    literalstr = ''.join(chr(c) for c in range(256) if chr(c) not in literalstr)
  return literals(literalstr),l[1]

#print parse_bracketexpr("[A-Za-z0-9]"[1:])[0].s
#print parse_bracketexpr("[^\x00-AC-\xff]"[1:])[0].s
#print parse_bracketexpr("[^\\x00-AC-\\xff]"[1:])[0].s
#print parse_bracketexpr("[-!#$%&'*+.^_`|~0-9A-Za-z]"[1:])[0].s
#print parse_bracketexpr("[]:/?#@!$&'()*+,;=0-9A-Za-z._~%[-]"[1:])[0].s
#raise SystemExit()

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
    return state.tbl
  tbl = []
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
    tbl.append(queued)
    for ch,node in queued.d.items():
      if type(node) == set:
        for n in node:
          if n not in visited:
            tovisit.append(n)
      else:
        if node not in visited:
          tovisit.append(node)
    if "default" in queued.__dict__ and queued.default != None:
      if queued.default not in visited:
        tovisit.append(queued.default)
  state.tbl = tbl
  return tbl

def state_is_final(state):
  if state.default:
    return False
  if state.d:
    return False
  return True

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

#print fsmviz(re_compile(foldstart).nfa(),False)
#print "------------------"
#print fsmviz(nfa2dfa(re_compile(foldstart).nfa()),True)
#raise SystemExit()



#print fsmviz(nfa2dfa(re_compile("[^\x00-AC-\xff]").nfa()),True)
#print "-------------------"
#print fsmviz(nfa2dfa(re_compile("[A-Ca-c0-3]").nfa()),True)
#raise SystemExit()

#print fsmviz(nfa2dfa(re_compile("[A-Ca-c0-3]").nfa()),True)
#raise SystemExit()

#dfahost = nfa2dfa(re_compilemulti("[Hh][Oo][Ss][Tt]","\r\n","[#09AHOSTZahostz]+","[ \t]+").nfa())
#set_accepting(dfahost, [1,0,0,0])
#dfatbl = set_ids(dfahost)

#if len(dfatbl) > 255:
#  assert False # 255 is reserved for invalid non-accepting state
#if maximal_backtrack(dfahost) > 255:
#  assert False

def dump_headers(re_by_idx, list_of_reidx_sets):
  maxbt = 0
  for reidx_set in list_of_reidx_sets:
    re_set = set([re_by_idx[idx] for idx in reidx_set])
    dfa = nfa2dfa(re_compilemulti(*re_set).nfa())
    curbt = maximal_backtrack(dfa)
    if curbt > maxbt:
      maxbt = curbt
  if maxbt > 250: # A bit of safety margin below 255
    assert False
  print \
  """\
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define BACKTRACKLEN (%d)
#define BACKTRACKLEN_PLUS_1 ((BACKTRACKLEN) + 1)

struct state {
  uint8_t accepting;
  uint8_t acceptid;
  uint8_t final;
  uint8_t transitions[256];
};

struct rectx {
  uint8_t state; // 0 is initial state
  uint8_t last_accept; // 255 means never accepted
  uint8_t backtrackstart;
  uint8_t backtrackend;
  uint8_t backtrack[BACKTRACKLEN_PLUS_1];
};

static void
init_statemachine(struct rectx *ctx)
{
  ctx->state = 0;
  ctx->last_accept = 255;
  ctx->backtrackstart = 0;
  ctx->backtrackend = 0;
}

static ssize_t
feed_statemachine(struct rectx *ctx, const struct state *stbl, const void *buf, size_t sz, uint8_t *state)
{
  const unsigned char *ubuf = (unsigned char*)buf;
  const struct state *st;
  size_t i;
  uint8_t newstate;
  if (ctx->state == 255)
  {
    *state = 255;
    return -EINVAL;
  }
  //printf("Called: %%s\\n", buf);
  if (unlikely(ctx->backtrackstart != ctx->backtrackend))
  {
    while (ctx->backtrackstart != ctx->backtrackend)
    {
      st = &stbl[ctx->state];
      ctx->state = st->transitions[ctx->backtrack[ctx->backtrackstart]];
      if (unlikely(ctx->state == 255))
      {
        if (ctx->last_accept == 255)
        {
          *state = 255;
          return -EINVAL;
        }
        ctx->state = ctx->last_accept;
        ctx->last_accept = 255;
        st = &stbl[ctx->state];
        *state = st->acceptid;
        ctx->state = 0;
        return 0;
      }
      ctx->backtrackstart++;
      if (ctx->backtrackstart >= BACKTRACKLEN_PLUS_1)
      {
        ctx->backtrackstart = 0;
      }
      st = &stbl[ctx->state];
      if (st->accepting)
      {
        if (st->final)
        {
          *state = st->acceptid;
          ctx->state = 0;
          ctx->last_accept = 255;
          return 0;
        }
        else
        {
          ctx->last_accept = ctx->state; // FIXME correct?
        }
      }
    }
  }
  for (i = 0; i < sz; i++)
  {
    st = &stbl[ctx->state];
    newstate = st->transitions[ubuf[i]];
    if (newstate != ctx->state) // Improves perf a lot
    {
      ctx->state = newstate;
    }
    //printf("New state: %%d\\n", ctx->state);
    if (unlikely(newstate == 255)) // use newstate here, not ctx->state, faster
    {
      if (ctx->last_accept == 255)
      {
        *state = 255;
        //printf("Error\\n");
        return -EINVAL;
      }
      ctx->state = ctx->last_accept;
      ctx->last_accept = 255;
      st = &stbl[ctx->state];
      *state = st->acceptid;
      ctx->state = 0;
      return i;
    }
    st = &stbl[ctx->state]; // strangely, ctx->state seems faster here
    if (st->accepting)
    {
      if (st->final)
      {
        *state = st->acceptid;
        ctx->state = 0;
        ctx->last_accept = 255;
        return i + 1;
      }
      else
      {
        ctx->last_accept = ctx->state; // FIXME correct?
      }
    }
    else
    {
      if (ctx->last_accept != 255)
      {
        ctx->backtrack[ctx->backtrackstart++] = ubuf[i]; // FIXME correct?
        if (ctx->backtrackstart >= BACKTRACKLEN_PLUS_1)
        {
          ctx->backtrackstart = 0;
        }
        if (ctx->backtrackstart == ctx->backtrackend)
        {
          abort();
        }
      }
    }
  }
  *state = 255;
  return -EAGAIN; // Not yet
}
""" % (maxbt,)
  return

def dump_all(re_by_idx, list_of_reidx_sets, priorities):
  for reidx_set in list_of_reidx_sets:
    sorted_reidx_set = list(sorted(reidx_set))
    name = '_'.join(str(x) for x in sorted_reidx_set)
    re_list = list([re_by_idx[idx] for idx in sorted_reidx_set])
    dfa = nfa2dfa(re_compilemulti(*re_list).nfa())
    set_accepting(dfa, priorities)
    dfatbl = set_ids(dfa)
    print >> sys.stderr, "DFA %s has %d entries" % (name, len(dfatbl))
    if len(dfatbl) > 255:
      assert False # 255 is reserved for invalid non-accepting state
    print "const struct state states_%s[] = {" % (name,)
    for stateid in range(len(dfatbl)):
      state = dfatbl[stateid]
      print "{",
      print ".accepting =",
      print state.accepting and "1," or "0,",
      print ".acceptid =",
      if state.accepting:
        print sorted_reidx_set[state.acceptid],",",
      else:
        print 0,",",
      print ".final =", (state_is_final(state) and 1 or 0), ","
      print ".transitions =",
      print "{",
      for n in range(256):
        ch = chr(n)
        if ch in state.d:
          print state.d[ch].id,",",
        elif state.default:
          print state.default.id,",",
        else:
          print 255,",",
      print "},",
      print "},"
    print "};"
  return

def dump_state(state):
  dfatbl = set_ids(state)
  
  print "const struct state states[] = {"
  for stateid in range(len(dfatbl)):
    state = dfatbl[stateid]
    print "{",
    print ".accepting =",
    print state.accepting and "1," or "0,",
    print ".acceptid =",
    if state.accepting:
      print state.acceptid,",",
    else:
      print 0,",",
    print ".transitions =",
    print "{",
    for n in range(256):
      ch = chr(n)
      if ch in state.d:
        print state.d[ch].id,",",
      elif state.default:
        print state.default.id,",",
      else:
        print 255,",",
    print "},",
    print "},"
  print "};"

##print fsmviz(dfahost,True)
#raise SystemExit()
#
#dfaproblematic = nfa2dfa(re_compilemulti("ab","abcd","abce").nfa())
##print maximal_backtrack(dfaproblematic)
##raise SystemExit(1)
#
#dfaproblematic2 = nfa2dfa(re_compilemulti("ab","abc*d","abc*e").nfa())
##print maximal_backtrack(dfaproblematic2)
##raise SystemExit(1)
#
#print fsmviz(nfa2dfa(re_compilemulti("ab","ac","de").nfa()),True)
#raise SystemExit()
#
#
#print fsmviz(nfa2dfa(re_compilemulti("[Hh][Oo][Ss][Tt]","\r\n","[#09AHOSTZahostz]+","[ \t]+").nfa()),True)
#
##print fsmviz(re_compilemulti("ab","abcd","abce").nfa(),False)
##print fsmviz(nfa2dfa(re_compilemulti("ab","abcd","abce").nfa()),True)
##print fsmviz(nfa2dfa(re_compilemulti("....a","Host").nfa()),True)
##print fsmviz(nfa2dfa(re_compilemulti(".*ab",".*abcd",".*abce").nfa()),True)
##print fsmviz(nfa2dfa(re_compile("(ab|abcd|abce)*").nfa()),True)
#raise SystemExit(1)
#
##print fsmviz(re_compile("ab|abcd|abce").nfa(),False)
##print fsmviz(nfa2dfa(re_compile("ab|abcd|abce").nfa()),True)
#
##print fsmviz(nfa2dfa(re_compile("[ab]*|[af]c*[de]").nfa()),True)
##print fsmviz(nfa2dfa(re_compile("(a|b)+|e?(a|f)c*(d|e).").nfa()),True)
##print fsmviz(re_compile("[ab]*|[^af]c*[de]").nfa())
##print fsmviz(nfa2dfa(re_compile("[^af]").nfa()),True)
#
#
#dfa = nfa2dfa(re_compile("[ab]*|[af]c*[de].").nfa())
#print dfa.execute("accef")
