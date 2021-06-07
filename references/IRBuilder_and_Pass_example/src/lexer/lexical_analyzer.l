%option noyywrap
%{
/*****************声明和选项设置  begin*****************/
#include <stdio.h>
#include <stdlib.h>

#include "lexical_analyzer.h"

int lines;
int pos_start;
int pos_end;

/*****************声明和选项设置  end*****************/

%}

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

 /******************TODO*********************/
 /****请在此补全所有flex的模式与动作  start******/
 //STUDENT TO DO

else    {return ELSE;}
if      {return IF;}
int     {return INT;}
return  {return RETURN;}
void    {return VOID;}
while   {return WHILE;}
float   {return FLOAT;}

"*"     {return MUL;}
"/"     {return DIV;}
"+"     {return ADD;}
"-"     {return SUB;}
"<"     {return LT;}
"<="    {return LTE;}
">"     {return GT;}
">="    {return GTE;}
"=="    {return EQ;}
"!="    {return NEQ;}
"="     {return ASSIN;}

";"     {return SEMICOLON;}
","     {return COMMA;}
"("     {return LPARENTHESE;}
")"     {return RPARENTHESE;}
"["     {return LBRACKET;}
"]"     {return RBRACKET;}
"{"     {return LBRACE;}
"}"     {return RBRACE;}

{ID}            {return IDENTIFIER;}
{INTEGER}       {return INTEGER;}
{FLOATPOINT}    {return FLOATPOINT;}
{ARRAY}         {return ARRAY;}

{EOL}           {return EOL;}
{BLANK}         {return BLANK;}

{COMMENT}       {return COMMENT;}
{HALF_COMMENT}  {printf("BAD COMMENT"); return ERROR;}

. {return ERROR;}




 /****请在此补全所有flex的模式与动作  end******/
%%
/****************C代码 start*************/

/// \brief analysize a *.cminus file
///
/// \param input_file, 需要分析的文件路径
/// \param token stream, Token_Node结构体数组，用于存储分析结果，具体定义参考lexical_analyer.h

void analyzer(char* input_file, Token_Node* token_stream){
    lines = 1;
    pos_start = 1;
    pos_end = 1;
    if(!(yyin = fopen(input_file,"r"))){
        printf("[ERR] No input file\n");
        exit(1);
    }
    printf("[START]: Read from: %s\n", input_file);

    int token;
    int index = 0;

    int counter;

    while(token = yylex()){
        switch(token){
            case COMMENT:
                //STUDENT TO DO
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
                break;
            case BLANK:
                //STUDENT TO DO
                pos_end += yyleng;
                break;
            case EOL:
                //STUDENT TO DO
                lines++;
                pos_end = 1;
                break;
            case ERROR:
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
                printf("[ERR]: unable to analysize %s at %d line, from %d to %d\n", yytext, lines, pos_start, pos_end);
            default :
                if (token == ERROR){
                    sprintf(token_stream[index].text, "[ERR]: unable to analysize %s at %d line, from %d to %d", yytext, lines, pos_start, pos_end);
                } else {
                    strcpy(token_stream[index].text, yytext);
                    pos_start = pos_end;
                    pos_end += yyleng;
                }
                token_stream[index].token = token;
                token_stream[index].lines = lines;
                token_stream[index].pos_start = pos_start;
                token_stream[index].pos_end = pos_end;
                index++;
                if (index >= MAX_NUM_TOKEN_NODE){
                    printf("%s has too many tokens (> %d)", input_file, MAX_NUM_TOKEN_NODE);
                    exit(1);
                }
        }
    }
    printf("[END]: Analysis completed.\n");
    return;
}



/****************C代码 end*************/
