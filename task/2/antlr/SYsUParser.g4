parser grammar SYsUParser;

options {
  tokenVocab=SYsULexer;
}

primaryExpression
    :   Identifier
    |   Constant
    |   StringLiteral
    |   LeftParen expression RightParen
    ;


postfixExpression
    : primaryExpression postfixSuffix*
    ;

postfixSuffix
    : LeftBracket expression RightBracket
    | LeftParen argumentExpressionList? RightParen
    ;


argumentExpressionList
    : assignmentExpression (Comma assignmentExpression)*
    ;

unaryExpression
    :
    (postfixExpression
    |   unaryOperator unaryExpression
    )
    ;

unaryOperator
    :   Plus | Minus | Star
    ;

logicalOrExpression
    : logicalAndExpression (PipePipe logicalAndExpression)*
    ;

logicalAndExpression
    : equalityExpression (AmpAmp equalityExpression)*
    ;

equalityExpression
    : relationalExpression ((EqualEqual | ExclaimEqual) relationalExpression)*
    ;

relationalExpression
    : additiveExpression ((Less | LessEqual | Greater | GreaterEqual) additiveExpression)*
    ;

additiveExpression
    :   multiplicativeExpression ((Plus|Minus) multiplicativeExpression)*
    ;

multiplicativeExpression
    :   unaryExpression ((Star|Div|Mod) unaryExpression)*
    ;

assignmentExpression
    :   unaryExpression Equal assignmentExpression
    |   logicalOrExpression
    // |   conditionalExpression
    ;

expression
    :   assignmentExpression (Comma assignmentExpression)*
    ;

// constantExpression
//     : conditionalExpression
//     ;

// conditionalExpression
//     : logical_or_expression
//     | logical_or_expression '?' expression ':' conditional_expression
//     ;

declaration
    :   declarationSpecifiers initDeclaratorList? Semi
    ;

declarationSpecifiers
    :   declarationSpecifier+
    ;

declarationSpecifier
    :   typeSpecifier
    |   typeQualifier 
    ;


initDeclaratorList
    :   initDeclarator (Comma initDeclarator)*
    ;

initDeclarator
    :   declarator (LeftParen parameterList? RightParen)? (Equal initializer)?
    ;


typeSpecifier
    :   Int | Void | Float | Double | Char | Struct | Class
    ;

typeQualifier
    : Const
    ;
    
typeQualifierList
    : typeQualifier+
    ;
    
declarator
    :   directDeclarator
    ;

directDeclarator
    : Identifier
    | directDeclarator LeftBracket assignmentExpression? RightBracket
    // | directDeclarator LeftParen parameterList? RightParen
    ;

identifierList
    :   Identifier (Comma Identifier)*
    ;

initializer
    :   assignmentExpression
    |   LeftBrace initializerList? Comma? RightBrace
    ;

initializerList
    :   designation? initializer (Comma designation? initializer)*
    |   initializer (Comma initializer)*
    ;

designation
    : designator_list Equal
    ;

designator_list
    : designator
    | designator_list designator
    ;

designator
    : Dot Identifier
    // | LeftBracket constantExpression RightBracket
    ;

statement
    :   compoundStatement
    |   selectionStatement
    |   iterationStatement
    |   expressionStatement
    |   jumpStatement
    ;


iterationStatement
    :   While LeftParen expression RightParen statement
    ;

selectionStatement
    :   If LeftParen expression RightParen statement (Else statement)?
    ;

compoundStatement
    :   LeftBrace blockItemList? RightBrace
    ;

blockItemList
    :   blockItem+
    ;

blockItem
    :   statement
    |   declaration
    ;

expressionStatement
    :   expression? Semi
    ;



jumpStatement
    :   (Return expression?
    |   Break
    |   Continue)
    Semi
    ;

// 整个文件
compilationUnit
    :   translationUnit? EOF
    ;

translationUnit
    :   externalDeclaration+
    ;

// 全局元素
externalDeclaration
    :   functionDefinition //函数
    |   declaration //全局变量
    ;

// 函数定义
functionDefinition
    : declarationSpecifiers declarator LeftParen (parameterList)? RightParen compoundStatement
    // : declarationSpecifiers declarator compoundStatement
    ;
    
// 参数列表
parameterList
    : parameter (Comma parameter)*
    ;

parameter
    : declarationSpecifiers declarator?
    ;