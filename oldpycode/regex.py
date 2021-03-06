# -*- coding: iso-8859-15 -*-
from __future__ import division
from __future__ import print_function
import collections
import random
import re
import sys
try:
  from StringIO import StringIO
except ImportError:
  from io import StringIO
import time

class REContainer(object):
  def __init__(self, parsername, re_by_idx, list_of_reidx_sets, priorities):
    maxbt = 0
    dfa_by_reidx_set = {}
    for reidx_set in list_of_reidx_sets:
      sorted_reidx_set = list(sorted(reidx_set))
      re_set = list([re_by_idx[idx] for idx in sorted_reidx_set])
      dfa = nfa2dfa(re_compilemulti(*re_set).nfa())
      set_accepting(dfa, priorities)
      curbt = maximal_backtrack(dfa)
      dfa_by_reidx_set[reidx_set] = dfa
      if curbt > maxbt:
        maxbt = curbt
    if maxbt > 250: # A bit of safety margin below 255
      assert False
    self.maxbt = maxbt
    self.parsername = parsername
    self.dfa_by_reidx_set = dfa_by_reidx_set
    self.list_of_reidx_sets = list_of_reidx_sets
  def dump_headers(self, sio):
    maxbt = self.maxbt
    parsername = self.parsername
    print(
    """
#define """+parsername.upper()+"""_BACKTRACKLEN ("""+str(maxbt)+""")
#define """+parsername.upper()+"""_BACKTRACKLEN_PLUS_1 (("""+parsername.upper()+"""_BACKTRACKLEN) + 1)

struct """+parsername+"""_parserctx;

struct """+parsername+"""_rectx {
  lexer_uint_t state; // 0 is initial state
  lexer_uint_t last_accept; // LEXER_UINT_MAX means never accepted
  uint8_t backtrackstart;
  uint8_t backtrackend;
  unsigned char backtrack["""+parsername.upper()+"""_BACKTRACKLEN_PLUS_1];
};

static inline void
"""+parsername+"""_init_statemachine(struct """+parsername+"""_rectx *ctx)
{
  ctx->state = 0;
  ctx->last_accept = LEXER_UINT_MAX;
  ctx->backtrackstart = 0;
  ctx->backtrackend = 0;
}

ssize_t
"""+parsername+"""_feed_statemachine(struct """+parsername+"""_rectx *ctx, const struct state *stbl, const void *buf, size_t sz, parser_uint_t *state, ssize_t(*cbtbl[])(const char*, size_t, int, struct """+parsername+"""_parserctx*), const parser_uint_t *cbs, parser_uint_t cb1);//, void *baton);
""", file=sio)
    return
  def dump_all(self, sio):
    parsername = self.parsername
    list_of_reidx_sets = self.list_of_reidx_sets
    print("""
static inline int
"""+parsername+"""_is_fastpath(const struct state *st, unsigned char uch)
{
  return !!(st->fastpathbitmask[uch/64] & (1ULL<<(uch%64)));
}

ssize_t
"""+parsername+"""_feed_statemachine(struct """+parsername+"""_rectx *ctx, const struct state *stbl, const void *buf, size_t sz, parser_uint_t *state, ssize_t(*cbtbl[])(const char*, size_t, int, struct """+parsername+"""_parserctx*), const parser_uint_t *cbs, parser_uint_t cb1)//, void *baton)
{
  const unsigned char *ubuf = (unsigned char*)buf;
  const struct state *st = NULL;
  size_t i;
  int start = 0;
  lexer_uint_t newstate;
  struct """+parsername+"""_parserctx *pctx = CONTAINER_OF(ctx, struct """+parsername+"""_parserctx, rctx);
  if (ctx->state == LEXER_UINT_MAX)
  {
    *state = PARSER_UINT_MAX;
    return -EINVAL;
  }
  if (ctx->state == 0)
  {
    start = 1;
  }
  //printf("Called: %s\\n", buf);
  if (unlikely(ctx->backtrackstart != ctx->backtrackend))
  {
    while (ctx->backtrackstart != ctx->backtrackend)
    {
      st = &stbl[ctx->state];
      ctx->state = st->transitions[ctx->backtrack[ctx->backtrackstart]];
      if (unlikely(ctx->state == LEXER_UINT_MAX))
      {
        if (ctx->last_accept == LEXER_UINT_MAX)
        {
          *state = PARSER_UINT_MAX;
          return -EINVAL;
        }
        ctx->state = ctx->last_accept;
        ctx->last_accept = LEXER_UINT_MAX;
        st = &stbl[ctx->state];
        *state = st->acceptid;
        ctx->state = 0;
        return 0;
      }
      ctx->backtrackstart++;
      if (ctx->backtrackstart >= """+parsername.upper()+"""_BACKTRACKLEN_PLUS_1)
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
          ctx->last_accept = LEXER_UINT_MAX;
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
    if ("""+parsername+"""_is_fastpath(st, ubuf[i]))
    {
      ctx->last_accept = ctx->state;
      while (i + 8 < sz) // FIXME test this thoroughly, all branches!
      {
        if (!"""+parsername+"""_is_fastpath(st, ubuf[i+1]))
        {
          i += 0;
          break;
        }
        if (!"""+parsername+"""_is_fastpath(st, ubuf[i+2]))
        {
          i += 1;
          break;
        }
        if (!"""+parsername+"""_is_fastpath(st, ubuf[i+3]))
        {
          i += 2;
          break;
        }
        if (!"""+parsername+"""_is_fastpath(st, ubuf[i+4]))
        {
          i += 3;
          break;
        }
        if (!"""+parsername+"""_is_fastpath(st, ubuf[i+5]))
        {
          i += 4;
          break;
        }
        if (!"""+parsername+"""_is_fastpath(st, ubuf[i+6]))
        {
          i += 5;
          break;
        }
        if (!"""+parsername+"""_is_fastpath(st, ubuf[i+7]))
        {
          i += 6;
          break;
        }
        if (!"""+parsername+"""_is_fastpath(st, ubuf[i+8]))
        {
          i += 7;
          break;
        }
        i += 8;
      }
      continue;
    }
    newstate = st->transitions[ubuf[i]];
    if (newstate != ctx->state) // Improves perf a lot
    {
      ctx->state = newstate;
    }
    //printf("New state: %d\\n", ctx->state);
    if (unlikely(newstate == LEXER_UINT_MAX)) // use newstate here, not ctx->state, faster
    {
      if (ctx->last_accept == LEXER_UINT_MAX)
      {
        *state = PARSER_UINT_MAX;
        //printf("Error\\n");
        return -EINVAL;
      }
      ctx->state = ctx->last_accept;
      ctx->last_accept = LEXER_UINT_MAX;
      st = &stbl[ctx->state];
      *state = st->acceptid;
      ctx->state = 0;
      if (cbs && st->accepting && cbs[st->acceptid] != PARSER_UINT_MAX)
      {
        cbtbl[cbs[st->acceptid]](buf, i, start, pctx);
      }
      if (cb1 != PARSER_UINT_MAX && st->accepting)
      {
        cbtbl[cb1](buf, i, start, pctx);
      }
      return i;
    }
    st = &stbl[ctx->state]; // strangely, ctx->state seems faster here
    if (st->accepting)
    {
      if (st->final)
      {
        *state = st->acceptid;
        ctx->state = 0;
        ctx->last_accept = LEXER_UINT_MAX;
        if (cbs && st->accepting && cbs[st->acceptid] != PARSER_UINT_MAX)
        {
          cbtbl[cbs[st->acceptid]](buf, i + 1, start, pctx);
        }
        if (cb1 != PARSER_UINT_MAX && st->accepting)
        {
          cbtbl[cb1](buf, i + 1, start, pctx);
        }
        return i + 1;
      }
      else
      {
        ctx->last_accept = ctx->state; // FIXME correct?
      }
    }
    else
    {
      if (ctx->last_accept != LEXER_UINT_MAX)
      {
        ctx->backtrack[ctx->backtrackstart++] = ubuf[i]; // FIXME correct?
        if (ctx->backtrackstart >= """+parsername.upper()+"""_BACKTRACKLEN_PLUS_1)
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
  if (st && cbs && st->accepting && cbs[st->acceptid] != PARSER_UINT_MAX)
  {
    cbtbl[cbs[st->acceptid]](buf, sz, start, pctx);
  }
  if (st && cb1 != PARSER_UINT_MAX && st->accepting)
  {
    cbtbl[cb1](buf, sz, start, pctx);
  }
  *state = PARSER_UINT_MAX;
  return -EAGAIN; // Not yet
}
""", file=sio)
    dict_transitions = {}
    print("#ifdef SMALL_CODE", file=sio)
    print("const lexer_uint_t %s_transitiontbl[][256] = {" % (parsername,), file=sio)
    cur_dictid = 0
    for reidx_set in list_of_reidx_sets:
      sorted_reidx_set = list(sorted(reidx_set))
      #re_list = list([re_by_idx[idx] for idx in sorted_reidx_set])
      #dfa = nfa2dfa(re_compilemulti(*re_list).nfa())
      dfa = self.dfa_by_reidx_set[reidx_set]
      #set_accepting(dfa, priorities)
      dfatbl = set_ids(dfa)
      for stateid in range(len(dfatbl)):
        state = dfatbl[stateid]
        transitions = get_transitions(state)
        if transitions in dict_transitions:
          continue
        dict_transitions[transitions] = cur_dictid
        cur_dictid += 1
        print("{", file=sio)
        for t in transitions:
          if t == 255:
            print("LEXER_UINT_MAX,", file=sio, end=" ")
          else:
            print(t,",", file=sio, end=" ")
        print("},", file=sio)
    print("};", file=sio)
    print("#endif", file=sio)
    for reidx_set in list_of_reidx_sets:
      sorted_reidx_set = list(sorted(reidx_set))
      name = '_'.join(str(x) for x in sorted_reidx_set)
      #re_list = list([re_by_idx[idx] for idx in sorted_reidx_set])
      #dfa = nfa2dfa(re_compilemulti(*re_list).nfa())
      dfa = self.dfa_by_reidx_set[reidx_set]
      #set_accepting(dfa, priorities)
      dfatbl = set_ids(dfa)
      print("DFA %s has %d entries" % (name, len(dfatbl)), file=sys.stderr)
      if len(dfatbl) > 255:
        assert False # 255 is reserved for invalid non-accepting state
      print("const struct state %s_states_%s[] = {" % (parsername,name,), file=sio)
      for stateid in range(len(dfatbl)):
        state = dfatbl[stateid]
        print("{", file=sio, end=" ")
        print(".accepting =", file=sio, end=" ")
        print(state.accepting and "1," or "0,", file=sio, end=" ")
        print(".acceptid =", file=sio, end=" ")
        if state.accepting:
          print(sorted_reidx_set[state.acceptid],",", file=sio, end=" ")
        else:
          print(0,",", file=sio, end=" ")
        print(".final =", (state_is_final(state) and 1 or 0), ",", file=sio)
        print(".fastpathbitmask = {", file=sio, end=" ")
        if state.accepting and not state_is_final(state):
          for iid in range(4):
            curval = 0
            for jid in range(64):
              uch = 64*iid + jid
              ch = chr(uch)
              if ch in state.d and state.d[ch].id == stateid:
                curval |= (1<<jid)
              elif state.default and state.default.id == stateid:
                curval |= (1<<jid)
            print("0x%x," % (curval,), file=sio, end=" ")
        print("},", file=sio)
        transitions = get_transitions(state)
        print("#ifdef SMALL_CODE", file=sio)
        print(".transitions = "+parsername+"_transitiontbl[", dict_transitions[transitions],"],", file=sio)
        print("#else", file=sio)
        print(".transitions = ", file=sio, end=" ")
        print("{", file=sio, end=" ")
        for t in transitions:
          if t == 255:
            print("LEXER_UINT_MAX,", file=sio, end=" ")
          else:
            print(t,",", file=sio, end=" ")
        print("},", file=sio)
        print("#endif", file=sio)
        print("},", file=sio)
      print("};", file=sio)
    return


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
  result = StringIO()
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
      taintid += 1 # FIXME looks suspicious to have this in middle of two uses
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
      print(expr, s, dfa.execute(s), bool(compiled.match(s)))

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

def dfa_assert_recursive(state, assertion):
  tovisit = [state]
  visited = set([])
  max_backtrack = 0
  while tovisit:
    queued = tovisit.pop()
    if queued in visited:
      continue
    visited.add(queued)
    assert assertion(state)
    for ch,node in queued.d.items():
      if node not in visited:
        tovisit.append(node)
    if queued.default != None:
      if queued.default not in visited:
        tovisit.append(queued.default)

def check_cb_first(state, acceptid, state2):
  if state2.accepting and state2.acceptid == acceptid:
    dfa_assert_recursive(state2,
      lambda s3: s3.accepting and s3.acceptid == acceptid)
  else:
    dfa_assert_recursive(state2,
      lambda s3: not s3.accepting or s3.acceptid != acceptid)

def check_cb(state, acceptid):
  for ch,node in state.d.items():
    check_cb_first(state, acceptid, node)
  if state.default != None:
    check_cb_first(state, acceptid, state.default)

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
    #queued.id = next_id.next()
    queued.id = next(next_id)
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

def get_transitions(state):
  transitions = []
  for n in range(256):
    ch = chr(n)
    if ch in state.d:
      transitions.append(state.d[ch].id)
      #print state.d[ch].id,",",
    elif state.default:
      transitions.append(state.default.id)
      #print state.default.id,",",
    else:
      transitions.append(255)
      #print 255,",",
  return tuple(transitions)

#def dump_all(sio, parsername, re_by_idx, list_of_reidx_sets, priorities):

def dump_state(state):
  dfatbl = set_ids(state)
  
  print("const struct state states[] = {")
  for stateid in range(len(dfatbl)):
    state = dfatbl[stateid]
    print("{", end=" ")
    print(".accepting =", end=" ")
    print(state.accepting and "1," or "0,", end=" ")
    print(".acceptid =", end=" ")
    if state.accepting:
      print(state.acceptid,",",end=" ")
    else:
      print(0,",",end=" ")
    print(".transitions =", end=" ")
    print("{", end=" ")
    for n in range(256):
      ch = chr(n)
      if ch in state.d:
        print(state.d[ch].id,",",end=" ")
      elif state.default:
        print(state.default.id,",",end=" ")
      else:
        print("LEXER_UINT_MAX,",end=" ")
    print("},",end=" ")
    print("},")
  print("};")

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
