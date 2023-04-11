/**
 * @file parser.h
 * @version 0.1.0
 * @date 2023-04-08
 * 
 * @copyright Copyright Miracle Factory (c) 2022
 * 
 */

#ifndef PARSER_H
#define PARSER_H

#pragma once

#include "ast.h"
#include "lexer.h"
using namespace std;


// declare all the parsing functions
unique_ptr<ExprAST> NumberExpr(Lexer *lex);
unique_ptr<ExprAST> ParenExpr(Lexer *lex);
unique_ptr<ExprAST> Expr(Lexer *lex);
unique_ptr<ExprAST> BinOpRHS(Lexer *lex, int exprPrec, 
                             unique_ptr<ExprAST> lhs);
unique_ptr<PrototypeAST> Prototype(Lexer *lex);
unique_ptr<FunctionAST> Definition(Lexer *lex);
unique_ptr<FunctionAST> TopLevelExpr(Lexer *lex);
unique_ptr<PrototypeAST> Extern(Lexer *lex);


#endif
