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
%destructor { free ($$); } C_LITERAL
%destructor { free ($$); } PERCENTC_LITERAL

%token C_LITERAL
%token PERCENTC_LITERAL

%token TOKEN
%token PRIO
%token DIRECTIVE
%token MAIN
%token ENTRY

%token BYTES
%token FEED
%token REINIT_FEED

%token PARSERNAME
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
%token LT
%token GT
%token PIPE
%token DOLLAR_LITERAL
%token MINUS
%token CB

%token UINT8
%token UINT16BE
%token UINT16LE
%token UINT24BE
%token UINT24LE
%token UINT32BE
%token UINT32LE
%token UINT64BE
%token UINT64LE


%token VAL
%token EQUALSEQUALS
%token PERIOD


%token ERROR_TOK

%type<i> INT_LITERAL
%type<s> STRING_LITERAL
%type<s> C_LITERAL
%type<s> PERCENTC_LITERAL

%%

yalerules:
| yalerules yalerule
;

yalerule:
  TOKEN maybe_prio FREEFORM_TOKEN EQUALS STRING_LITERAL SEMICOLON
{
  free($5);
}
| FREEFORM_TOKEN EQUALS elements SEMICOLON
| DIRECTIVE directive_continued
| PERCENTC_LITERAL
{
  csaddstr(&yale->cs, $1);
  free($1);
};
;

maybe_prio:
| LT PRIO EQUALS maybe_minus INT_LITERAL GT
;

maybe_minus:
| MINUS;


directive_continued:
MAIN EQUALS FREEFORM_TOKEN SEMICOLON
| ENTRY EQUALS FREEFORM_TOKEN SEMICOLON
| PARSERNAME EQUALS FREEFORM_TOKEN SEMICOLON
;

elements:
alternation
;

alternation:
| concatenation
| alternation PIPE concatenation
;

/*
alternation:
concatenation
maybe_alternationlist
;

maybe_alternationlist:
| maybe_alternationlist PIPE concatenation
;
*/

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
maybe_c_literal
;

maybe_c_literal:
| C_LITERAL
{
  printf("%s\n", $1);
  free($1);
};

maybe_repeat:
| INT_LITERAL
| DOLLAR_LITERAL
| INT_LITERAL ASTERISK INT_LITERAL
| ASTERISK INT_LITERAL
| INT_LITERAL ASTERISK
| ASTERISK
;

element:
FREEFORM_TOKEN maybe_token_ltgt
| uint_token
| BYTES maybe_bytes_ltgt
| group
| option
;

uint_token:
uint_token_raw
maybe_uint_ltgt
;

maybe_uint_ltgt:
| LT uint_ltgtexp GT
;

maybe_token_ltgt:
| LT token_ltgtexp GT
;

token_ltgtexp:
VAL EQUALSEQUALS valstr_literal
| CB EQUALS FREEFORM_TOKEN
;

uint_ltgtexp:
VAL EQUALSEQUALS val_literal
;

valstr_literal:
PERIOD
| STRING_LITERAL
{
  free($1);
}
;

val_literal:
PERIOD
| INT_LITERAL
;

maybe_bytes_ltgt:
| LT bytes_ltgtexp GT
;

bytes_ltgtexp:
  FEED EQUALS FREEFORM_TOKEN
| REINIT_FEED EQUALS FREEFORM_TOKEN
| CB EQUALS FREEFORM_TOKEN
;

uint_token_raw:
UINT8
| UINT16BE
| UINT16LE
| UINT24BE
| UINT24LE
| UINT32BE
| UINT32LE
| UINT64BE
| UINT64LE
;

group:
OPEN_PAREN alternation CLOSE_PAREN
;

option:
OPEN_BRACKET alternation CLOSE_BRACKET
;
