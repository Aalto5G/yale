from __future__ import division
from __future__ import print_function
import regex

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

def firstset_issubset(a,b):
  for k in a.keys():
    if k not in b:
      return False
  for k,v in a.items():
    if not v.issubset(b[k]):
      return False
  return True

class WrapCB(object):
  def __init__(self, p, token, cbname):
    assert p.isTerminal(token)
    self.token = token
    self.cbname = cbname
class Action(object):
  def __init__(self, cbname):
    self.token = 255
    self.cbname = cbname

def unwrap(x):
  if type(x) == list:
    return list(unwrap(y) for y in x)
  if type(x) == WrapCB:
    return x.token
  if type(x) == Action:
    return x # XXX or x.token?
  if type(x) == int:
    return x
  assert False

def unactionize(x):
  if type(x) == list:
    return list(unactionize(y) for y in x if type(y) != Action)
  if type(x) == WrapCB:
    return x
  if type(x) == Action:
    return x
  if type(x) == int:
    return x
  assert False

def unwrap_unactionize(x):
  return unwrap(unactionize(x))
  #if type(x) == list:
  #  return list(unwrap_unactionize(y) for y in x if type(y) != Action)
  #if type(x) == WrapCB:
  #  return x.token
  #if type(x) == int:
  #  return x
  #assert False


class ParserGen(object):
  def __init__(self, parsername):
    self.re_by_idx = []
    self.priorities = []
    self.terminals = []
    self.nonterminals = []
    self.parsername = parsername
    self.tokens_finalized = False
    self.epsilon = -1
    self.eof = -2
  def action(self, cbname):
    return Action(cbname)
  def wrapCB(self, token, cbname):
    return WrapCB(self, token, cbname)
  def add_token(self, re, priority=0):
    assert not self.tokens_finalized
    assert len(self.priorities) < 255
    result = len(self.priorities)
    self.re_by_idx.append(re)
    self.priorities.append(priority)
    self.terminals.append(result)
    return result
  def finalize_tokens(self):
    self.tokens_finalized = True
    self.num_terminals = len(self.priorities)
  def add_nonterminal(self):
    assert self.tokens_finalized
    assert len(self.priorities) + len(self.nonterminals) < 255
    result = len(self.priorities) + len(self.nonterminals)
    self.nonterminals.append(result)
    return result
  def start_state(self, S):
    self.S = S
    return S
  def firstset_func(self,rhs):
    if len(rhs) == 0:
      return {self.epsilon: set()}
      #return set([epsilon])
    first=rhs[0]
    if self.isTerminal(first):
      return firstset_singleton(first)
      #return {first: set()}
      #return set([first])
    if self.epsilon not in self.Fi[first]:
      return self.Fi[first]
    else:
      #result = self.Fi[first][:]
      result = dict(self.Fi[first])
      del result[self.epsilon]
      firstset_update(result, self.firstset_func(rhs[1:]))
      #result.update(self.firstset_func(rhs[1:])) # FIXME
      return result
      #return self.Fi[first].difference(set([epsilon])).union(self.firstset_func(rhs[1:]))
  def isTerminal(self,x):
    assert self.tokens_finalized
    if type(x) == WrapCB:
      return True
    if type(x) == Action:
      return False
    return x >= 0 and x < self.num_terminals
  def get_max_sz_dfs(self, eof=-1, maxrecurse=16384):
    terminals = self.terminals
    rules = self.rules
    Tt = self.Tt
    S = self.S
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
      if last in terminals or type(last) == Action:
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
  def set_rules(self, rules):
    self.rules = rules
  def parse(self,req):
    p = self
    revreq = list(reversed(req))
    eof = p.eof
    S = p.S
    stack = [eof, S]
    while True:
      if len(stack) > p.max_stack_size:
        raise Exception("stack overflow")
      if stack[-1] == eof and len(revreq) == 0:
        break
      if stack[-1] == eof or len(revreq) == 0:
        raise Exception("parse error")
      sym = revreq[-1]
      if p.isTerminal(stack[-1]):
        #print "Removing terminal %d" % (sym,)
        if stack[-1] != sym:
          raise Exception("parse error")
        revreq.pop()
        stack.pop()
        continue
      #print "Getting action %d %d" % (stack[-1], sym,)
      action = p.Tt[stack[-1]][sym][0]
      if action == None:
        raise Exception("parse error")
      rule = p.rules[action]
      rhs = unwrap(rule[1])
      stack.pop()
      for val in reversed(rhs):
        stack.append(val)
  def gen_parser(self):
    rules = self.rules
    self.Fi = {}
    Fi = self.Fi
    for nonterminal in self.nonterminals:
      Fi[nonterminal] = {}#set()
    changed = True
    while changed:
      changed = False
      for rule in rules:
        nonterminal = rule[0]
        origrhs = unactionize(rule[1])
        rhs = unwrap_unactionize(origrhs)
        trhs = tuple(rhs)
        firstset = self.firstset_func(origrhs)
        Fi.setdefault(trhs, {})
        #if firstset.issubset(Fi[trhs]):
        if firstset_issubset(firstset, Fi[trhs]):
          continue
        firstset_update(Fi[trhs], firstset)
        #Fi[trhs].update(firstset)
        changed = True
      for rule in rules:
        nonterminal = rule[0]
        rhs = unwrap_unactionize(rule[1])
        trhs = tuple(rhs)
        #if Fi[trhs].issubset(Fi[nonterminal]):
        if firstset_issubset(Fi[trhs], Fi[nonterminal]):
          continue
        firstset_update(Fi[nonterminal], Fi[trhs])
        #Fi[nonterminal].update(Fi[trhs])
        changed = True
    #
    Fo = {}
    for nonterminal in self.nonterminals:
      Fo[nonterminal] = {}#set()
    #Fo[S] = set([eof])
    Fo[self.S] = {self.eof: set()}
    #
    changed = True
    while changed:
      changed = False
      for rule in rules:
        nonterminal = rule[0]
        origrhs = unactionize(rule[1])
        #rhs = unwrap(origrhs)
        rhs = unwrap_unactionize(origrhs)
        for idx in range(len(rhs)):
          origrhsmid = origrhs[idx]
          rhsmid = rhs[idx]
          if self.isTerminal(rhsmid):
            continue
          origrhsleft = origrhs[:idx]
          rhsleft = rhs[:idx]
          origrhsright = origrhs[idx+1:]
          rhsright = rhs[idx+1:]
          firstrhsright = self.firstset_func(origrhsright)
          for terminal in self.terminals:
            if terminal in firstrhsright:
              if terminal not in Fo[rhsmid]:
                changed = True
                firstset_update(Fo[rhsmid], {terminal: firstrhsright[terminal]})
                #Fo[rhsmid].add(terminal)
          if self.epsilon in firstrhsright:
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
    #
    T = {}
    for A in self.nonterminals:
      T[A] = {}
      for a in self.terminals:
        T[A][a] = set([])
    #
    for idx in range(len(rules)):
      rule = rules[idx]
      A = rule[0]
      w = unwrap_unactionize(rule[1])
      tw = tuple(w)
      for a in self.terminals:
        fi = Fi[tw]
        fo = Fo[A]
        if a in fi:
          if len(fi[a]) == 0:
            T[A][a].add((idx, None))
          for cb in fi[a]:
            T[A][a].add((idx, cb))
        if self.epsilon in fi and a in fo:
          if len(fo[a]) == 0:
            T[A][a].add((idx, None))
          for cb in fo[a]:
            T[A][a].add((idx, cb))
    #
    Tt = {}
    for A in self.nonterminals:
      Tt[A] = {}
      for a in self.terminals:
        Tt[A][a] = None
    #
    for A in self.nonterminals:
      for a in self.terminals:
        if len(T[A][a]) > 1:
          raise Exception("Conflict %d %d %s" % (A,a,T[A][a]))
        elif len(T[A][a]) == 1:
          Tt[A][a] = set(T[A][a]).pop()
          #print "Non-conflict %d %d: %s" % (A,a,Tt[A][a])
    #
    self.T = T
    self.Tt = Tt
    #
    self.max_stack_size = self.get_max_sz_dfs()
    if self.max_stack_size > 255:
      assert False
    #
    callbacks_by_name = dict()
    callbacks_by_value = []
    for lhs,rhs in rules:
      for rhsitem in rhs:
        if type(rhsitem) == Action or type(rhsitem) == WrapCB:
          if rhsitem.cbname not in callbacks_by_name:
            callbacks_by_name[rhsitem.cbname] = len(callbacks_by_value)
            callbacks_by_value.append(rhsitem.cbname)
    assert len(callbacks_by_value) <= 255 # 255 reserved for NULL
    self.callbacks_by_value = callbacks_by_value
    self.callbacks_by_name = callbacks_by_name
    #
    list_of_reidx_sets = set()
    # Single token DFAs
    list_of_reidx_sets.update([frozenset([x]) for x in range(self.num_terminals)])
    #
    for X in self.nonterminals:
      list_of_reidx_sets.add(frozenset([x for x in self.terminals if T[X][x]]))
    self.list_of_reidx_sets = list_of_reidx_sets
    self.rec = regex.REContainer(self.parsername, self.re_by_idx, self.list_of_reidx_sets, self.priorities)
  #
  def print_headers(self, sio):
    parsername = self.parsername
    re_by_idx = self.re_by_idx
    list_of_reidx_sets = self.list_of_reidx_sets
    priorities = self.priorities
    callbacks_by_name = self.callbacks_by_name
    callbacks_by_value = self.callbacks_by_value
    max_stack_size = self.max_stack_size
    terminals = self.terminals
    nonterminals = self.nonterminals
    rec = self.rec
    T = self.T
    Tt = self.Tt
    rules = self.rules
    print("#include \"yalecommon.h\"", file=sio)
    #regex.dump_headers(sio, parsername, re_by_idx, list_of_reidx_sets)
    rec.dump_headers(sio)
    #
    print("""
struct %s_parserctx {
  uint8_t stacksz;
  struct ruleentry stack[%d]; // WAS: uint8_t stack[...];
  struct %s_rectx rctx;
  uint8_t saved_token;
};

static inline void %s_parserctx_init(struct %s_parserctx *pctx)
{
  pctx->saved_token = 255;
  pctx->stacksz = 1;
  pctx->stack[0].rhs = %d;
  pctx->stack[0].cb = 255;
  %s_init_statemachine(&pctx->rctx);
}

ssize_t %s_parse_block(struct %s_parserctx *pctx, const char *blk, size_t sz, void *baton);
""" % (parsername, max_stack_size, parsername, parsername, parsername, self.S, parsername, parsername, parsername), file=sio)
    #
  def print_parser(self, sio):
    parsername = self.parsername
    re_by_idx = self.re_by_idx
    list_of_reidx_sets = self.list_of_reidx_sets
    priorities = self.priorities
    callbacks_by_name = self.callbacks_by_name
    callbacks_by_value = self.callbacks_by_value
    max_stack_size = self.max_stack_size
    terminals = self.terminals
    nonterminals = self.nonterminals
    rec = self.rec
    T = self.T
    Tt = self.Tt
    rules = self.rules
    #regex.dump_all(sio, parsername, re_by_idx, list_of_reidx_sets, priorities)
    rec.dump_all(sio)
    #
    print("const uint8_t %s_num_terminals;" % parsername, file=sio)
    print("void(*%s_callbacks[])(const char*, size_t, void*) = {" % parsername, file=sio)
    for cb in callbacks_by_value:
      print(cb,",", file=sio)
    print("};", file=sio)
    #
    print("""
struct %s_parserstatetblentry {
  const struct state *re;
  const uint8_t rhs[%d];
  const uint8_t cb[%d];
};
""" % (parsername, len(terminals), len(terminals),), file=sio)
    #
    print("const uint8_t %s_num_terminals = %d;" % (parsername, len(terminals),), file=sio)
    print("const uint8_t %s_start_state = %d;" % (parsername, self.S,), file=sio)
    #
    print("const struct reentry %s_reentries[] = {" % (parsername,), file=sio)
    #
    for x in sorted(terminals):
      print("{", file=sio)
      name = str(x)
      print(".re = " + parsername + "_states_" + name + ",", file=sio)
      print("},", file=sio)
    #
    print("};", file=sio)
    #
    print("const struct %s_parserstatetblentry %s_parserstatetblentries[] = {" % (parsername, parsername,), file=sio)
    #
    for X in sorted(nonterminals):
      sorted_reidx_set = sorted([x for x in terminals if T[X][x]])
      #re_list = list([self.re_by_idx[idx] for idx in sorted_reidx_set])
      #dfa = regex.nfa2dfa(regex.re_compilemulti(*re_list).nfa())
      #regex.set_accepting(dfa, priorities)
      dfa = self.rec.dfa_by_reidx_set[frozenset(sorted_reidx_set)]
      for x in sorted(terminals):
        if Tt[X][x] == None or Tt[X][x][1] == None:
          continue
        regex.check_cb(dfa, x)
      print("{", file=sio)
      name = '_'.join(str(x) for x in sorted([x for x in terminals if T[X][x]]))
      print(".re = "+parsername+"_states_" + name + ",", file=sio)
      print(".rhs = {", file=sio, end=" ")
      for x in sorted(terminals):
        if Tt[X][x] == None:
          print(255,",", file=sio, end=" ")
        else:
          print(Tt[X][x][0],",", file=sio, end=" ")
      print("},", file=sio)
      print(".cb = {", file=sio, end=" ")
      for x in sorted(terminals):
        if Tt[X][x] == None or Tt[X][x][1] == None:
          print("255,", file=sio, end=" ")
        else:
          print(callbacks_by_name[Tt[X][x][1]],",", file=sio, end=" ")
      print("},", file=sio)
      print("},", file=sio)
    #
    print("};", file=sio)
    #
    for n in range(len(rules)):
      lhs,rhs = rules[n]
      print("const struct ruleentry %s_rule_%d[] = {" % (parsername, n,), file=sio)
      for rhsitem in reversed(rhs):
        print("{", file=sio, end=" ")
        if type(rhsitem) == Action:
          print(".rhs = 255, .cb = ", callbacks_by_name[rhsitem.cbname], file=sio, end=" ")
        elif type(rhsitem) == WrapCB:
          x = rhsitem.token
          sortex_reidx_set = sorted([x])
          #re_list = list([self.re_by_idx[idx] for idx in sorted_reidx_set])
          #dfa = regex.nfa2dfa(regex.re_compilemulti(*re_list).nfa())
          #regex.set_accepting(dfa, priorities)
          dfa = self.rec.dfa_by_reidx_set[frozenset(sorted_reidx_set)]
          regex.check_cb(dfa, x)
          print(".rhs =", rhsitem.token, ",", ".cb = ", callbacks_by_name[rhsitem.cbname], file=sio, end=" ")
        else:
          print(".rhs =", rhsitem, ",", ".cb = 255", file=sio, end=" ")
        print("}",",", file=sio, end=" ")
      print(file=sio)
      print("};", file=sio)
      print("const uint8_t %s_rule_%d_len = sizeof(%s_rule_%d)/sizeof(struct ruleentry);" % (parsername,n,parsername,n,), file=sio)
    print("const struct rule %s_rules[] = {" % (parsername,), file=sio)
    for n in range(len(rules)):
      lhs,rhs = rules[n]
      print("{", file=sio)
      print("  .lhs =", lhs, ",", file=sio)
      print("  .rhssz = sizeof(%s_rule_%d)/sizeof(struct ruleentry)," % (parsername, n,), file=sio)
      print("  .rhs = %s_rule_%d," % (parsername,n,), file=sio)
      print("},", file=sio)
    print("};", file=sio)
    #
    print("""
static inline ssize_t
"""+parsername+"""_get_saved_token(struct """+parsername+"""_parserctx *pctx, const struct state *restates,
                const char *blkoff, size_t szoff, uint8_t *state,
                const uint8_t *cbs, uint8_t cb1, void *baton)
{
  if (pctx->saved_token != 255)
  {
    *state = pctx->saved_token;
    pctx->saved_token = 255;
    return 0;
  }
  return """+parsername+"""_feed_statemachine(&pctx->rctx, restates, blkoff, szoff, state, """+parsername+"""_callbacks, cbs, cb1, baton);
}

#undef EXTRA_SANITY

ssize_t """+parsername+"""_parse_block(struct """+parsername+"""_parserctx *pctx, const char *blk, size_t sz, void *baton)
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
    if (curstate == 255)
    {
      void (*cb1f)(const char *, size_t, void*);
      cb1f = """+parsername+"""_callbacks[pctx->stack[pctx->stacksz - 1].cb];
      cb1f(NULL, 0, baton);
      pctx->stacksz--;
    }
    else if (curstate < """+parsername+"""_num_terminals)
    {
      uint8_t state;
      const struct state *restates = """+parsername+"""_reentries[curstate].re;
      uint8_t cb1;
      cb1 = pctx->stack[pctx->stacksz - 1].cb;
      ret = """+parsername+"""_get_saved_token(pctx, restates, blk+off, sz-off, &state, NULL, cb1, baton);
      if (ret == -EAGAIN)
      {
        off = sz;
        return -EAGAIN;
      }
      else if (ret < 0)
      {
        //fprintf(stderr, "Parser error: tokenizer error, curstate=%d\\n", curstate);
        //fprintf(stderr, "blk[off] = '%c'\\n", blk[off]);
        //abort();
        //exit(1);
        return -EINVAL;
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
        //fprintf(stderr, "Parser error: state mismatch %d %d\\n",
        //                (int)curstate, (int)state);
        //exit(1);
        return -EINVAL;
      }
      //printf("Got expected token %d\\n", (int)state);
      pctx->stacksz--;
    }
    else
    {
      uint8_t state;
      uint8_t curstateoff = curstate - """+parsername+"""_num_terminals;
      uint8_t ruleid;
      size_t i;
      const struct rule *rule;
      const struct state *restates = """+parsername+"""_parserstatetblentries[curstateoff].re;
      const uint8_t *cbs;
      cbs = """+parsername+"""_parserstatetblentries[curstateoff].cb;
      ret = """+parsername+"""_get_saved_token(pctx, restates, blk+off, sz-off, &state, cbs, 255, baton);
      if (ret == -EAGAIN)
      {
        off = sz;
        return -EAGAIN;
      }
      else if (ret < 0 || state == 255)
      {
        //fprintf(stderr, "Parser error: tokenizer error, curstate=%d, token=%d\\n", (int)curstate, (int)state);
        //exit(1);
        return -EINVAL;
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
      ruleid = """+parsername+"""_parserstatetblentries[curstateoff].rhs[state];
      rule = &"""+parsername+"""_rules[ruleid];
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
""", file=sio)
    pass
