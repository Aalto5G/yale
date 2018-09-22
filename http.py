import regex
import parser
import sys

p = parser.ParserGen("http")

hosttoken = p.add_token("[Hh][Oo][Ss][Tt]", priority=1)
crlf      = p.add_token("\r?\n")
onespace  = p.add_token(" ")
#colon     = p.add_token(":")
#optspace  = p.add_token("[ \t]*")
colonosp  = p.add_token(":[ \t]*")
httptoken = p.add_token("[-!#$%&'*+.^_`|~0-9A-Za-z]+")
httpfield = p.add_token("[\t\x20-\x7E\x80-\xFF]*")
spvcrlf   = p.add_token(" HTTP/[0-9]+[.][0-9]+\r?\n")
uri       = p.add_token("[]:/?#@!$&'()*+,;=0-9A-Za-z._~%[-]+")
foldstart = p.add_token("\r?\n[ \t]+")

p.finalize_tokens()

headerField        = p.add_nonterminal()
requestLine        = p.add_nonterminal()
requestHdrs        = p.add_nonterminal()
requestWithHeaders = p.add_nonterminal()
httpFoldField      = p.add_nonterminal()
httpFoldFieldEnd   = p.add_nonterminal()
hostFoldField      = p.add_nonterminal()
hostFoldFieldEnd   = p.add_nonterminal()

p.start_state(requestWithHeaders)

p.set_rules([
  (hostFoldField, [p.wrapCB(httpfield, "print"), hostFoldFieldEnd]),
  (hostFoldFieldEnd, []),
  (hostFoldFieldEnd, [foldstart, p.action("printsp"),
                      p.wrapCB(httpfield, "print"),
                      hostFoldFieldEnd]),
  (httpFoldField, [httpfield, httpFoldFieldEnd]),
  (httpFoldFieldEnd, []),
  (httpFoldFieldEnd, [foldstart, httpfield, httpFoldFieldEnd]),
  #(headerField, [hosttoken, colon, optspace, hostFoldField]),
  #(headerField, [httptoken, colon, optspace, httpFoldField]),
  (headerField, [hosttoken, colonosp, hostFoldField]),
  (headerField, [httptoken, colonosp, httpFoldField]),
  (requestLine, [httptoken, onespace, uri, spvcrlf]),
  (requestHdrs, []),
  (requestHdrs, [headerField, crlf, requestHdrs]),
  (requestWithHeaders, [requestLine, requestHdrs, crlf]),
])
p.gen_parser()


#myreq = [
#  httptoken, onespace, uri, onespace,
#    httpname, slash, digit, period, digit, crlf,
#  httptoken, colon, optspace, httpfield, crlf,
#  hosttoken, colon, optspace, httpfield, crlf,
#  httptoken, colon, optspace, httpfield,# crlf,
#    foldstart, httpfield, #crlf,
#    foldstart, httpfield, crlf,
#  httptoken, colon, optspace, httpfield, #crlf,
#    foldstart, httpfield, crlf,
#  httptoken, colon, optspace, httpfield, crlf,
#  crlf]
#p.parse(myreq)
#
#myreqshort = [
#  httptoken, onespace, uri, onespace,
#    httpname, slash, digit, period, digit, crlf,
#  httptoken, colon, optspace, httpfield, crlf,
#  hosttoken, colon, optspace, httpfield, crlf,
#  httptoken, colon, optspace, httpfield, #crlf,
#    foldstart, httpfield, #crlf,
#    foldstart, httpfield, crlf,
#  httptoken, colon, optspace, httpfield, #crlf,
#    foldstart, httpfield, crlf,
#  httptoken, colon, optspace, httpfield, crlf]
#ok = False
#try:
#  p.parse(myreqshort)
#except:
#  ok = True
#assert ok
#
#myreqlong = [
#  httptoken, onespace, uri, onespace,
#    httpname, slash, digit, period, digit, crlf,
#  httptoken, colon, optspace, httpfield, crlf,
#  hosttoken, colon, optspace, httpfield, crlf,
#  httptoken, colon, optspace, httpfield, #crlf,
#    foldstart, httpfield, #crlf,
#    foldstart, httpfield, crlf,
#  httptoken, colon, optspace, httpfield, #crlf,
#    foldstart, httpfield, crlf,
#  httptoken, colon, optspace, httpfield, crlf,
#  crlf,
#  crlf]
#ok = False
#try:
#  p.parse(myreqlong)
#except:
#  ok = True
#assert ok
#
#myreqtrivial = [
#  httptoken, onespace, uri, onespace,
#    httpname, slash, digit, period, digit, crlf,
#  crlf]
#p.parse(myreqtrivial)

if sys.argv[1] == "h":
  with open('httpparser.h', 'w') as fd:
    print >> fd, "#ifndef _HTTPPARSER_H_"
    print >> fd, "#define _HTTPPARSER_H_"
    p.print_headers(fd)
    print >> fd, "#endif"
elif sys.argv[1] == "c":
  with open('httpparser.c', 'w') as fd:
    print >> fd, "#include \"httpcommon.h\""
    print >> fd, "#include \"httpparser.h\""
    p.print_parser(fd)
