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

\{        return OPEN_BRACE;
\}        return CLOSE_BRACE;
;         return SEMICOLON;
=         return EQUALS;
[[]       return OPEN_BRACKET;
[]]       return CLOSE_BRACKET;
\(        return OPEN_PAREN;
\)         return CLOSE_PAREN;
\|        return PIPE;
\*        return ASTERISK;

[A-Za-z_][A-Za-z0-9_]* return FREEFORM_TOKEN;

\"([^\\\"]|\\.)*\"  yylval->s=yy_escape_string(yytext); return STRING_LITERAL;

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
