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
    :   primaryExpression  
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


multiplicativeExpression
    :   unaryExpression ((Star|Div|Mod) unaryExpression)*
    ;

additiveExpression
    :   multiplicativeExpression ((Plus|Minus) multiplicativeExpression)*
    ;

assignmentExpression
    :   additiveExpression
    |   unaryExpression Equal assignmentExpression
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
    :   declarator (Equal initializer)?
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
    :   Identifier
    |   directDeclarator LeftBracket assignmentExpression? RightBracket
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
    |   expressionStatement
    |   jumpStatement
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
    :   (Return expression?)
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
    : declarationSpecifiers directDeclarator LeftParen RightParen compoundStatement
    ;

