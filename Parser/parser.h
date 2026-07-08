#pragma once

#include "../AST/ast.h"

#include <memory>
#include <map>

extern std::map<char, int> BinopPrecedence;
std::unique_ptr<ExprAST> ParseExpression();
std::unique_ptr<PrototypeAST> ParsePrototype();
std::unique_ptr<FunctionAST> ParseDefinition();
std::unique_ptr<PrototypeAST> ParseExtern();
std::unique_ptr<FunctionAST> ParseTopLevelExpr();

void MainLoop();