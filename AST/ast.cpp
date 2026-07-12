#include "ast.h"

#include <cstdio>
#include <map>

#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

static std::unique_ptr<llvm::LLVMContext> TheContext;
static std::unique_ptr<llvm::IRBuilder<>> Builder;
static std::unique_ptr<llvm::Module> TheModule;
static std::map<std::string, llvm::Value *> NamedValues;

void InitializeModule() {
    TheContext = std::make_unique<llvm::LLVMContext>();
    TheModule = std::make_unique<llvm::Module>("Ketlang", *TheContext);
    Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);
}

llvm::Value *LogErrorV(const char *Str) {
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}

llvm::Value *NumberExprAST::codegen() {
    return llvm::ConstantFP::get(*TheContext, llvm::APFloat(Val));
}

llvm::Value *VariableExprAST::codegen() {
    llvm::Value *V = NamedValues[Name];
    if (!V)
        return LogErrorV("Unknown variable name");
    return V;
}

llvm::Value *BinaryExprAST::codegen() {
    llvm::Value *L = LHS->codegen();
    llvm::Value *R = RHS->codegen();

    if (!L || !R)
        return nullptr;

    switch (Op) {
    case '+':
        return Builder->CreateFAdd(L, R, "addtmp");
    case '-':
        return Builder->CreateFSub(L, R, "subtmp");
    case '*':
        return Builder->CreateFMul(L, R, "multmp");
    case '/':
        return Builder->CreateFDiv(L, R, "divtmp");
    default:
        return LogErrorV("Invalid binary operator");
    }
}

llvm::Value *CallExprAST::codegen() {
    llvm::Function *CalleeF = TheModule->getFunction(Callee);

    if (!CalleeF)
        return LogErrorV("Unknown function referenced");

    if (CalleeF->arg_size() != Args.size())
        return LogErrorV("Incorrect # arguments passed to function");

    std::vector<llvm::Value *> ArgsV;
    for (auto &Arg : Args) {
        llvm::Value *V = Arg->codegen();
        if (!V)
            return nullptr;
        ArgsV.push_back(V);
    }

    return Builder->CreateCall(
        CalleeF->getFunctionType(),
        CalleeF,
        ArgsV,
        "calltmp");
}

llvm::Function *PrototypeAST::codegen() {
    // Reuse an existing declaration if one already exists.
    if (llvm::Function *F = TheModule->getFunction(Name))
        return F;

    std::vector<llvm::Type *> Doubles(
        Args.size(),
        llvm::Type::getDoubleTy(*TheContext));

    llvm::FunctionType *FT =
        llvm::FunctionType::get(
            llvm::Type::getDoubleTy(*TheContext),
            Doubles,
            false);

    llvm::Function *F =
        llvm::Function::Create(
            FT,
            llvm::Function::ExternalLinkage,
            Name,
            TheModule.get());

    unsigned Idx = 0;
    for (auto &Arg : F->args())
        Arg.setName(Args[Idx++]);

    return F;
}

llvm::Function *FunctionAST::codegen() {
    // Replace the previous anonymous expression.
    if (Proto->getName() == "__anon_expr") {
        if (llvm::Function *Old =
                TheModule->getFunction("__anon_expr"))
            Old->eraseFromParent();
    }

    llvm::Function *TheFunction =
        TheModule->getFunction(Proto->getName());

    if (!TheFunction)
        TheFunction = Proto->codegen();

    if (!TheFunction)
        return nullptr;

    // Prevent redefinition of named functions.
    if (!TheFunction->empty()) {
        if (Proto->getName() == "__anon_expr") {
            TheFunction->eraseFromParent();
            TheFunction = Proto->codegen();
            if (!TheFunction)
                return nullptr;
        } else {
            return (llvm::Function *)LogErrorV(
                "Function cannot be redefined");
        }
    }

    llvm::BasicBlock *BB =
        llvm::BasicBlock::Create(
            *TheContext,
            "entry",
            TheFunction);

    Builder->SetInsertPoint(BB);

    NamedValues.clear();

    for (auto &Arg : TheFunction->args())
        NamedValues[std::string(Arg.getName())] = &Arg;

    llvm::Value *RetVal = nullptr;

    for (auto &Expr : Body) {
        RetVal = Expr->codegen();
        if (!RetVal)
            break;
    }

    if (!RetVal) {
        TheFunction->eraseFromParent();
        return nullptr;
    }

    Builder->CreateRet(RetVal);

    if (llvm::verifyFunction(*TheFunction, &llvm::errs())) {
        TheFunction->eraseFromParent();
        return (llvm::Function *)LogErrorV(
            "Function verification failed");
    }

    return TheFunction;
}