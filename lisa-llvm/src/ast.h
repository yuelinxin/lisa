/**
 * @file ast.h
 * @version 0.1.2
 * @date 2023-04-08
 * 
 * @copyright Copyright Miracle Factory (c) 2023
 * 
 */

#ifndef AST_H
#define AST_H

#pragma once

#include <string>
#include <utility>
#include <vector>
#include <map>
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "jit.h"
using namespace llvm;


// define all the AST nodes
class ExprAST;
class NumberExprAST;
class VariableExprAST;
class BinaryExprAST;
class IfExprAST;
class ForExprAST;
class WhileExprAST;
class ReturnExprAST;
class CallExprAST;
class PrototypeAST;
class FunctionAST;


class CodeGenVisitor 
{
private:
    std::unique_ptr<LLVMContext> context;
    IRBuilder<> builder;
    std::unique_ptr<Module> module;
    std::unique_ptr<legacy::FunctionPassManager> fpm;
    std::unique_ptr<lisa::LisaJIT> jit;
    std::map<std::string, AllocaInst*> namedValues;
public:
    CodeGenVisitor();
    virtual ~CodeGenVisitor() = default;
    Module* borrowModule() {return module.get();}
    AllocaInst* createEntryBlockAlloca(Function *theFunction, 
                                       const std::string &varName);
    virtual Value* visit(NumberExprAST *node);
    virtual Value* visit(VariableExprAST *node);
    virtual Value* visit(BinaryExprAST *node);
    virtual Value* visit(IfExprAST *node);
    virtual Value* visit(ForExprAST *node);
    virtual Value* visit(WhileExprAST *node);
    virtual Value* visit(ReturnExprAST *node);
    virtual Value* visit(CallExprAST *node);
    virtual Function* visit(PrototypeAST *node);
    virtual Function* visit(FunctionAST *node);
};


class ExprAST
{
public:
    virtual ~ExprAST() = default;
    virtual Value* accept(CodeGenVisitor &v) = 0;
};


class NumberExprAST : public ExprAST
{
public:
    double val;
public:
    explicit NumberExprAST(const std::string& valStr) {
        val = strtod(valStr.c_str(), nullptr);
    }
    Value* accept(CodeGenVisitor &v) override {
        return v.visit(this);
    }
};


class VariableExprAST : public ExprAST
{
public:
    std::string name;
public:
    explicit VariableExprAST(std::string name) : name(std::move(name)) {}
    Value* accept(CodeGenVisitor &v) override {
        return v.visit(this);
    }
};


class BinaryExprAST : public ExprAST
{
public:
    char op;
    std::unique_ptr<ExprAST> lhs, rhs;
public:
    BinaryExprAST(char op, std::unique_ptr<ExprAST>lhs,
                  std::unique_ptr<ExprAST>rhs) :
        op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    Value* accept(CodeGenVisitor &v) override {
        return v.visit(this);
    }
};


class IfExprAST : public ExprAST
{
public:
    std::unique_ptr<ExprAST> cond;
    std::vector<std::unique_ptr<ExprAST>> if_body, els_body;
public:
    IfExprAST(std::unique_ptr<ExprAST> cond, 
              std::vector<std::unique_ptr<ExprAST>> if_body,
              std::vector<std::unique_ptr<ExprAST>> els_body) :
        cond(std::move(cond)), if_body(std::move(if_body)), 
        els_body(std::move(els_body)) {}
    Value* accept(CodeGenVisitor &v) override {
        return v.visit(this);
    }
};


class ForExprAST : public ExprAST
{
public:
    std::string var_name;
    std::unique_ptr<ExprAST> start, end, step;
    std::vector<std::unique_ptr<ExprAST>> body;
public:
    ForExprAST(std::string var_name, std::unique_ptr<ExprAST> start,
               std::unique_ptr<ExprAST> end, std::unique_ptr<ExprAST> step,
               std::vector<std::unique_ptr<ExprAST>> body) :
        var_name(std::move(var_name)), start(std::move(start)),
        end(std::move(end)), step(std::move(step)), body(std::move(body)) {}
    Value* accept(CodeGenVisitor &v) override {
        return v.visit(this);
    }
};


class WhileExprAST : public ExprAST
{
public:
    std::unique_ptr<ExprAST> cond;
    std::vector<std::unique_ptr<ExprAST>> body;
public:
    WhileExprAST(std::unique_ptr<ExprAST> cond,
                 std::vector<std::unique_ptr<ExprAST>> body) :
        cond(std::move(cond)), body(std::move(body)) {}
    Value* accept(CodeGenVisitor &v) override {
        return v.visit(this);
    }
};


class ReturnExprAST : public ExprAST
{
public:
    std::unique_ptr<ExprAST> expr;
public:
    explicit ReturnExprAST(std::unique_ptr<ExprAST> expr) :
        expr(std::move(expr)) {}
    Value* accept(CodeGenVisitor &v) override {
        return v.visit(this);
    }
};


class CallExprAST : public ExprAST
{
public:
    std::string callee;
    std::vector<std::unique_ptr<ExprAST>> args;
public:
    CallExprAST(std::string callee,
                std::vector<std::unique_ptr<ExprAST>> args) :
        callee(std::move(callee)), args(std::move(args)) {}
    Value* accept(CodeGenVisitor &v) override {
        return v.visit(this);
    }
};


class PrototypeAST
{
public:
    std::string name;
    std::vector<std::string> args;
public:
    PrototypeAST(std::string name, std::vector<std::string> args) :
        name(std::move(name)), args(std::move(args)) {}
    const std::string& getName() const { return name; }
    Function* accept(CodeGenVisitor &v) {
        return v.visit(this);
    }
};


class FunctionAST
{
public:
    std::unique_ptr<PrototypeAST> proto;
    std::vector<std::unique_ptr<ExprAST>> body;
public:
    FunctionAST(std::unique_ptr<PrototypeAST> proto,
                std::vector<std::unique_ptr<ExprAST>> body) :
        proto(std::move(proto)), body(std::move(body)) {}
    Function* accept(CodeGenVisitor &v) {
        return v.visit(this);
    }
};


#endif
