/**
 * @file codegen.cpp
 * @version 0.1.1
 * @date 2023-04-10
 * 
 * @copyright Copyright Miracle Factory (c) 2023
 * 
 */

#include "lexer.h"
#include "parser.h"
#include "ast.h"
using namespace llvm;


// error printing function
Value *codeGenError(const char *str) {
    fprintf(stderr, "\033[1;31mCode Gen Error:\033[0m %s\n", str);
    return nullptr;
}


// Code Generation Visitor
CodeGenVisitor::CodeGenVisitor() : 
    context(std::make_unique<LLVMContext>()), 
    builder(*context), 
    module(std::make_unique<Module>("lisa", *context)),
    fpm(std::make_unique<legacy::FunctionPassManager>(module.get()))
    // jit(std::make_unique<lisa::LisaJIT>()) 
    {
        fpm->add(createInstructionCombiningPass());
        fpm->add(createReassociatePass());
        fpm->add(createGVNPass());
        fpm->add(createCFGSimplificationPass());
        fpm->doInitialization();
        // module->setDataLayout(jit->getDataLayout());
    }


// for NumberExprAST
Value *CodeGenVisitor::visit(NumberExprAST *node) {
    return ConstantFP::get(*context, APFloat(node->val));
}


// for VariableExprAST
Value *CodeGenVisitor::visit(VariableExprAST *node) {
    Value *v = namedValues[node->name];
    if (!v)
        return codeGenError("Unknown variable name");
    return v;
}


// for BinaryExprAST
Value *CodeGenVisitor::visit(BinaryExprAST *node) {
    Value *lhs = node->lhs->accept(*this);
    Value *rhs = node->rhs->accept(*this);
    if (!lhs || !rhs)
        return nullptr;
    switch (node->op) {
        // case ':':
        //     ExprAST* lhsExpr = node->lhs.get();
        //     if (VariableExprAST *lhsVar = static_cast<VariableExprAST*>(lhsExpr)) {
        //         namedValues[lhsVar->name] = rhs;
        //         return rhs;
        //     } else {
        //         codeGenError("invalid left-hand side of assignment");
        //         return nullptr;
        //     }
        case '+':
            return builder.CreateFAdd(lhs, rhs, "addtmp");
        case '-':
            return builder.CreateFSub(lhs, rhs, "subtmp");
        case '*':
            return builder.CreateFMul(lhs, rhs, "multmp");
        case '/':
            return builder.CreateFDiv(lhs, rhs, "divtmp");
        case '<':
            lhs = builder.CreateFCmpULT(lhs, rhs, "cmptmp");
            return builder.CreateUIToFP(lhs, Type::getDoubleTy(*context), "booltmp");
        case '>':
            lhs = builder.CreateFCmpUGT(lhs, rhs, "cmptmp");
            return builder.CreateUIToFP(lhs, Type::getDoubleTy(*context), "booltmp");
        case '=':
            lhs = builder.CreateFCmpUEQ(lhs, rhs, "cmptmp");
            return builder.CreateUIToFP(lhs, Type::getDoubleTy(*context), "booltmp");
        default:
            codeGenError("invalid binary operator");
            return nullptr;
    }
}


// for CallExprAST
Value *CodeGenVisitor::visit(CallExprAST *node) {
    Function *calleeF = module->getFunction(node->callee);
    if (!calleeF)
        return codeGenError("Unknown function referenced");
    if (calleeF->arg_size() != node->args.size())
        return codeGenError("Incorrect # arguments passed");
    std::vector<Value *> argsV;
    for (auto &arg : node->args) {
        argsV.push_back(arg->accept(*this));
        if (!argsV.back())
            return nullptr;
    }
    return builder.CreateCall(calleeF, argsV, "calltmp");
}


// for FunctionAST
Function *CodeGenVisitor::visit(FunctionAST *node) {
    Function *theFunction = module->getFunction(node->proto->name);
    if (!theFunction)
        theFunction = node->proto->accept(*this);
    if (!theFunction)
        return nullptr;
    BasicBlock *bb = BasicBlock::Create(*context, "entry", theFunction);
    builder.SetInsertPoint(bb);
    namedValues.clear();
    for (auto &arg : theFunction->args())
        namedValues[string(arg.getName())] = &arg;
    if (Value *retVal = node->body->accept(*this)) {
        builder.CreateRet(retVal);
        verifyFunction(*theFunction);
        fpm->run(*theFunction); // function pass optimize
        return theFunction;
    }
    theFunction->eraseFromParent();
    return nullptr;
}


// for PrototypeAST
Function *CodeGenVisitor::visit(PrototypeAST *node) {
    std::vector<Type *> doubles(node->args.size(), Type::getDoubleTy(*context));
    FunctionType *ft = FunctionType::get(Type::getDoubleTy(*context), doubles, false);
    Function *f = Function::Create(ft, Function::ExternalLinkage, node->name, module.get());
    unsigned idx = 0;
    for (auto &arg : f->args())
        arg.setName(node->args[idx++]);
    return f;
}


// handle definition
static void handleDefinition(Lexer *lex, CodeGenVisitor *codegen) {
    if (auto fnAST = Definition(lex)) {
        if (auto *fnIR = fnAST->accept(*codegen)) {
            fprintf(stderr, "\033[1;34m->\033[0m Read function definition:\n");
            fnIR->print(errs());
        }
    } else {
        lex->getTok(lex);
    }
}


// handle extern
static void handleExtern(Lexer *lex, CodeGenVisitor *codegen) {
    if (auto protoAST = Extern(lex)) {
        if (auto *fnIR = protoAST->accept(*codegen)) {
            fprintf(stderr, "\033[1;34m->\033[0m Read extern:\n");
            fnIR->print(errs());
        }
    } else {
        lex->getTok(lex);
    }
}


// handle top-level expression
static void handleTopLevelExpr(Lexer *lex, CodeGenVisitor *codegen) {
    if (auto fnAST = TopLevelExpr(lex)) {
        if (auto *fnIR = fnAST->accept(*codegen)) {
            fprintf(stderr, "\033[1;34m->\033[0m Read top-level expression:\n");
            fnIR->print(errs());
        }
    } else {
        lex->getTok(lex);
    }
}


// main loop
static void mainLoop(Lexer *lex, CodeGenVisitor *codegen) {
    while (true) {
        Token t = lex->peekTok(lex);
        switch (t.tp) {
            case TOK_EOF:
                return;
            case TOK_FN:
                handleDefinition(lex, codegen);
                break;
            case TOK_EXTERN:
                handleExtern(lex, codegen);
                break;
            default:
                handleTopLevelExpr(lex, codegen);
                break;
        }
    }
}


#define TEST_CODEGEN
#ifdef TEST_CODEGEN
int main() {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
    auto *lex = new Lexer("../lisa_examples/simple.lisa");
    auto *codegen = new CodeGenVisitor();
    std::cout << "Lisa Compiler Ready >" << std::endl;
    mainLoop(lex, codegen);
    delete codegen;
    delete lex;
    return 0;
}
#endif
