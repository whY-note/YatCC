%{
#include "lex.hpp"
/* 所有代码全部抽离出来，放到 lex.hpp 和 lex.cpp 里 */

using namespace lex;

#define ADDCOL() g.mColumn += yyleng;
#define COME(id) return come(id, yytext, yyleng, yylineno)
%}

%option 8bit warn noyywrap yylineno

D     [0-9]
L     [a-zA-Z_]
IS    ((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))

%%


"("         { ADDCOL(); COME(L_PAREN); }
")"         { ADDCOL(); COME(R_PAREN); }
"["         { ADDCOL(); COME(L_SQUARE); }
"]"         { ADDCOL(); COME(R_SQUARE); }
"{"         { ADDCOL(); COME(L_BRACE); }
"}"         { ADDCOL(); COME(R_BRACE); }


";"         { ADDCOL(); COME(SEMI); }
","         { ADDCOL(); COME(COMMA); }


"="         { ADDCOL(); COME(EQUAL); }
"+"         { ADDCOL(); COME(PLUS); }
"-"         { ADDCOL(); COME(MINUS); }
"*"         { ADDCOL(); COME(STAR); }
"/"         { ADDCOL(); COME(SLASH); }
"%"         { ADDCOL(); COME(PERCENT); }


"=="        { ADDCOL(); COME(EQUALEQUAL); }
">"         { ADDCOL(); COME(GREATER); }
"<"         { ADDCOL(); COME(LESS); }
">="        { ADDCOL(); COME(GREATEREQUAL); }
"<="        { ADDCOL(); COME(LESSEQUAL); }
"!="        { ADDCOL(); COME(EXCLAIMEQUAL); }


"!"         { ADDCOL(); COME(EXCLAIM); }
"&&"        { ADDCOL(); COME(AMPAMP); }
"||"        { ADDCOL(); COME(PIPEPIPE); }


"const"     { ADDCOL(); COME(CONST); }


"void"      { ADDCOL(); COME(VOID); }
"float"     { ADDCOL(); COME(FLOAT); }
"double"    { ADDCOL(); COME(DOUBLE); }
"char"      { ADDCOL(); COME(CHAR); }
"int"       { ADDCOL(); COME(INT); }
"struct"    { ADDCOL(); COME(STRUCT); }
"class"     { ADDCOL(); COME(CLASS); }


"if"        { ADDCOL(); COME(IF); }
"else"      { ADDCOL(); COME(ELSE); }
"return"    { ADDCOL(); COME(RETURN); }
"while"     { ADDCOL(); COME(WHILE); }
"for"       { ADDCOL(); COME(FOR); }
"break"     { ADDCOL(); COME(BREAK); }
"continue"  { ADDCOL(); COME(CONTINUE); }


{L}({L}|{D})*         { ADDCOL(); COME(IDENTIFIER); }

L?\"(\\.|[^\\"\n])*\" { ADDCOL(); COME(STRING_LITERAL); }

0[0-7]*{IS}?          { ADDCOL(); COME(CONSTANT); }
[1-9]{D}*{IS}?        { ADDCOL(); COME(CONSTANT); }


^#[^\n]*              {
                        char filename[512];
                        int lineno;
                        if (sscanf(yytext, "# %d \"%[^\"]\"", &lineno, filename) == 2 ) {
                            if (filename[0] != '<') {
                                yylineno = lineno-1;      /* 更新行号 */
                                yyleng = 0;
                                g.mFile = filename;     /* 文件名*/
                            }
                        }
}

[\t\f]               { ADDCOL(); return ~YYEOF; }

[ ] {
    g.mLeadingSpace = true;
    ADDCOL(); return ~YYEOF;
}

\v {
    g.mLine += 1;
    g.mStartOfLine = true;
    return ~YYEOF;
}

\n { 
    g.mColumn = 1;
    g.mLine += 1;
    g.mStartOfLine = true;
    return ~YYEOF;
} /* 换行 */

0[xX][0-9a-fA-F]+           {ADDCOL(); COME(CONSTANT);} /* 十六进制常量 */

<<EOF>>     { ADDCOL(); COME(YYEOF); }




%%

/* about symbols avaliable (yytext, yyleng etc.) in the context of Flex:
 * https://ftp.gnu.org/old-gnu/Manuals/flex-2.5.4/html_node/flex_14.html
 * https://ftp.gnu.org/old-gnu/Manuals/flex-2.5.4/html_node/flex_15.html
*/
