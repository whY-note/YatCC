#pragma once

#include <string>
#include <string_view>
#include <cstring>

namespace lex {

enum Id
{
  // Error
  YYEMPTY = -2,
  YYEOF = 0,     /* "end of file"  */
  YYerror = 256, /* error  */
  YYUNDEF = 257, /* "invalid token"  */

  // Identifier and Contants
  IDENTIFIER,
  CONSTANT,
  STRING_LITERAL,
  
  // 括号 Braces
  L_BRACE,
  R_BRACE,
  L_SQUARE,
  R_SQUARE,
  L_PAREN,
  R_PAREN,

  // 分隔符 Delimiters
  SEMI,
  COMMA,

  // Arithmetic Operators
  EQUAL,
  PLUS,
  MINUS,
  STAR,
  SLASH,
  PERCENT,

  // Comparison Operators
  GREATER,
  LESS,
  GREATEREQUAL,
  LESSEQUAL,
  EXCLAIMEQUAL,
  EQUALEQUAL,

  // Logical Operators
  EXCLAIM,
  AMPAMP,
  PIPEPIPE,

  // Keywords for Type Modifiers
  CONST,
  
  // Keywords for Type
  VOID,
  FLOAT,
  DOUBLE,
  CHAR,
  INT,
  STRUCT,
  CLASS,

  // Keywords for Control
  IF,
  ELSE,
  RETURN,
  WHILE,
  FOR,
  BREAK,
  CONTINUE,
};

const char*
id2str(Id id);

struct G
{
  Id mId{ YYEOF };              // 词号
  std::string_view mText;       // 对应文本
  std::string mFile;            // 文件路径
  int mLine{ 1 }, mColumn{ 1 }; // 行号、列号
  bool mStartOfLine{ true };    // 是否是行首
  bool mLeadingSpace{ false };  // 是否有前导空格
};

extern G g;

int
come(int tokenId, const char* yytext, int yyleng, int yylineno);

} // namespace lex
