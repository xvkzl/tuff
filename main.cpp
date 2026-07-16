#include "Parser/parser.h"
#include "Lexer/lexer.h"
#include "AST/ast.h"

#include <cstdio>
#include <map>
// std::unique_ptr<KetlangJIT> TheJIT;
std::map<char, int> BinopPrecedence;

int main() {
/*
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
*/
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40;

    InitializeModuleAndManagers();

/*  TheJIT = std::make_unique<KaleidoscopeJIT>(); */

    MainLoop();

    return 0;
}