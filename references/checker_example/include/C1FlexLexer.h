#ifndef _C1_FLEX_LEXER_H_
#define _C1_FLEX_LEXER_H_

#ifndef YY_DECL
#define YY_DECL                                                         \
    yy::C1Parser::symbol_type C1FlexLexer::yylex(C1Driver& driver)
#endif

// We need this for yyFlexLexer. If we don't #undef yyFlexLexer, the
// preprocessor chokes on the line `#define yyFlexLexer yyFlexLexer`
// in `FlexLexer.h`:
#undef yyFlexLexer
#include <FlexLexer.h>

// We need this for the yy::C1Parser::symbol_type:
#include "C1Parser.h"

// We need this for the yy::location type:
#include "location.hh"

class C1FlexLexer : public yyFlexLexer {
public:
    // Use the superclass's constructor:
    using yyFlexLexer::yyFlexLexer;

    // Provide the interface to `yylex`; `flex` will emit the
    // definition into `C1Scanner.cpp`:
    yy::C1Parser::symbol_type yylex(C1Driver& driver);

    // This seems like a reasonable place to put the location object
    // rather than it being static (in the sense of having internal
    // linkage at translation unit scope, not in the sense of being a
    // class variable):
    yy::location loc;
};

#endif // _C1_FLEX_LEXER_H_
