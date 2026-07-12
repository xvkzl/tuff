#include "parser.h"
#include "../Lexer/lexer.h"

#include <cctype>
#include <cstdio>
#include <map>
#include <memory>
#include <vector>

#include <llvm/Support/raw_ostream.h>

static int CurTok;

int getNextToken() {
    CurTok = gettok();
    fprintf(stderr, "[PARSER] CurTok = %d\n", CurTok);
    return CurTok;
}


static int GetTokPrecedence() {
    if (!isascii(CurTok))
        return -1;
    int TokPrec = BinopPrecedence[CurTok];
    if (TokPrec <= 0)
        return -1;
    return TokPrec;
}


template <typename T>
std::unique_ptr<T> LogError(const char *Str) {
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}

static std::unique_ptr<ExprAST> ParseNumberExpr() {
    fprintf(stderr, "ParseNumberExpr\n");

    auto Result = std::make_unique<NumberExprAST>(NumVal);
    getNextToken();
    return Result;
}

static std::unique_ptr<ExprAST> ParseParenExpr() {
    getNextToken(); // eat '('
    auto V = ParseExpression();
    if (!V)
        return nullptr;

    if (CurTok != ')')
        return LogError<ExprAST>("expected ')'");
    getNextToken(); // eat ')'
    return V;
};
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
    std::string IdName = IdentifierStr;
    getNextToken(); // eat identifier
    if (CurTok != '(') // Simple variable ref.
        return std::make_unique<VariableExprAST>(IdName);

    getNextToken(); // eat '('
    std::vector<std::unique_ptr<ExprAST>> Args;

    if (CurTok != ')') {
        while (true) {
            if (auto Arg = ParseExpression())
                Args.push_back(std::move(Arg));
            else
                return nullptr;

            if (CurTok == ')')
                break;

            if (CurTok != ',')
                return LogError<ExprAST>("Expected ')' or ',' in argument list");
            getNextToken();
        }
    }

    getNextToken(); // eat ')'
    return std::make_unique<CallExprAST>(IdName, std::move(Args));
};
static std::unique_ptr<ExprAST> ParsePrimary() {
    switch (CurTok) {
        default:
            return LogError<ExprAST>("unknown token when expecting an expression");
        case tok_identifier:
            return ParseIdentifierExpr();
        case tok_number:
            return ParseNumberExpr();
        case '(':
            return ParseParenExpr();
    }
};

static std::unique_ptr<ExprAST> ParseBinOpRHS(
    int ExprPrec,
    std::unique_ptr<ExprAST> LHS) {
    while (true) {
        int TokPrec = GetTokPrecedence();
        if (TokPrec < ExprPrec)
            return LHS;

        int BinOp = CurTok;
        getNextToken(); // eat binop

        auto RHS = ParsePrimary();
        if (!RHS)
            return nullptr;

        int NextPrec = GetTokPrecedence();
        if (TokPrec < NextPrec) {
            RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
            if (!RHS)
                return nullptr;
        }

        LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
    }
}




std::unique_ptr<ExprAST> ParseExpression() {
    auto LHS = ParsePrimary();
    if (!LHS)
        return nullptr;

    return ParseBinOpRHS(0, std::move(LHS));
}

std::unique_ptr<PrototypeAST> ParsePrototype() {
    if (CurTok != tok_identifier)
        return LogError<PrototypeAST>("Expected function name in prototype");

    std::string FnName = IdentifierStr;
    getNextToken();

    if (CurTok != '(')
        return LogError<PrototypeAST>("Expected '(' in prototype");

    getNextToken(); // eat '('

    std::vector<std::string> ArgNames;

    while (CurTok != ')') {

        if (CurTok != tok_identifier)
            return LogError<PrototypeAST>("Expected parameter name");

        ArgNames.push_back(IdentifierStr);
        getNextToken();

        if (CurTok == ',')
            getNextToken();
        else if (CurTok != ')')
            return LogError<PrototypeAST>("Expected ',' or ')'");
    }

    getNextToken(); // eat ')'

    return std::make_unique<PrototypeAST>(
        FnName,
        std::move(ArgNames)
    );
}

std::unique_ptr<FunctionAST> ParseDefinition() {
    getNextToken(); // eat 'funk'

    auto Proto = ParsePrototype();
    if (!Proto)
        return nullptr;

    if (CurTok != '{')
        return LogError<FunctionAST>("Expected '{' in function definition");

    getNextToken(); // eat '{'

    std::vector<std::unique_ptr<ExprAST>> Body;

    while (CurTok != '}' && CurTok != tok_eof) {
        if (auto E = ParseExpression()) {
            Body.push_back(std::move(E));
            if (CurTok != ';')
                return LogError<FunctionAST>("Expected ';' after expression");
            getNextToken();

        } else {
            getNextToken(); // recovery step (important)
        }
    }

    if (CurTok != '}')
        return LogError<FunctionAST>("Expected '}' in function definition");

    getNextToken(); // eat '}'

    return std::make_unique<FunctionAST>(
        std::move(Proto),
        std::move(Body));
}

std::unique_ptr<PrototypeAST> ParseExtern() {
    getNextToken(); // eat 'dih'
    return ParsePrototype();
}

std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
    auto E = ParseExpression();
    if (!E)
        return nullptr;

    if (CurTok == ';')
        getNextToken();

    auto Proto = std::make_unique<PrototypeAST>(
        "__anon_expr",
        std::vector<std::string>()
    );

    std::vector<std::unique_ptr<ExprAST>> Body;
    Body.push_back(std::move(E));

    return std::make_unique<FunctionAST>(
        std::move(Proto),
        std::move(Body)
    );
}

void MainLoop() {
    while (true) {
        fprintf(stderr, "ready> ");

        getNextToken();

        switch (CurTok) {
        case tok_eof:
            return;

        case ';':
            break;

        case tok_funk:
            if (auto FnAST = ParseDefinition()) {
                if (auto *FnIR = FnAST->codegen()) {
                    FnIR->print(llvm::errs());
                    fprintf(stderr, "\n");
                }
            } else {
                fprintf(stderr, "Error in function definition.\n");
            }
            break;

        case tok_dih:
            if (auto ProtoAST = ParseExtern()) {
                if (auto *FnIR = ProtoAST->codegen()) {
                    FnIR->print(llvm::errs());
                    fprintf(stderr, "\n");
                }
            } else {
                fprintf(stderr, "Error in extern.\n");
            }
            break;

        default:
            if (auto FnAST = ParseTopLevelExpr()) {
                if (auto *FnIR = FnAST->codegen()) {
                    FnIR->print(llvm::errs());
                    fprintf(stderr, "\n");
                }
            } else {
                fprintf(stderr, "Error in expression.\n");
            }
            break;
        }
    }
}