%code requires {
#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void *yyscan_t;
#endif
#include "yale.h"
#include <sys/types.h>
}

%define api.prefix {yaleyy}

%{

#include "yale.h"
#include "yyutils.h"
#include "yale.tab.h"
#include "yale.lex.h"
#include <arpa/inet.h>

void yaleyyerror(YYLTYPE *yylloc, yyscan_t scanner, struct yale *yale, const char *str)
{
        fprintf(stderr, "error: %s at line %d col %d\n",str, yylloc->first_line, yylloc->first_column);
}

int yaleyywrap(yyscan_t scanner)
{
        return 1;
}

%}

%pure-parser
%lex-param {yyscan_t scanner}
%parse-param {yyscan_t scanner}
%parse-param {struct yale *yale}
%locations

%union {
  int i;
  char *s;
  struct {
    int i;
    char *s;
  } both;
}

%destructor { free ($$); } STRING_LITERAL

%token TOKEN
%token DIRECTIVE
%token MAIN
%token EQUALS
%token SEMICOLON
%token STRING_LITERAL
%token INT_LITERAL
%token FREEFORM_TOKEN
%token ASTERISK
%token OPEN_PAREN
%token CLOSE_PAREN
%token OPEN_BRACKET
%token CLOSE_BRACKET
%token OPEN_BRACE
%token CLOSE_BRACE
%token PIPE

%token ERROR_TOK

%type<i> INT_LITERAL
%type<s> STRING_LITERAL

%%

yalerules:
| yalerules yalerule
;

yalerule:
  TOKEN FREEFORM_TOKEN EQUALS STRING_LITERAL SEMICOLON
{
  free($4);
}
| FREEFORM_TOKEN EQUALS elements SEMICOLON
| DIRECTIVE MAIN EQUALS FREEFORM_TOKEN SEMICOLON
;

elements:
alternation
;

alternation:
concatenation
maybe_alternationlist
;

maybe_alternationlist:
| maybe_alternationlist PIPE concatenation
;

concatenation:
repetition
maybe_concatenationlist
;

maybe_concatenationlist:
| maybe_concatenationlist repetition
;

repetition:
maybe_repeat
element
;

maybe_repeat:
| INT_LITERAL
| INT_LITERAL ASTERISK INT_LITERAL
| ASTERISK INT_LITERAL
| INT_LITERAL ASTERISK
| ASTERISK
;

element:
FREEFORM_TOKEN
| group
| option
;

group:
OPEN_PAREN alternation CLOSE_PAREN
;

option:
OPEN_BRACKET alternation CLOSE_BRACKET
;
