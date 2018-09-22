%option nounput noinput
%option prefix="yaleyy"

%{
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include "yale.tab.h"
#include "yyutils.h"

#define YYSTYPE YALEYYSTYPE
#define YYLTYPE YALEYYLTYPE

#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno; \
  yylloc->first_column = yycolumn; yylloc->last_column = yycolumn+yyleng-1; \
  yycolumn += yyleng;

struct CSnippet {
  char *data;
  size_t len;
  size_t capacity;
};

static inline void csadd(struct CSnippet *cs, char ch)
{
  if (cs->len + 1 >= cs->capacity)
  {
    size_t new_capacity = cs->capacity * 2 + 2;
    cs->data = realloc(cs->data, new_capacity);
    cs->capacity = new_capacity;
  }
  cs->data[cs->len] = ch;
  cs->data[cs->len + 1] = '\0';
  cs->len++;
}

%}

%option reentrant bison-bridge bison-locations
%x COMMENTS
%x CCOMMENTS
%x C
%x PERCENTCCOMMENTS
%x PERCENTC
%%

%{
int openBraceCount = 0;
struct CSnippet cSnippet = {
  .data = NULL,
  .len = 0,
  .capacity = 0,
};
%}

token     return TOKEN;
directive return DIRECTIVE;
main      return MAIN;
entry     return ENTRY;
bytes     return BYTES;
feed      return FEED;
reinit_feed return REINIT_FEED;
val       return VAL;
prio      return PRIO;
cb        return CB;
parsername return PARSERNAME;

uint8     return UINT8;
uint16be  return UINT16BE;
uint16le  return UINT16LE;
uint24be  return UINT24BE;
uint24le  return UINT24LE;
uint32be  return UINT32BE;
uint32le  return UINT32LE;
uint64be  return UINT64BE;
uint64le  return UINT64LE;


;         return SEMICOLON;
=         return EQUALS;
==        return EQUALSEQUALS;
[[]       return OPEN_BRACKET;
[]]       return CLOSE_BRACKET;
\(        return OPEN_PAREN;
\)        return CLOSE_PAREN;
\|        return PIPE;
\*        return ASTERISK;
\<        return LT;
\>        return GT;
\.        return PERIOD;
-         return MINUS;


[A-Za-z_][A-Za-z0-9_]* return FREEFORM_TOKEN;

\"([^\\\"]|\\.)*\"  yylval->s=yy_escape_string(yytext); return STRING_LITERAL;

0x[0-9A-Fa-f]+       {
  char *endptr;
  long l;
  errno = 0;
  l=strtol(yytext+2, &endptr, 16);
  if (errno == ERANGE)
  {
    return ERROR_TOK;
  }
  if (l > INT_MAX || l < INT_MIN)
  {
    return ERROR_TOK;
  }
  if (*endptr != '\0')
  {
    return ERROR_TOK;
  }
  yylval->i = l;
  return INT_LITERAL;
}

[0-9]+       {
  char *endptr;
  long l;
  errno = 0;
  l=strtol(yytext, &endptr, 10);
  if (errno == ERANGE)
  {
    return ERROR_TOK;
  }
  if (l > INT_MAX || l < INT_MIN)
  {
    return ERROR_TOK;
  }
  if (*endptr != '\0')
  {
    return ERROR_TOK;
  }
  yylval->i = l;
  return INT_LITERAL;
}

\$[0-9]+       {
  char *endptr;
  long l;
  errno = 0;
  l=strtol(yytext+1, &endptr, 10);
  if (errno == ERANGE)
  {
    return ERROR_TOK;
  }
  if (l > INT_MAX || l < INT_MIN)
  {
    return ERROR_TOK;
  }
  if (*endptr != '\0')
  {
    return ERROR_TOK;
  }
  yylval->i = l;
  return DOLLAR_LITERAL;
}

"{"              { BEGIN(C); openBraceCount++; cSnippet.capacity = 0; cSnippet.len = 0; cSnippet.data = NULL; }
"%{"             { BEGIN(PERCENTC); cSnippet.capacity = 0; cSnippet.len = 0; cSnippet.data = NULL; }
<C>"{"           { csadd(&cSnippet, yytext[0]); openBraceCount++; }
<C>"/*"          BEGIN(CCOMMENTS);
<C>\/\/.*        /* ignore single-line comments */;
<CCOMMENTS>"*/"  BEGIN(C);
<CCOMMENTS>"/*"  return ERROR_TOK;
<CCOMMENTS><<EOF>> return ERROR_TOK;
<CCOMMENTS>.     ;
<CCOMMENTS>"\n"  { ++yylineno; yycolumn=1; }
<C>\}            {
                   openBraceCount--;
                   if (!openBraceCount) {
                     BEGIN(INITIAL);
                     yylval->s = cSnippet.data;
                     cSnippet.data = NULL;
                     cSnippet.capacity = 0;
                     cSnippet.len = 0;
                     return C_LITERAL;
                   }
                   else {
                     csadd(&cSnippet, yytext[0]);
                   }
                 }
<C>.             { csadd(&cSnippet, yytext[0]); }
<C>"\n"          { csadd(&cSnippet, yytext[0]); ++yylineno; yycolumn=1; }
<PERCENTC>"/*"          BEGIN(PERCENTCCOMMENTS);
<PERCENTC>\/\/.*        /* ignore single-line comments */;
<PERCENTCCOMMENTS>"*/"  BEGIN(PERCENTC);
<PERCENTCCOMMENTS>"/*"  return ERROR_TOK;
<PERCENTCCOMMENTS><<EOF>> return ERROR_TOK;
<PERCENTCCOMMENTS>.     ;
<PERCENTCCOMMENTS>"\n"  { ++yylineno; yycolumn=1; }
<PERCENTC>%\}           {
                     BEGIN(INITIAL);
                     yylval->s = cSnippet.data;
                     cSnippet.data = NULL;
                     cSnippet.capacity = 0;
                     cSnippet.len = 0;
                     return PERCENTC_LITERAL;
                 }
<PERCENTC>.             { csadd(&cSnippet, yytext[0]); }
<PERCENTC>"\n"          { csadd(&cSnippet, yytext[0]); ++yylineno; yycolumn=1; }

\n                                { ++yylineno; yycolumn=1; }
[ \t]+                            /* ignore whitespaces */;
\/\/.*                            /* ignore single-line comments */;
.            return ERROR_TOK;

"/*"                  BEGIN(COMMENTS);
<COMMENTS>"/*"         return ERROR_TOK;
<COMMENTS>"*/"         BEGIN(INITIAL);
<COMMENTS><<EOF>>      return ERROR_TOK;
<COMMENTS>.          ;
<COMMENTS>"\n"       { ++yylineno; yycolumn=1; }


%%
