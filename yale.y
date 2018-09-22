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
%destructor { free ($$); } FREEFORM_TOKEN
%destructor { free ($$); } C_LITERAL
%destructor { free ($$); } PERCENTC_LITERAL

%token C_LITERAL
%token PERCENTC_LITERAL

%token TOKEN
%token ACTION
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
%type<i> maybe_prio
%type<i> maybe_minus
%type<i> token_ltgtexp
%type<i> maybe_token_ltgt
%type<s> STRING_LITERAL
%type<s> FREEFORM_TOKEN
%type<s> C_LITERAL
%type<s> PERCENTC_LITERAL

%%

yalerules:
| yalerules yalerule
;

yalerule:
  TOKEN maybe_prio FREEFORM_TOKEN EQUALS STRING_LITERAL SEMICOLON
{
  struct token *tk;
  uint8_t i;
  if (yale->tokencnt >= sizeof(yale->tokens)/sizeof(*yale->tokens))
  {
    printf("1\n");
    YYABORT;
  }
  for (i = 0; i < yale->nscnt; i++)
  {
    if (strcmp(yale->ns[i].name, $3) == 0)
    {
      yale->ns[i].is_token = 1;
      if (yale->ns[i].is_lhs)
      {
        printf("1.1\n");
        YYABORT;
      } 
      free($3);
      break;
    }
  }
  if (i == yale->nscnt)
  {
    if (i == 255)
    {
      printf("1.2\n");
      YYABORT;
    }
    yale->ns[i].name = $3;
    yale->ns[i].is_token = 1;
    yale->nscnt++;
  }
  tk = &yale->tokens[yale->tokencnt++];
  tk->priority = $2;
  tk->nsitem = i;
  tk->re = $5;
}
| FREEFORM_TOKEN EQUALS
{
  struct rule *rule;
  uint8_t i;
  if (yale->rulecnt >= sizeof(yale->rules)/sizeof(*yale->rules))
  {
    printf("3\n");
    YYABORT;
  }
  rule = &yale->rules[yale->rulecnt++];

  for (i = 0; i < yale->nscnt; i++)
  {
    if (strcmp(yale->ns[i].name, $1) == 0)
    {
      yale->ns[i].is_lhs = 1;
      if (yale->ns[i].is_token)
      {
        printf("3.1 is_token %s %d\n", $1, i);
        YYABORT;
      } 
      free($1);
      break;
    }
  }
  if (i == yale->nscnt)
  {
    if (i == 255)
    {
      printf("3.2\n");
      YYABORT;
    }
    yale->ns[i].name = $1;
    yale->ns[i].is_lhs = 1;
    yale->nscnt++;
  }
  rule->lhs = i;
}
elements SEMICOLON
| DIRECTIVE directive_continued
| PERCENTC_LITERAL
{
  csaddstr(&yale->cs, $1);
  free($1);
};
;

maybe_prio:
{
  $$ = 0;
}
| LT PRIO EQUALS maybe_minus INT_LITERAL GT
{
  $$ = $4 * $5;
}
;

maybe_minus:
{
  $$ = +1;
}
| MINUS
{
  $$ = -1;
}
;


directive_continued:
MAIN EQUALS FREEFORM_TOKEN SEMICOLON
{
  uint8_t i;
  for (i = 0; i < yale->nscnt; i++)
  {
    if (strcmp(yale->ns[i].name, $3) == 0)
    {
      yale->ns[i].is_lhs = 1;
      if (yale->ns[i].is_token)
      {
        printf("M.1\n");
        YYABORT;
      } 
      free($3);
      break;
    }
  }
  if (i == yale->nscnt)
  {
    if (i == 255)
    {
      printf("M.2\n");
      YYABORT;
    }
    yale->ns[i].is_lhs = 1;
    yale->ns[i].name = $3;
    yale->nscnt++;
  }
  yale->startns = i;
  yale->startns_present = 1;
}
| ENTRY EQUALS FREEFORM_TOKEN SEMICOLON
{
  free($3);
}
| PARSERNAME EQUALS FREEFORM_TOKEN SEMICOLON
{
  yale->parsername = $3;
}
;

elements:
alternation
;

alternation:
| concatenation
| alternation PIPE
{
  struct rule *rule;
  if (yale->rulecnt >= sizeof(yale->rules)/sizeof(*yale->rules))
  {
    printf("6\n");
    YYABORT;
  }
  rule = &yale->rules[yale->rulecnt];
  rule->lhs = yale->rules[yale->rulecnt-1].lhs;
  yale->rulecnt++;
}
concatenation
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
ACTION maybe_token_ltgt
{
  struct rule *rule;
  struct ruleitem *it;
  rule = &yale->rules[yale->rulecnt - 1];
  if (rule->itemcnt == 255)
  {
    printf("7\n");
    YYABORT;
  }
  it = &rule->rhs[rule->itemcnt++];
  it->is_action = 1;
  it->value = 255;
  it->cb = $2;
}
| FREEFORM_TOKEN maybe_token_ltgt
{
  struct rule *rule;
  struct ruleitem *it;
  uint8_t i;
  rule = &yale->rules[yale->rulecnt - 1];
  if (rule->itemcnt == 255)
  {
    printf("7\n");
    YYABORT;
  }
  it = &rule->rhs[rule->itemcnt++];
  for (i = 0; i < yale->nscnt; i++) // FIXME check all cnt uses
  {
    if (strcmp(yale->ns[i].name, $1) == 0)
    {
      break;
    }
  }
  if (i != yale->nscnt)
  {
    it->value = i;
    it->cb = $2;
    if ($2 != 255 && yale->ns[i].is_lhs)
    {
      printf("7.1\n");
      YYABORT;
    }
  }
  else
  {
    if (i == 255)
    {
      printf("7.2\n");
      YYABORT;
    }
    yale->ns[i].name = strdup($1);
    it->value = i;
    it->cb = $2;
    yale->nscnt++;
  }
  free($1);
}
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
{
  $$ = 255;
}
| LT token_ltgtexp GT
{
  $$ = $2;
}
;

token_ltgtexp:
VAL EQUALSEQUALS valstr_literal
{
  $$ = 255;
}
| CB EQUALS FREEFORM_TOKEN
{
  uint8_t i;
  for (i = 0; i < yale->cbcnt; i++)
  {
    if (strcmp(yale->cbs[i].name, $3) == 0)
    {
      free($3);
      break;
    }
  }
  if (i == yale->cbcnt)
  {
    if (i == 255)
    {
      printf("9\n");
      YYABORT;
    }
    yale->cbs[i].name = $3;
    yale->cbcnt++;
  }
  $$ = i;
}
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
{
  free($3);
}
| REINIT_FEED EQUALS FREEFORM_TOKEN
{
  free($3);
}
| CB EQUALS FREEFORM_TOKEN
{
  free($3);
}
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
