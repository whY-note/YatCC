#include "lex.hpp"
#include <iostream>

void
print_token();

namespace lex {

static const char* kTokenNames[] = {
  // Identifier and Contants
  "identifier",   "numeric_constant",   "string_literal",                      

  // 括号 Braces
  "l_brace",      "r_brace",            "l_square",
  "r_square",     "l_paren",            "r_paren",

  // 分隔符 Delimiters
  "semi",         "comma",

  // Arithmetic Operators
  "equal",        "plus",               "minus",      
  "star",         "slash",              "percent",            

  // Comparison Operators
  "greater",      "less",               "greaterequal",
  "lessequal",    "exclaimequal",       "equalequal",
  
  // Logical Operators
  "exclaim",      "ampamp",             "pipepipe",

  // Keywords for Type Modifiers
  "const",

  // Keywords for Type
  "void",         "float",              "double",
  "char",         "int", 
  "struct",       "class",

  // Keywords for Control
  "if",           "else",               "return",
  "while",        "for",                "break",
  "continue",
};

const char*
id2str(Id id)
{
  static char sCharBuf[2] = { 0, 0 };
  if (id == Id::YYEOF) {
    return "eof";
  }
  else if (id < Id::IDENTIFIER) {
    sCharBuf[0] = char(id);
    return sCharBuf;
  }
  return kTokenNames[int(id) - int(Id::IDENTIFIER)];
}

G g;

int
come(int tokenId, const char* yytext, int yyleng, int yylineno)
{
  g.mId = Id(tokenId);
  g.mText = { yytext, std::size_t(yyleng) };
  g.mLine = yylineno;

  print_token();
  g.mStartOfLine = false;
  g.mLeadingSpace = false;

  return tokenId;
}

} // namespace lex
