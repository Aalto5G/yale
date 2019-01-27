%option nounput noinput
%option prefix="yaleyy"

%{
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include "yyutils.h"
#include "yale.tab.h"

#define YYSTYPE YALEYYSTYPE
#define YYLTYPE YALEYYLTYPE

#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno; \
  yylloc->first_column = yycolumn; yylloc->last_column = yycolumn+yyleng-1; \
  yycolumn += yyleng;

%}

%option reentrant bison-bridge bison-locations
%x COMMENTS
%x PERCENTCCOMMENTS
%x PERCENTC
%%

%{
struct CSnippet cSnippet = {
  .data = NULL,
  .len = 0,
  .capacity = 0,
};
%}

i         return I;
bytessizetype return BYTESSIZETYPE;
stateinclude return STATEINCLUDE;
parserinclude return PARSERINCLUDE;
initinclude return INITINCLUDE;
hdrinclude return HDRINCLUDE;
action    return ACTION;
token     return TOKEN;
directive return DIRECTIVE;
nofastpath return NOFASTPATH;
shortcutting return SHORTCUTTING;
main      return MAIN;
bytes     return BYTES;
prio      return PRIO;
cb        return CB;
parsername return PARSERNAME;
cond      return COND;


;         return SEMICOLON;
,         return COMMA;
=         return EQUALS;
\|        return PIPE;
\<        return LT;
\>        return GT;
-         return MINUS;


[A-Za-z_][A-Za-z0-9_]* {
  yylval->s = strdup(yytext);
  return FREEFORM_TOKEN;
}

\"([^\\\"]|\\.)*\"  yylval->str=yy_escape_string(yytext); return STRING_LITERAL;

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

"%{"             { BEGIN(PERCENTC); cSnippet.capacity = 0; cSnippet.len = 0; cSnippet.data = NULL; }
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
