#include "Ast2Asg.hpp"
#include <unordered_map>

#define self (*this)

namespace asg {

// 符号表，保存当前作用域的所有声明
struct Ast2Asg::Symtbl : public std::unordered_map<std::string, Decl*>
{
  Ast2Asg& m;
  Symtbl* mPrev;

  Symtbl(Ast2Asg& m)
    : m(m)
    , mPrev(m.mSymtbl)
  {
    m.mSymtbl = this;
  }

  ~Symtbl() { m.mSymtbl = mPrev; }

  Decl* resolve(const std::string& name);
};

Decl*
Ast2Asg::Symtbl::resolve(const std::string& name)
{
  auto iter = find(name);
  if (iter != end())
    return iter->second;
  ASSERT(mPrev != nullptr); // 标识符未定义
  return mPrev->resolve(name);
}

TranslationUnit*
Ast2Asg::operator()(ast::TranslationUnitContext* ctx)
{
  auto ret = make<asg::TranslationUnit>();
  if (ctx == nullptr)
    return ret;

  Symtbl localDecls(self);

  // 兜底内建声明：允许在未显式声明 putint 的情况下完成符号解析。
  if (localDecls.find("putint") == localDecls.end()) {
    auto fdecl = make<FunctionDecl>();
    fdecl->name = "putint";

    auto retType = make<Type>();
    retType->spec = Type::Spec::kVoid;
    retType->qual = Type::Qual();

    auto ftype = make<FunctionType>();
    ftype->sub = nullptr;

    auto argType = make<Type>();
    argType->spec = Type::Spec::kInt;
    argType->qual = Type::Qual();
    argType->texp = nullptr;
    ftype->params.push_back(argType);

    retType->texp = ftype;
    fdecl->type = retType;

    auto argDecl = make<VarDecl>();
    argDecl->name = "x";
    argDecl->type = argType;
    fdecl->params.push_back(argDecl);

    localDecls[fdecl->name] = fdecl;
  }

  for (auto&& i : ctx->externalDeclaration()) {
    if (auto p = i->declaration()) {
      auto decls = self(p);
      ret->decls.insert(ret->decls.end(),
                        std::make_move_iterator(decls.begin()),
                        std::make_move_iterator(decls.end()));

      // 全局声明需要进入符号表，供后续函数体内标识符解析使用。
      for (auto* decl : decls)
        localDecls[decl->name] = decl;
    }

    else if (auto p = i->functionDefinition()) {
      auto funcDecl = self(p);
      ret->decls.push_back(funcDecl);

      // 添加到声明表
      localDecls[funcDecl->name] = funcDecl;
      // (*mSymtbl)[funcDecl->name] = funcDecl;
    }

    else
      ABORT();
  }

  return ret;
}

//==============================================================================
// 类型
//==============================================================================

Ast2Asg::SpecQual
Ast2Asg::operator()(ast::DeclarationSpecifiersContext* ctx)
{
  SpecQual ret = { Type::Spec::kINVALID, Type::Qual() };

  for (auto&& i : ctx->declarationSpecifier()) {
    if (auto p = i->typeSpecifier()) {
      if (ret.first == Type::Spec::kINVALID) {
        if (p->Int())
          ret.first = Type::Spec::kInt;
        else if (p->Void())
          ret.first = Type::Spec::kVoid;
        else if (p->Char())
          ret.first = Type::Spec::kChar;
        
        else
          ABORT(); // 未知的类型说明符
      }

      else
        ABORT(); // 未知的类型说明符
    }

    else if (auto p = i->typeQualifier()) {
      if (p->Const())
        ret.second.const_ = true;
      else
        ABORT();
    }

    else
      ABORT();
  }

  return ret;
}

std::pair<TypeExpr*, std::string>
Ast2Asg::operator()(ast::DeclaratorContext* ctx, TypeExpr* sub)
{
  return self(ctx->directDeclarator(), sub);
}

static int
eval_arrlen(Expr* expr)
{
  if (auto p = expr->dcst<IntegerLiteral>())
    return p->val;

  if (auto p = expr->dcst<DeclRefExpr>()) {
    if (p->decl == nullptr)
      ABORT();

    auto var = p->decl->dcst<VarDecl>();
    if (!var || !var->type->qual.const_)
      ABORT(); // 数组长度必须是编译期常量

    switch (var->type->spec) {
      case Type::Spec::kChar:
      case Type::Spec::kInt:
      case Type::Spec::kLong:
      case Type::Spec::kLongLong:
        return eval_arrlen(var->init);

      default:
        ABORT(); // 长度表达式必须是数值类型
    }
  }

  if (auto p = expr->dcst<UnaryExpr>()) {
    auto sub = eval_arrlen(p->sub);

    switch (p->op) {
      case UnaryExpr::kPos:
        return sub;

      case UnaryExpr::kNeg:
        return -sub;

      default:
        ABORT();
    }
  }

  if (auto p = expr->dcst<BinaryExpr>()) {
    auto lft = eval_arrlen(p->lft);
    auto rht = eval_arrlen(p->rht);

    switch (p->op) {
      case BinaryExpr::kAdd:
        return lft + rht;

      case BinaryExpr::kSub:
        return lft - rht;

      default:
        ABORT();
    }
  }

  if (auto p = expr->dcst<InitListExpr>()) {
    if (p->list.empty())
      return 0;
    return eval_arrlen(p->list[0]);
  }

  ABORT();
}

std::pair<TypeExpr*, std::string>
Ast2Asg::operator()(ast::DirectDeclaratorContext* ctx, TypeExpr* sub)
{
  if (auto p = ctx->Identifier())
    return { sub, p->getText() };

  // Array Declaration
  if (ctx->LeftBracket()) {
    auto arrayType = make<ArrayType>();
    arrayType->sub = sub;

    if (auto p = ctx->assignmentExpression())
      arrayType->len = eval_arrlen(self(p));
    else
      arrayType->len = ArrayType::kUnLen;

    return self(ctx->directDeclarator(), arrayType);
  }

  // // Function Declaration
  // if (ctx->LeftParen())
  // {
  //   auto funcType = make<FunctionType>();
  //   funcType->sub = sub;

  //   // 参数
  //   if (auto plist = ctx->parameterList())
  //   {
  //     for (auto p : plist->parameter())
  //     {
  //       auto [ptexp, pname] = self(p->declarator(), nullptr);

  //       auto psq = self(p->declarationSpecifiers());

  //       auto ptype = make<Type>();
  //       ptype->spec = psq.first;
  //       ptype->qual = psq.second;
  //       ptype->texp = ptexp;

  //       funcType->params.push_back(ptype);
  //     }
  //   }

  //   return self(ctx->directDeclarator(), funcType);
  // }

  ABORT();
}

//==============================================================================
// 表达式
//==============================================================================

Expr*
Ast2Asg::operator()(ast::ExpressionContext* ctx)
{
  auto list = ctx->assignmentExpression();
  Expr* ret = self(list[0]);

  for (unsigned i = 1; i < list.size(); ++i) {
    auto node = make<BinaryExpr>();
    node->op = node->kComma;
    node->lft = ret;
    node->rht = self(list[i]);
    ret = node;
  }

  return ret;
}

Expr*
Ast2Asg::operator()(ast::AssignmentExpressionContext* ctx)
{
  if (ctx->Equal())
  {
    auto ret = make<BinaryExpr>();
    ret->op = ret->kAssign;
    ret->lft = self(ctx->unaryExpression());
    ret->rht = self(ctx->assignmentExpression());
    return ret;
  }

  return self(ctx->logicalOrExpression());
}

Expr*
Ast2Asg::operator()(ast::LogicalOrExpressionContext* ctx) {
  auto children = ctx->children;
  Expr* ret = self(dynamic_cast<ast::LogicalAndExpressionContext*>(children[0]));
  for (unsigned i = 1; i < children.size(); i++) {
    auto node = make<BinaryExpr>();

    auto token = dynamic_cast<antlr4::tree::TerminalNode*>(children[i])
                  ->getSymbol()
                  ->getType();

    switch (token) {
      case ast::PipePipe:
        node->op = node->kOr;
        break;
      default:
       ABORT();
    }
    node->lft = ret;
    node->rht = self(dynamic_cast<ast::LogicalAndExpressionContext*>(children[++i]));
    ret = node; 
  }
  return ret;
}

Expr*
Ast2Asg::operator()(ast::LogicalAndExpressionContext* ctx) {
  auto children = ctx->children;
  Expr* ret = self(dynamic_cast<ast::EqualityExpressionContext*>(children[0]));
  for (unsigned i = 1; i < children.size(); i++) {
    auto node = make<BinaryExpr>();

    auto token = dynamic_cast<antlr4::tree::TerminalNode*>(children[i])
                  ->getSymbol()
                  ->getType();
    switch (token) {
      case ast::AmpAmp:
        node->op = node->kAnd;
        break;
      default:
       ABORT();
    }
    node->lft = ret;
    node->rht = self(dynamic_cast<ast::EqualityExpressionContext*>(children[++i]));
    ret = node;
  }
  return ret;
}

Expr*
Ast2Asg::operator()(ast::EqualityExpressionContext* ctx) {
  auto children = ctx->children;
  Expr* ret = self(dynamic_cast<ast::RelationalExpressionContext*>(children[0]));
  for (unsigned i = 1; i < children.size(); i++) {
    auto node = make<BinaryExpr>();

    auto token = dynamic_cast<antlr4::tree::TerminalNode*>(children[i])
                  ->getSymbol()
                  ->getType();

    switch (token) {
      case ast::EqualEqual:
        node->op = node->kEq;
        break;
      case ast::ExclaimEqual:
        node->op = node->kNe;
        break;
      default:
        ABORT();
    }
    node->lft = ret;
    node->rht = self(dynamic_cast<ast::RelationalExpressionContext*>(children[++i]));
    ret = node;
  }
  return ret;
}

Expr*
Ast2Asg::operator()(ast::RelationalExpressionContext* ctx) {
  auto children = ctx->children;
  Expr* ret = self(dynamic_cast<ast::AdditiveExpressionContext*>(children[0]));
  for (unsigned i = 1; i < children.size(); i++) {
    auto node = make<BinaryExpr>();

    auto token = dynamic_cast<antlr4::tree::TerminalNode*>(children[i])
                    ->getSymbol()
                    ->getType();

    switch (token) {
      case ast::Less:
        node->op = node->kLt;
        break;

      case ast::LessEqual:
        node->op = node->kLe;
        break;
      
      case ast::Greater:
        node->op = node->kGt;
        break;
      
      case ast::GreaterEqual:
        node->op = node->kGe;
        break;

      default:
        ABORT();
    }
    node->lft = ret;
    node->rht = self(dynamic_cast<ast::AdditiveExpressionContext*>(children[++i]));
    ret = node;
  }
  return ret;
}

Expr*
Ast2Asg::operator()(ast::AdditiveExpressionContext* ctx)
{
  auto children = ctx->children;
  // assert(dynamic_cast<ast::UnaryExpressionContext*>(children[0]));
  Expr* ret =self(dynamic_cast<ast::MultiplicativeExpressionContext*>(children[0]));

  for (unsigned i = 1; i < children.size(); ++i) {
    auto node = make<BinaryExpr>();

    auto token = dynamic_cast<antlr4::tree::TerminalNode*>(children[i])
                   ->getSymbol()
                   ->getType();
    switch (token) {
      case ast::Plus:
        node->op = node->kAdd;
        break;

      case ast::Minus:
        node->op = node->kSub;
        break;

      default:
        ABORT();
    }

    node->lft = ret;
    node->rht =
      self(dynamic_cast<ast::MultiplicativeExpressionContext*>(children[++i]));
    ret = node;
  }

  return ret;
}

Expr*
Ast2Asg::operator()(ast::MultiplicativeExpressionContext* ctx)
{
  auto children = ctx->children;
  Expr* ret = self(dynamic_cast<ast::UnaryExpressionContext*>(children[0]));

  for (unsigned i = 1; i < children.size(); ++i) {
    auto node = make<BinaryExpr>();

    auto token = dynamic_cast<antlr4::tree::TerminalNode*>(children[i])
                   ->getSymbol()
                   ->getType();
    switch (token) {
      case ast::Star:
        node->op = node->kMul;
        break;

      case ast::Div:
        node->op = node->kDiv;
        break;

      case ast::Mod:
        node->op = node->kMod;
        break;

      default:
        ABORT();
    }

    node->lft = ret;
    node->rht = self(dynamic_cast<ast::UnaryExpressionContext*>(children[++i]));
    ret = node;
  }

  return ret;
}

Expr*
Ast2Asg::operator()(ast::UnaryExpressionContext* ctx)
{
  if (auto p = ctx->postfixExpression())
    return self(p);

  auto ret = make<UnaryExpr>();

  switch (
    dynamic_cast<antlr4::tree::TerminalNode*>(ctx->unaryOperator()->children[0])
      ->getSymbol()
      ->getType()) {
    case ast::Plus:
      ret->op = ret->kPos;
      break;

    case ast::Minus:
      ret->op = ret->kNeg;
      break;

    default:
      ABORT();
  }

  ret->sub = self(ctx->unaryExpression());

  return ret;
}

Expr*
Ast2Asg::operator()(ast::PostfixExpressionContext* ctx)
{
  Expr* ret =
      self(ctx->primaryExpression());

  for (auto suffix : ctx->postfixSuffix())
  {
    // array index
    if (suffix->LeftBracket())
    {
      auto indexExpr =
          self(suffix->expression());

      auto node = make<BinaryExpr>();
      node->op = BinaryExpr::kIndex;

      node->lft = addImplicitCast(
          ret,
          ImplicitCastExpr::Kind::kArrayToPointerDecay);

      node->rht = addImplicitCast(
          indexExpr,
          ImplicitCastExpr::Kind::kLValueToRValue);

      ret = node;
      continue;
    }

    // function call
    if (suffix->LeftParen())
    {
      auto call = make<CallExpr>();
      call->head = ret;

      auto declref = ret->dcst<DeclRefExpr>();
      if (!declref || !declref->decl->dcst<FunctionDecl>()) {
        ABORT();
      }

      auto argsCtx = suffix->argumentExpressionList();

      if (argsCtx)
      {
        for (auto expr : argsCtx->assignmentExpression())
        {
          call->args.push_back(self(expr));
        }
      }

      ret = call;
      continue;
    }

    ABORT();
  }

  return ret;
}

Expr*
Ast2Asg::operator()(ast::PrimaryExpressionContext* ctx)
{

  if (auto p = ctx->Identifier()) {
    auto name = p->getText();
    auto ret = make<DeclRefExpr>();
    ret->decl = mSymtbl->resolve(name);
    return ret;
  }

  if (auto p = ctx->Constant()) {
    auto text = p->getText();

    auto ret = make<IntegerLiteral>();

    ASSERT(!text.empty());
    if (text[0] != '0')
      ret->val = std::stoll(text);

    else if (text.size() == 1)
      ret->val = 0;

    else if (text[1] == 'x' || text[1] == 'X')
      ret->val = std::stoll(text.substr(2), nullptr, 16);

    else
      ret->val = std::stoll(text.substr(1), nullptr, 8);

    return ret;
  }

  if (auto p = ctx->StringLiteral()) {
        auto text = p->getText();
        auto ret = make<StringLiteral>();
        // 去掉首尾引号
        ret->val = text.substr(1, text.size() - 2);
        return ret;
  }

  // Parenthesized expression: ( expression )
  if (auto exprCtx = ctx->expression()) {
    auto ret = make<ParenExpr>();
    ret->sub = (*this)(exprCtx); // 括号里的 expression
    return ret;
  }

  ABORT();
}


Expr*
Ast2Asg::operator()(ast::InitializerContext* ctx)
{
  if (auto p = ctx->assignmentExpression())
    return self(p);

  auto ret = make<InitListExpr>();

  if (auto p = ctx->initializerList()) {
    for (auto&& i : p->initializer()) {
      // 先不要 flatten
      ret->list.push_back(self(i));
    }
  }

  return ret;
}

//==============================================================================
// 语句
//==============================================================================

Stmt*
Ast2Asg::operator()(ast::StatementContext* ctx)
{
  if (auto p = ctx->compoundStatement())
    return self(p);

  if (auto p = ctx->selectionStatement())
    return self(p);

  if (auto p = ctx->expressionStatement())
    return self(p);

  if (auto p = ctx->jumpStatement())
    return self(p);

  ABORT();
}

Stmt*
Ast2Asg::operator()(ast::SelectionStatementContext* ctx)
{
  if (ctx->If()) {
    auto ret = make<IfStmt>();
    ret->cond = self(ctx->expression());
    ret->then = self(ctx->statement(0));

    if (ctx->Else())
      ret->else_ = self(ctx->statement(1));

    return ret;
  }

  ABORT();
}

CompoundStmt*
Ast2Asg::operator()(ast::CompoundStatementContext* ctx)
{
  auto ret = make<CompoundStmt>();

  if (auto p = ctx->blockItemList()) {
    Symtbl localDecls(self);

    for (auto&& i : p->blockItem()) {
      if (auto q = i->declaration()) {
        auto sub = make<DeclStmt>();
        sub->decls = self(q);
        ret->subs.push_back(sub);
      }

      else if (auto q = i->statement())
        ret->subs.push_back(self(q));

      else
        ABORT();
    }
  }

  return ret;
}

Stmt*
Ast2Asg::operator()(ast::ExpressionStatementContext* ctx)
{
  if (auto p = ctx->expression()) {
    auto ret = make<ExprStmt>();
    ret->expr = self(p);
    return ret;
  }

  return make<NullStmt>();
}

Stmt*
Ast2Asg::operator()(ast::JumpStatementContext* ctx)
{
  if (ctx->Return()) {
    auto ret = make<ReturnStmt>();
    ret->func = mCurrentFunc;
    if (auto p = ctx->expression())
      ret->expr = self(p);
    return ret;
  }

  ABORT();
}

//==============================================================================
// 声明
//==============================================================================

std::vector<Decl*>
Ast2Asg::operator()(ast::DeclarationContext* ctx)
{
  std::vector<Decl*> ret;

  auto specs = self(ctx->declarationSpecifiers());

  if (auto p = ctx->initDeclaratorList()) {
    for (auto&& j : p->initDeclarator())
      ret.push_back(self(j, specs));
  }

  // 如果 initDeclaratorList 为空则这行声明语句无意义
  return ret;
}

FunctionDecl*
Ast2Asg::operator()(ast::FunctionDefinitionContext* ctx)
{
  auto ret = make<FunctionDecl>();
  mCurrentFunc = ret;

  auto type = make<Type>();
  ret->type = type;

  auto sq = self(ctx->declarationSpecifiers());
  type->spec = sq.first, type->qual = sq.second;

  auto [texp, name] = self(ctx->declarator(), nullptr);
  auto funcType = make<FunctionType>();
  funcType->sub = texp;
  type->texp = funcType;

  // type->texp = texp;

  Symtbl funcScope(self);

  // 把函数加入符号表（支持递归）
  (*mSymtbl)[name] = ret;
  
  // 处理参数列表
  if (ctx->parameterList()) {
    for (auto p : ctx->parameterList()->parameter()) {
        if (!p->declarator())
          ABORT(); // 函数定义中的参数必须具名

        auto [ptexp, pname] = self(p->declarator(), nullptr);

        auto psq = self(p->declarationSpecifiers());
        auto param_type = make<Type>();
        param_type->spec = psq.first;
        param_type->qual = psq.second;
        param_type->texp = ptexp;

        auto vdecl = make<VarDecl>();
        vdecl->type = param_type;
        vdecl->name = pname;

        (*mSymtbl)[pname] = vdecl;

        ret->params.push_back(vdecl);

        funcType->params.push_back(param_type);
    }
  }
  
  ret->name = std::move(name);

  Symtbl localDecls(self);

  // 函数定义在签名之后就加入符号表，以允许递归调用
  (*mSymtbl)[ret->name] = ret;

  ret->body = self(ctx->compoundStatement());

  return ret;
}

Decl*
Ast2Asg::operator()(ast::InitDeclaratorContext* ctx, SpecQual sq)
{
  auto [texp, name] = self(ctx->declarator(), nullptr);
  Decl* ret;

  FunctionType* declFuncType = nullptr;
  std::vector<Decl*> declParams;

  if (ctx->LeftParen()) {
    auto funcType = make<FunctionType>();
    funcType->sub = texp;

    if (ctx->parameterList()) {
      for (auto p : ctx->parameterList()->parameter()) {
        TypeExpr* ptexp = nullptr;
        std::string pname;
        if (p->declarator()) {
          auto parsed = self(p->declarator(), nullptr);
          ptexp = parsed.first;
          pname = std::move(parsed.second);
        }

        auto psq = self(p->declarationSpecifiers());
        auto ptype = make<Type>();
        ptype->spec = psq.first;
        ptype->qual = psq.second;
        ptype->texp = ptexp;
        funcType->params.push_back(ptype);

        auto pdecl = make<VarDecl>();
        pdecl->type = ptype;
        pdecl->name = pname;
        declParams.push_back(pdecl);
      }
    }

    declFuncType = funcType;
  }

  else if (texp)
    declFuncType = texp->dcst<FunctionType>();

  if (auto funcType = declFuncType) {
    auto fdecl = make<FunctionDecl>();
    auto type = make<Type>();
    fdecl->type = type;

    type->spec = sq.first;
    type->qual = sq.second;
    type->texp = funcType;

    fdecl->name = std::move(name);

    if (!declParams.empty())
      fdecl->params = std::move(declParams);
    else {
      for (auto p : funcType->params) {
        auto paramDecl = make<VarDecl>();
        paramDecl->type = p;
        fdecl->params.push_back(paramDecl);
      }
    }

    if (ctx->initializer())
      ABORT();
    fdecl->body = nullptr;

    ret = fdecl;
  }

  else {
    auto vdecl = make<VarDecl>();
    auto type = make<Type>();
    vdecl->type = type;

    type->spec = sq.first;
    type->qual = sq.second;
    type->texp = texp;
    vdecl->name = std::move(name);

    if (auto p = ctx->initializer()){
      // vdecl->init = self(p); //origin
      auto raw = self(p);
      vdecl->init = lowerInit(type, raw);
    }
    else
      vdecl->init = nullptr;

    ret = vdecl;
  }

  // 这个实现允许符号重复定义，新定义会取代旧定义
  (*mSymtbl)[ret->name] = ret;
  return ret;
}

  //============================================================================
  // 辅助
  //============================================================================
Expr* Ast2Asg::lowerInit(Type* type, Expr* init)
{
  std::vector<Expr*> flat;

  flattenInit(type, init, flat);

  auto ret = make<InitListExpr>();
  ret->list = std::move(flat);
  return ret;
}

void Ast2Asg::flattenInit(Type* type, Expr* init, std::vector<Expr*>& out)
{
  // ========================
  // 1️⃣ 标量类型
  // ========================
  if (!type->texp || !type->texp->dcst<ArrayType>()) {
    if (auto list = init->dcst<InitListExpr>()) {
      if (list->list.empty())
        out.push_back(makeZero());
      else
        out.push_back(list->list[0]);
    } else {
      out.push_back(init);
    }
    return;
  }

  // ========================
  // 2️⃣ 数组类型
  // ========================
  auto arr = type->texp->dcst<ArrayType>();
  int N = arr->len;

  Type* subType = make<Type>();
  subType->spec = type->spec;
  subType->qual = type->qual;
  subType->texp = arr->sub;

  std::vector<Expr*> elems;

  if (auto list = init->dcst<InitListExpr>())
    elems = list->list;
  else
    elems = { init };

  int idx = 0;
  int i = 0;

  while (idx < N) {
    if (i >= elems.size()) {
      // 补零
      appendZero(subType, out);
      idx++;
      continue;
    }

    Expr* cur = elems[i];

    if (cur->dcst<InitListExpr>()) {
      flattenInit(subType, cur, out);
      i++;
    }
    else {
      // ⭐ flatten 关键：连续填充子数组
      if (arr->sub->dcst<ArrayType>()) {
        int subSize = eval_arrlen_from_type(arr->sub); 
        auto fake = make<InitListExpr>();

        for (int k = 0; k < subSize && i < elems.size(); k++, i++) {
          fake->list.push_back(elems[i]);
        }

        flattenInit(subType, fake, out);
      }
      else {
        flattenInit(subType, cur, out);
        i++;
      }
    }

    idx++;
  }
}

void Ast2Asg::appendZero(Type* type, std::vector<Expr*>& out)
{
  if (!type->texp || !type->texp->dcst<ArrayType>()) {
    out.push_back(makeZero());
    return;
  }

  auto arr = type->texp->dcst<ArrayType>();

  Type* subType = make<Type>();
  subType->spec = type->spec;
  subType->qual = type->qual;
  subType->texp = arr->sub;

  for (int i = 0; i < arr->len; i++) {
    appendZero(subType, out);
  }
}

Expr* Ast2Asg::makeZero()
{
  auto zero = make<IntegerLiteral>();
  zero->val = 0;
  return zero;
}

int Ast2Asg::eval_arrlen_from_type(TypeExpr* t)
{
  if (auto arr = t->dcst<ArrayType>()) {
    return arr->len * eval_arrlen_from_type(arr->sub);
  }
  return 1;
}

Expr* Ast2Asg::addImplicitCast(Expr* e, ImplicitCastExpr::Kind kind)
{
  auto node = make<ImplicitCastExpr>();
  node->kind = kind;
  node->sub = e;
  return node;
}


} // namespace asg
