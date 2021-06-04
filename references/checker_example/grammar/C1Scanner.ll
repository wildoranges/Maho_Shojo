%{ /* -*- C++ -*- */
# include <cerrno>
# include <climits>
# include <cstdlib>
# include <cstring> // strerror
# include <string>
# include <algorithm>
# include <iostream>
# include "C1Driver.h"
# include "C1Parser.h"
%}

%{
#if defined __clang__
# define CLANG_VERSION (__clang_major__ * 100 + __clang_minor__)
#endif

// Clang and ICC like to pretend they are GCC.
#if defined __GNUC__ && !defined __clang__ && !defined __ICC
# define GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#endif

// Pacify warnings in yy_init_buffer (observed with Flex 2.6.4)
// and GCC 6.4.0, 7.3.0 with -O3.
#if defined GCC_VERSION && 600 <= GCC_VERSION
# pragma GCC diagnostic ignored "-Wnull-dereference"
#endif

// This code uses Flex's C backend, yet compiles it as C++.
// So expect warnings about C style casts and NULL.
#if defined CLANG_VERSION && 500 <= CLANG_VERSION
# pragma clang diagnostic ignored "-Wold-style-cast"
# pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#elif defined GCC_VERSION && 407 <= GCC_VERSION
# pragma GCC diagnostic ignored "-Wold-style-cast"
# pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#define FLEX_VERSION (YY_FLEX_MAJOR_VERSION * 100 + YY_FLEX_MINOR_VERSION)

// Old versions of Flex (2.5.35) generate an incomplete documentation comment.
//
//  In file included from src/scan-code-c.c:3:
//  src/scan-code.c:2198:21: error: empty paragraph passed to '@param' command
//        [-Werror,-Wdocumentation]
//   * @param line_number
//     ~~~~~~~~~~~~~~~~~^
//  1 error generated.
#if FLEX_VERSION < 206 && defined CLANG_VERSION
# pragma clang diagnostic ignored "-Wdocumentation"
#endif

// Old versions of Flex (2.5.35) use 'register'.  Warnings introduced in
// GCC 7 and Clang 6.
#if FLEX_VERSION < 206
# if defined CLANG_VERSION && 600 <= CLANG_VERSION
#  pragma clang diagnostic ignored "-Wdeprecated-register"
# elif defined GCC_VERSION && 700 <= GCC_VERSION
#  pragma GCC diagnostic ignored "-Wregister"
# endif
#endif

#if FLEX_VERSION < 206
# if defined CLANG_VERSION
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wdocumentation"
#  pragma clang diagnostic ignored "-Wshorten-64-to-32"
#  pragma clang diagnostic ignored "-Wsign-conversion"
# elif defined GCC_VERSION
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
# endif
#endif
%}

%option noyywrap nounput noinput batch debug

 /**/
%{
  // Code run each time a pattern is matched.
  # define YY_USER_ACTION  loc.columns(yyleng);
%}

 /* Regex abbreviations: */

MultilineComment		"/*"([^\*]|(\*)*[^\*/])*(\*)*"*/"				
SingleLineComment		"//".*$									
Identifier		      [_a-zA-Z][a-zA-Z0-9_]*
IntConst            (("0"[0-7]*)|([1-9][0-9]*)|("0"[xX][0-9a-fA-F]+))
FloatConst          ((([0-9]*[.][0-9]+)|([0-9]+[.]))([eE][-+]?[0-9]+)?)|([0-9]+[eE][-+]?[0-9]+)
Blank               [ \t\r]
NewLine             [\n]

%%
 /* keyword */
int 		{return yy::C1Parser::make_INT(loc);}
return 	    {return yy::C1Parser::make_RETURN(loc);}
void 		{return yy::C1Parser::make_VOID(loc);}
const		{return yy::C1Parser::make_CONST(loc);}
break       {return yy::C1Parser::make_BREAK(loc);}
continue    {return yy::C1Parser::make_CONTINUE(loc);}
while       {return yy::C1Parser::make_WHILE(loc);}
if          {return yy::C1Parser::make_IF(loc);}
else        {return yy::C1Parser::make_ELSE(loc);}

[<]     {return yy::C1Parser::make_LT(loc);}
"<="    {return yy::C1Parser::make_LTE(loc);}
[>]     {return yy::C1Parser::make_GT(loc);}
">="    {return yy::C1Parser::make_GTE(loc);}
"=="    {return yy::C1Parser::make_EQ(loc);}
"!="    {return yy::C1Parser::make_NEQ(loc);}
[!]     {return yy::C1Parser::make_NOT(loc);}
"&&"    {return yy::C1Parser::make_LOGICAND(loc);}
"||"    {return yy::C1Parser::make_LOGICOR(loc);}
[+] 		{return yy::C1Parser::make_PLUS(loc);}
[-] 		{return yy::C1Parser::make_MINUS(loc);}
[*] 		{return yy::C1Parser::make_MULTIPLY(loc);}
[/] 		{return yy::C1Parser::make_DIVIDE(loc);}
[%]			{return yy::C1Parser::make_MODULO(loc);}
[=] 		{return yy::C1Parser::make_ASSIGN(loc);}
[;] 		{return yy::C1Parser::make_SEMICOLON(loc);}
[,] 		{return yy::C1Parser::make_COMMA(loc);}
[(] 		{return yy::C1Parser::make_LPARENTHESE(loc);}
[)] 		{return yy::C1Parser::make_RPARENTHESE(loc);}
[[] 		{return yy::C1Parser::make_LBRACKET(loc);} 
[]] 		{return yy::C1Parser::make_RBRACKET(loc);}
[{] 		{return yy::C1Parser::make_LBRACE(loc);}
[}] 		{return yy::C1Parser::make_RBRACE(loc);}

{Blank}+                  {loc.step();}
{NewLine}+                {loc.lines(yyleng); loc.step();}
{MultilineComment}				{std::string s = yytext;
                          size_t n = std::count(s.begin(), s.end(), '\n');
                          for (size_t i = 0; i < n; i++) loc.lines(1);}
{SingleLineComment}				/* ignore */
{IntConst} 							  {return yy::C1Parser::make_INTCONST(std::stoi(yytext,0,0),loc);}
{FloatConst}							{return yy::C1Parser::make_FLOATCONST(std::stod(yytext),loc);}
{Identifier} 					    {return yy::C1Parser::make_IDENTIFIER(yytext, loc);}

<<EOF>>                   {return yy::C1Parser::make_END(loc);}
.			                    {std::cout << "Error in scanner!" << '\n'; exit(1);}
%%

int yyFlexLexer::yylex() {
    std::cerr << "'int yyFlexLexer::yylex()' should never be called." << std::endl;
    exit(1);
}
