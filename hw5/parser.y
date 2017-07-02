%{
// Dummy parser for scanner project.

#include <cassert>

#include "lyutils.h"
#include "astree.h"

%}

%debug
%defines
%error-verbose
%token-table
%verbose



%printer { astree::dump (yyoutput, $$); } <>

%initial-action {
   parser::root = new astree (TOK_ROOT, {0, 0, 0}, "<<ROOT>>");
}


%token TOK_VOID TOK_CHAR TOK_INT TOK_STRING
%token TOK_IF TOK_ELSE TOK_WHILE TOK_RETURN TOK_STRUCT TOK_RETURNVOID
%token TOK_NULL TOK_NEW TOK_ARRAY
%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%token TOK_IDENT TOK_INTCON TOK_CHARCON 
%token TOK_STRINGCON TOK_DECLID TOK_INDEX

%token TOK_BLOCK TOK_CALL TOK_IFELSE TOK_INITDECL
%token TOK_POS TOK_NEG TOK_NEWARRAY TOK_TYPEID TOK_FIELD TOK_NEWSTRING
%token TOK_ROOT

%token TOK_PROTOTYPE TOK_FUNCTION TOK_PARAMLIST TOK_VARDECL

%right TOK_IF TOK_ELSE
%right '='
%left TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%left '+' '-'
%left '*' '/' '%'
%right TOK_POS TOK_NEG '!' TOK_NEW
%left '[' '.' TOK_CALL
%nonassoc '('

%start start

%%

start       : program                   { $$ = $1; }

program     : program structdef         { $$ = $1->adopt ($2); }
            | program function          { $$ = $1->adopt ($2); }
            | program statement         { $$ = $1->adopt ($2); }
            | program error '}'         { destroy ($3); $$ = $1; }
            | program error ';'         { destroy ($3); $$ = $1; }
            |                           { $$ = parser::root; }
            ;

structdef   : TOK_STRUCT TOK_IDENT 
                '{' '}'                 { $2->symbol = TOK_TYPEID; 
                                          $$ = $1->adopt($2); 
                                          destroy($3, $4); }
            | fields '}'                { destroy($2); $$ = $1; }
            ;

fields      : TOK_STRUCT TOK_IDENT 
                '{' fielddecl ';'       { $2->symbol = TOK_TYPEID; 
                                          destroy($3, $5); 
                                          $$ = $1->adopt($2, $4); }
            | fields fielddecl ';'      { destroy($3); 
                                          $$ = $1->adopt($2); }

fielddecl   : basetype TOK_ARRAY 
                TOK_IDENT               { $3->symbol = TOK_FIELD; 
                                          $$ = $2->adopt($1, $3); }
            | basetype TOK_IDENT        { $2->symbol = TOK_FIELD; 
                                          $$ = $1->adopt($2); }
            ;

function    : identdecl '(' ')' block   { $2->symbol = TOK_PARAMLIST;
                                          destroy($3); 
                                          $$ = adopt_func($1, $2, $4); }

            | identdecl funcargs 
                ')' block               { $2->symbol = TOK_PARAMLIST;
                                          destroy($3); 
                                          $$ = adopt_func($1, $2, $4); }

funcargs    : '(' identdecl             { $$ = $1->adopt($2); }
            | funcargs ',' identdecl    { destroy($2); 
                                          $$ = $1->adopt($3); }

statement   : block                     { $$ = $1; }
            | vardecl                   { $$ = $1; }
            | while                     { $$ = $1; }
            | ifelse                    { $$ = $1; }
            | return                    { $$ = $1; }
            | expr ';'                  { $$ = $1; destroy($2); }
            ;

block       : ';'                       { $1->symbol = TOK_BLOCK; 
                                          $$ = $1; }
            | '{' '}'                   { destroy($2); 
                                          $1->symbol = TOK_BLOCK; 
                                          $$ = $1; }
            | statements '}'            { destroy($2); 
                                          $1->symbol = TOK_BLOCK; 
                                          $$ = $1; }

statements  : '{' statement             { $$ = $1->adopt($2); }
            | statements statement      { $$ = $1->adopt($2); }
            ;

vardecl     : identdecl '=' expr ';'    { $2->symbol = TOK_VARDECL;
                                          $$ = $2->adopt($1, $3);
                                          destroy($4); }
            ;

identdecl   : basetype TOK_ARRAY 
                TOK_IDENT               { $3->symbol = TOK_DECLID;
                                          $$ = $2->adopt($1, $3); }
            | basetype TOK_IDENT        { $2->symbol = TOK_DECLID;
                                          $$ = $1->adopt($2); }
            ;

while       : TOK_WHILE 
              '(' expr ')' statement    { $$ = $1->adopt($3, $5);
                                          destroy($2, $4); }
            ;

ifelse      : if statement 
                TOK_ELSE statement      { $1->symbol = TOK_IFELSE;
                                          $$ = $1->adopt($2, $4); 
                                          destroy($3); }
            | if statement 
                %prec TOK_ELSE          { $$ = $1->adopt($2); }

if          : TOK_IF '(' expr ')'       { $$ = $1->adopt($3);
                                          destroy($2, $4); }

return      : TOK_RETURN ';'            { $1->symbol = TOK_RETURNVOID;
                                          $$ = $1; destroy($2); }
            | TOK_RETURN expr ';'       { $$ = $1->adopt($2);
                                          destroy($3); }
            ;

expr        : expr '='    expr          { $$ = $2->adopt($1, $3); }
            | expr TOK_EQ expr          { $$ = $2->adopt($1, $3); }
            | expr TOK_NE expr          { $$ = $2->adopt($1, $3); }
            | expr TOK_LT expr          { $$ = $2->adopt($1, $3); }
            | expr TOK_LE expr          { $$ = $2->adopt($1, $3); }
            | expr TOK_GT expr          { $$ = $2->adopt($1, $3); }
            | expr TOK_GE expr          { $$ = $2->adopt($1, $3); }
            | expr '+'    expr          { $$ = $2->adopt($1, $3); }
            | expr '-'    expr          { $$ = $2->adopt($1, $3); }
            | expr '*'    expr          { $$ = $2->adopt($1, $3); }
            | expr '/'    expr          { $$ = $2->adopt($1, $3); }
            | expr '%'    expr          { $$ = $2->adopt($1, $3); }
            | '+' expr                  { $1->symbol = TOK_POS;
                                          $$ = $1->adopt($2); }
            | '-' expr                  { $1->symbol = TOK_NEG; 
                                          $$ = $1->adopt($2); }
            | '!' expr                  { $$ = $1->adopt($2); }
            | allocator                 { $$ = $1; }
            | call                      { $$ = $1; }
            | '(' expr ')'              { $$ = $2; destroy($1, $3); }
            | variable                  { $$ = $1; } 
            | constant                  { $$ = $1; }
            ;

call        : TOK_IDENT '(' ')'         { destroy($3); 
                                          $2->symbol = TOK_CALL;
                                          $$ = $2->adopt($1); }

            | TOK_IDENT callargs ')'    { destroy($3);
                                          $2->symbol = TOK_CALL;
                                          $$ = $2->adopt_two($1); }

callargs    : '(' expr                  { $$ = $1->adopt($2); }
            | callargs ',' expr         { destroy($2); 
                                          $$ = $1->adopt($3); }

allocator   : TOK_NEW TOK_IDENT 
                '(' ')'                 { $2->symbol = TOK_TYPEID;
                                          destroy($3, $4); 
                                          $$ = $1->adopt($2); }
            | TOK_NEW TOK_STRING 
                '(' expr ')'            { $1->symbol = TOK_NEWSTRING; 
                                          destroy($2, $3); 
                                          destroy($5); 
                                          $$ = $1->adopt($4); }
            | TOK_NEW basetype 
                '[' expr ']'            { $1->symbol = TOK_NEWARRAY;
                                          $$ = $1->adopt($2, $4);
                                          destroy($3, $5); }

variable    : TOK_IDENT                 { $$ = $1; }
            | expr '[' expr ']'         { $2->symbol = TOK_INDEX;
                                          $$ = $2->adopt($1, $3); 
                                          destroy($4); }
            | expr '.' TOK_IDENT        { $3->symbol = TOK_FIELD; 
                                          $$ = $2->adopt($1, $3); }
            ;

constant    : TOK_INTCON                { $$ = $1; }
            | TOK_CHARCON               { $$ = $1; }
            | TOK_STRINGCON             { $$ = $1; }
            | TOK_NULL                  { $$ = $1; }
            ;

basetype    : TOK_VOID                  { $$ = $1; }
            | TOK_INT                   { $$ = $1; }
            | TOK_STRING                { $$ = $1; }
            | TOK_IDENT                 { $1->symbol = TOK_TYPEID; 
                                          $$ = $1; }
            ;

%%


const char *parser::get_tname (int symbol) {
   return yytname [YYTRANSLATE (symbol)];
}


bool is_defined_token (int symbol) {
   return YYTRANSLATE (symbol) > YYUNDEFTOK;
}

/*
static void* yycalloc (size_t size) {
   void* result = calloc (1, size);
   assert (result != nullptr);
   return result;
}
*/
