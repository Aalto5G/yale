%option nounput noinput
%option prefix="yaleyy"

%{
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

%}

%option reentrant bison-bridge bison-locations
%x COMMENTS
%%

token     return TOKEN;
directive return DIRECTIVE;
main      return MAIN;
entry     return ENTRY;
bytes     return BYTES;
print     return PRINT;
discard   return DISCARD;
feed      return FEED;
reinit_feed return REINIT_FEED;
val       return VAL;

uint8     return UINT8;
uint16be  return UINT16BE;
uint16le  return UINT16LE;
uint24be  return UINT24BE;
uint24le  return UINT24LE;
uint32be  return UINT32BE;
uint32le  return UINT32LE;
uint64be  return UINT64BE;
uint64le  return UINT64LE;


\{        return OPEN_BRACE;
\}        return CLOSE_BRACE;
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
:         return COLON;


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
