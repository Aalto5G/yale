import regex
import parser

p = parser.ParserGen("http")

hosttoken = p.add_token("[Hh][Oo][Ss][Tt]", priority=1)
crlf      = p.add_token("\r\n")
onespace  = p.add_token(" ")
httpname  = p.add_token("HTTP")
slash     = p.add_token("/")
digit     = p.add_token("[0-9]")
colon     = p.add_token(":")
optspace  = p.add_token("[ \t]*")
httptoken = p.add_token("[-!#$%&'*+.^_`|~0-9A-Za-z]+")
httpfield = p.add_token("[\t\x20-\x7E\x80-\xFF]*")
period    = p.add_token(".") # FIXME is period interpreted as "any character?"
uri       = p.add_token("[]:/?#@!$&'()*+,;=0-9A-Za-z._~%[-]+")
foldstart = p.add_token("\r\n[ \t]+")

p.finalize_tokens()
epsilon = p.epsilon
eof = p.eof

headerField        = p.add_nonterminal()
version            = p.add_nonterminal()
requestLine        = p.add_nonterminal()
requestHdrs        = p.add_nonterminal()
requestWithHeaders = p.add_nonterminal()
httpFoldField      = p.add_nonterminal()
httpFoldFieldEnd   = p.add_nonterminal()
hostFoldField      = p.add_nonterminal()
hostFoldFieldEnd   = p.add_nonterminal()

S = p.start_state(requestWithHeaders)

p.set_rules([
  (hostFoldField, [parser.WrapCB(p, httpfield, "print"), hostFoldFieldEnd]),
  (hostFoldFieldEnd, []),
  (hostFoldFieldEnd, [foldstart, parser.Action("printsp"),
                      parser.WrapCB(p, httpfield, "print"),
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
])
p.gen_parser()


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
p.parse(myreq)

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
  p.parse(myreqshort)
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
  p.parse(myreqlong)
except:
  ok = True
assert ok

myreqtrivial = [
  httptoken, onespace, uri, onespace,
    httpname, slash, digit, period, digit, crlf,
  crlf]
p.parse(myreqtrivial)

p.print_parser()

#print """
#int main(int argc, char **argv)
#{
#  char *input = "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
#                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
#                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
#                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
#                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
#                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
#                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
#                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
#                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
#                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ" // 500
#                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
#                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
#                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
#                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
#                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
#                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
#                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
#                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ" 
#                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ"
#                "ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHI "; // 1000
#
#  ssize_t consumed;
#  uint8_t state = 255;
#  struct rectx ctx = {};
#  size_t i;
#  struct """+p.parsername+"""_parserctx pctx = {};
#  char *http = "GET / HTTP/1.1\\r\\nHost: localhost\\r\\n\\r\\n";
#  char *httpa = "GET / HTTP/1.1\\r\\nHost: localhost\\r\\n\\r\\na";
#
#
#  for (i = 0; i < /* 1000* */ 1000 * 0; i++)
#  {
#    init_statemachine(&ctx);
#    consumed = feed_statemachine(&ctx, states_8, input, strlen(input), &state);
#    if (consumed != 999 || state != 8)
#    {
#      abort();
#    }
#    //printf("Consumed %zd state %d\\n", consumed, (int)state);
#  }
#
#  """+p.parsername+"""_parserctx_init(&pctx);
#  consumed = """+p.parsername+"""_parse_block(&pctx, http, strlen(http));
#  printf("Consumed %zd stack %d\\n", consumed, (int)pctx.stacksz);
#
#  """+p.parsername+"""_parserctx_init(&pctx);
#  consumed = """+p.parsername+"""_parse_block(&pctx, httpa, strlen(httpa));

print """
int main(int argc, char **argv)
{
  ssize_t consumed;
  uint8_t state = 255;
  size_t i;
  struct """+p.parsername+"""_parserctx pctx = {};
  char http[] =
    "GET /foo/bar/baz/barf/quux.html HTTP/1.1\\r\\n"
    "Host: www.google.fi\\r\\n"
    //"\twww.google2.fi\\r\\n"
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

  for (i = 0; i < 1000 * 1000 /* 1 */; i++)
  {
    """+p.parsername+"""_parserctx_init(&pctx);
    consumed = """+p.parsername+"""_parse_block(&pctx, http, sizeof(http)-1);
    if (consumed != sizeof(http)-1)
    {
      abort();
    }
  }

  return 0;
}
"""
