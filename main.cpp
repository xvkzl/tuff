#include "Parser/parser.h"
#include "Lexer/lexer.h"
#include "AST/ast.h"

#include <cstdio>
#include <map>
#include <memory>

#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

std::unique_ptr<llvm::orc::LLJIT> TheJIT;
std::map<char, int> BinopPrecedence;

int main() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    auto J = llvm::orc::LLJITBuilder().create();
    if (!J) {
        llvm::errs() << llvm::toString(J.takeError()) << '\n';
        return 1;
    }

    TheJIT = std::move(*J);

    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40;

    InitializeModuleAndManagers();

    MainLoop();

    return 0;
}