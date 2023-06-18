/**
 * @file codegen.cpp
 * @version 0.1.1
 * @date 2023-04-10
 * 
 * @copyright Copyright Miracle Factory (c) 2023
 * 
 */

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
        // add all the passes
        // fpm->add(createPromoteMemoryToRegisterPass());
        fpm->add(createInstructionCombiningPass());
        fpm->add(createReassociatePass());
        fpm->add(createGVNPass());
        fpm->add(createCFGSimplificationPass());
        fpm->doInitialization();
        // module->setDataLayout(jit->getDataLayout());
    }


AllocaInst *CodeGenVisitor::createEntryBlockAlloca(Function *theFunction,
                                          const std::string &varName) {
    IRBuilder<> tmpB(&theFunction->getEntryBlock(),
                     theFunction->getEntryBlock().begin());
    return tmpB.CreateAlloca(Type::getDoubleTy(*context), nullptr, varName);
}


// for NumberExprAST
Value *CodeGenVisitor::visit(NumberExprAST *node) {
    return ConstantFP::get(*context, APFloat(node->val));
}


// for VariableExprAST
Value *CodeGenVisitor::visit(VariableExprAST *node) {
    // Value *v = namedValues[node->name];
    AllocaInst *v = namedValues[node->name];
    if (!v) {
        std::string err = "Undefined identifier: " + node->name;
        return codeGenError(err.c_str());
    }
    return builder.CreateLoad(v->getAllocatedType(), v, node->name.c_str());
}


// for BinaryExprAST
Value *CodeGenVisitor::visit(BinaryExprAST *node) {
    // assignment
    if (node->op == ':') {
        VariableExprAST *lhs = dynamic_cast<VariableExprAST*>(node->lhs.get());
        if (!lhs)
            return codeGenError("invalid assignment target");
        Value *rhsVal = node->rhs->accept(*this);
        if (!rhsVal)
            return nullptr;
        Value *lhsVal = namedValues[lhs->name];
        if (!lhsVal) {
            Function *theFunction = builder.GetInsertBlock()->getParent();
            AllocaInst *alloca = createEntryBlockAlloca(theFunction, lhs->name);
            builder.CreateStore(rhsVal, alloca);
            namedValues[lhs->name] = alloca;
            return rhsVal;
        }
        builder.CreateStore(rhsVal, lhsVal);
        return rhsVal;
    }

    // other binary operations
    Value *lhs = node->lhs->accept(*this);
    Value *rhs = node->rhs->accept(*this);
    if (!lhs || !rhs)
        return nullptr;
    switch (node->op) {
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


// for IfExprAST
Value *CodeGenVisitor::visit(IfExprAST *node) {
    Value *condV = node->cond->accept(*this);
    if (!condV)
        return nullptr;
    condV = builder.CreateFCmpONE(
        condV, ConstantFP::get(*context, APFloat(0.0)), "ifcond");
    Function *theFunction = builder.GetInsertBlock()->getParent();
    BasicBlock *ifBodyBB = BasicBlock::Create(*context, "ifbody", theFunction);
    BasicBlock *mergeBB = BasicBlock::Create(*context, "ifcont");
    BasicBlock *elseBodyBB = nullptr;
    if (node->els_body.size() > 0)
        elseBodyBB = BasicBlock::Create(*context, "elsebody");
    builder.CreateCondBr(condV, ifBodyBB, elseBodyBB ? elseBodyBB : mergeBB);

    // Generate code for the "if" body
    builder.SetInsertPoint(ifBodyBB);
    Value *ifBodyV = nullptr;
    for (auto &expr : node->if_body)
        ifBodyV = expr->accept(*this);
    builder.CreateBr(mergeBB);
    ifBodyBB = builder.GetInsertBlock();

    // Generate code for the "else" body (if it exists)
    Value *elseBodyV = nullptr;
    if (elseBodyBB) {
        theFunction->getBasicBlockList().push_back(elseBodyBB);
        builder.SetInsertPoint(elseBodyBB);
        for (auto &expr : node->els_body)
            elseBodyV = expr->accept(*this);
        builder.CreateBr(mergeBB);
        elseBodyBB = builder.GetInsertBlock();
    }

    // Generate code for the merge block
    theFunction->getBasicBlockList().push_back(mergeBB);
    builder.SetInsertPoint(mergeBB);
    PHINode *phiNode = builder.CreatePHI(Type::getDoubleTy(*context), 2, "iftmp");
    phiNode->addIncoming(ifBodyV, ifBodyBB);
    if (elseBodyBB)
        phiNode->addIncoming(elseBodyV, elseBodyBB);
    else
        phiNode->addIncoming(ConstantFP::get(*context, APFloat(0.0)), mergeBB);

    return phiNode;
}


// for ForExprAST
Value *CodeGenVisitor::visit(ForExprAST *node) {
    Function *theFunction = builder.GetInsertBlock()->getParent();
    AllocaInst *alloca = createEntryBlockAlloca(theFunction, node->var_name);
    Value *startVal = node->start->accept(*this);
    if (!startVal)
        return nullptr;
    builder.CreateStore(startVal, alloca);

    BasicBlock *loopBB = BasicBlock::Create(*context, "loop", theFunction);
    builder.CreateBr(loopBB);
    builder.SetInsertPoint(loopBB);
    AllocaInst *oldVal = namedValues[node->var_name];
    namedValues[node->var_name] = alloca;
    for (auto &expr : node->body) {
        Value *bodyV = expr->accept(*this);
        if (!bodyV) {
            namedValues[node->var_name] = oldVal;
            return nullptr;
        }
    }

    Value *stepVal = nullptr;
    if (node->step) {
        stepVal = node->step->accept(*this);
        if (!stepVal)
            return nullptr;
    } else {
        stepVal = ConstantFP::get(*context, APFloat(1.0));
    }

    Value *endCond = node->end->accept(*this);
    if (!endCond)
        return nullptr;
    Value *curVar = builder.CreateLoad(alloca->getAllocatedType(), 
                                       alloca, node->var_name.c_str());
    Value *nextVar = builder.CreateFAdd(curVar, stepVal, "nextvar");
    builder.CreateStore(nextVar, alloca);
    endCond = builder.CreateFCmpONE(endCond, nextVar, "loopcond");

    BasicBlock *afterBB = BasicBlock::Create(*context, "afterloop", theFunction);
    builder.CreateCondBr(endCond, loopBB, afterBB);
    builder.SetInsertPoint(afterBB);

    if (oldVal)
        namedValues[node->var_name] = oldVal;
    else
        namedValues.erase(node->var_name);
    return Constant::getNullValue(Type::getDoubleTy(*context));
}


// for ReturnExprAST
Value *CodeGenVisitor::visit(ReturnExprAST *node) {
    Value *retVal = node->expr->accept(*this);
    if (!retVal)
        return nullptr;
    builder.CreateRet(retVal);
    return retVal;
}


// for CallExprAST
Value *CodeGenVisitor::visit(CallExprAST *node) {
    Function *calleeF = module->getFunction(node->callee);
    if (!calleeF) {
        std::string err = "Unknown function referenced: " + node->callee;
        return codeGenError(err.c_str());
    }
    if (calleeF->arg_size() != node->args.size())
        return codeGenError("Incorrect number of arguments passed");
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
    for (auto &arg : theFunction->args()) {
        AllocaInst *alloca = createEntryBlockAlloca(
            theFunction, std::string(arg.getName()));
        builder.CreateStore(&arg, alloca);
        namedValues[std::string(arg.getName())] = alloca;
    }
    // code gen for function body
    for (auto &expr : node->body) {
        // if the expr is the last one, generate as return value
        if (expr == node->body.back()) {
            if (ReturnExprAST *ret = dynamic_cast<ReturnExprAST *>(expr.get())) {
                if (!expr->accept(*this)) {
                    theFunction->eraseFromParent();
                    return nullptr;
                }
            }
            else {
                Value *retVal = expr->accept(*this);
                if (!retVal) {
                    theFunction->eraseFromParent();
                    return nullptr;
                }
                builder.CreateRet(retVal);
            }
        }
        else if (!expr->accept(*this)) {
            theFunction->eraseFromParent();
            return nullptr;
        }
    }
    verifyFunction(*theFunction);
    fpm->run(*theFunction); // function pass optimization
    return theFunction;
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


// #define TEST_CODEGEN
#ifdef TEST_CODEGEN
int main() {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
    auto lex = std::make_unique<Lexer>("../examples/test_files/simple.lisa");
    auto codegen = std::make_unique<CodeGenVisitor>();
    std::cout << "Lisa Compiler Ready >" << std::endl;
    mainLoop(lex.get(), codegen.get());
    return 0;
}
#endif
