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
using namespace std;
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
    unique_ptr<LLVMContext> context;
    IRBuilder<> builder;
    unique_ptr<Module> module;
    map<string, Value*> namedValues;
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
    NumberExprAST(string valStr) {
        val = strtod(valStr.c_str(), 0);
    }
    Value* accept(CodeGenVisitor &v) override {
        return v.visit(this);
    }
};


class VariableExprAST : public ExprAST
{
public:
    string name;
public:
    VariableExprAST(string name) : name(name) {}
    Value* accept(CodeGenVisitor &v) override {
        return v.visit(this);
    }
};


class BinaryExprAST : public ExprAST
{
public:
    char op;
    unique_ptr<ExprAST> lhs, rhs;
public:
    BinaryExprAST(char op, unique_ptr<ExprAST>lhs, 
                  unique_ptr<ExprAST>rhs) : 
        op(op), lhs(move(lhs)), rhs(move(rhs)) {}
    Value* accept(CodeGenVisitor &v) override {
        return v.visit(this);
    }
};


class CallExprAST : public ExprAST
{
public:
    string callee;
    vector<unique_ptr<ExprAST> > args;
public:
    CallExprAST(string callee, 
                vector<unique_ptr<ExprAST> > args) : 
        callee(callee), args(move(args)) {}
    Value* accept(CodeGenVisitor &v) override {
        return v.visit(this);
    }
};


class PrototypeAST
{
public:
    string name;
    vector<string> args;
public:
    PrototypeAST(string name, vector<string> args) : 
        name(name), args(move(args)) {}
    const string& getName() const { return name; }
    Function* accept(CodeGenVisitor &v) {
        return v.visit(this);
    }
};


class FunctionAST
{
public:
    unique_ptr<PrototypeAST> proto;
    unique_ptr<ExprAST> body;
public:
    FunctionAST(unique_ptr<PrototypeAST> proto, 
                unique_ptr<ExprAST> body) : 
        proto(move(proto)), body(move(body)) {}
    Function* accept(CodeGenVisitor &v) {
        return v.visit(this);
    }
};


#endif
