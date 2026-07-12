#include "Parser/parser.h"
#include "Lexer/lexer.h"
#include "AST/ast.h"

#include <cstdio>
#include <map>

std::map<char, int> BinopPrecedence;

int main() {
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40;

    InitializeModule();
    MainLoop();

    return 0;
}