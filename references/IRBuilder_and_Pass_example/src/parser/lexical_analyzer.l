%option noyywrap
%{
#include <stdio.h>
#include <stdlib.h>

#include "syntax_tree.h"
#include "syntax_analyzer.h"

int files_count;
int lines;
int pos_start;
int pos_end;
int counter;

void pass_node(char *text){
     yylval.node = new_syntax_tree_node(text);
}
%}

/***************
 TO STUDENTS: Copy your Lab1 here. Make adjustments if necessary.

 Note: don't modify the prologue unless you know what you are doing.
***************/

/* Example for you :-) */
/* \+  { pos_start = pos_end; pos_end += 1; pass_node(yytext); return ADD; } */

letter          [a-zA-Z]
digit           [0-9]
ID              {letter}+
INTEGER         {digit}+
FLOATPOINT      ({digit}+(\.)|{digit}*(\.){digit}+)
ARRAY           "[]"

EOL             "\n"
BLANK           [ \t\r\a]+

dict0           [^\*]
dict1           [^\*/]
string          (\*)+{dict1}{dict0}*
start           ("/\*")
end             ("\*/")
COMMENT         {start}{dict0}*{string}*(\*)*{end}
HALF_COMMENT    {start}{dict0}*{string}*(\*)*

%%

else    {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return ELSE;}
if      {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return IF;}
int     {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return INT;}
return  {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return RETURN;}
void    {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return VOID;}
while   {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return WHILE;}
float   {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return FLOAT;}

"*"     {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return MUL;}
"/"     {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return DIV;}
"+"     {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return ADD;}
"-"     {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return SUB;}
"<"     {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return LT;}
"<="    {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return LTE;}
">"     {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return GT;}
">="    {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return GTE;}
"=="    {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return EQ;}
"!="    {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return NEQ;}
"="     {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return ASSIN;}

";"     {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return SEMICOLON;}
","     {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return COMMA;}
"("     {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return LPARENTHESE;}
")"     {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return RPARENTHESE;}
"["     {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return LBRACKET;}
"]"     {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return RBRACKET;}
"{"     {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return LBRACE;}
"}"     {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return RBRACE;}

{ID}            {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return IDENTIFIER;}
{INTEGER}       {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return INTEGER;}
{FLOATPOINT}    {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return FLOATPOINT;}
{ARRAY}         {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return ARRAY;}

{EOL}           {lines++; pos_end = 1;}
{BLANK}         {pos_end += yyleng;}

{COMMENT}      {
                    counter = 0;
                    while (counter < yyleng){
                         if (*(yytext + counter) != '\n'){
                              pos_end++;
                         } else {
                              lines++;
                              pos_end = 1;
                         }
                         counter++;
                    }
                }
{HALF_COMMENT}  {
                    pos_start = pos_end;
                    if (*(yytext) == '/' && *(yytext + 1) == '*'){
                         counter = 0;
                         while (counter < yyleng){
                              if (*(yytext + counter) != '\n'){
                                   pos_end++;
                              } else {
                                   pos_end = 1;
                              }
                              counter++;
                         }
                    } else {
                         pos_end += yyleng;
                    }
                    fprintf(stderr, "error at line %d column %d: lexical error\n", lines, pos_start);
                    /*pos_start = pos_end;
                    pos_end += yyleng;*/
                    exit(1);
               }

.    {
          pos_start = pos_end;
          if (*(yytext) == '/' && *(yytext + 1) == '*'){
               counter = 0;
               while (counter < yyleng){
                    if (*(yytext + counter) != '\n'){
                         pos_end++;
                    } else {
                    pos_end = 1;
                    }
                    counter++;
               }
          } else {
               pos_end += yyleng;
          }
          fprintf(stderr, "error at line %d column %d: lexical error\n", lines, pos_start);
          /*pos_start = pos_end;
          pos_end += yyleng;*/
          exit(1);
     }

%%
