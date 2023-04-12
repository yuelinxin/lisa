/**
 * @file ast.h
 * @version 0.1.0
 * @date 2023-04-08
 * 
 * @copyright Copyright Miracle Factory (c) 2022
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
using namespace llvm;


// define all the AST nodes
class ExprAST;
class NumberExprAST;
class VariableExprAST;
class BinaryExprAST;
class CallExprAST;
class PrototypeAST;
class FunctionAST;


class CodeGenVisitor 
{
    std::unique_ptr<LLVMContext> context;
    IRBuilder<> builder;
    std::unique_ptr<Module> module;
    std::map<std::string, Value*> namedValues;
public:
    CodeGenVisitor();
    virtual ~CodeGenVisitor() = default;
    virtual Value* visit(NumberExprAST *node);
    virtual Value* visit(VariableExprAST *node);
    virtual Value* visit(BinaryExprAST *node);
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
        op(op), lhs(move(lhs)), rhs(move(rhs)) {}
    Value* accept(CodeGenVisitor &v) override {
        return v.visit(this);
    }
};


class CallExprAST : public ExprAST
{
public:
    std::string callee;
    std::vector<std::unique_ptr<ExprAST> > args;
public:
    CallExprAST(std::string callee,
                std::vector<std::unique_ptr<ExprAST> > args) :
        callee(std::move(callee)), args(move(args)) {}
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
        name(std::move(name)), args(move(args)) {}
    const std::string& getName() const { return name; }
    Function* accept(CodeGenVisitor &v) {
        return v.visit(this);
    }
};


class FunctionAST
{
public:
    std::unique_ptr<PrototypeAST> proto;
    std::unique_ptr<ExprAST> body;
public:
    FunctionAST(std::unique_ptr<PrototypeAST> proto,
                std::unique_ptr<ExprAST> body) :
        proto(move(proto)), body(move(body)) {}
    Function* accept(CodeGenVisitor &v) {
        return v.visit(this);
    }
};


#endif
