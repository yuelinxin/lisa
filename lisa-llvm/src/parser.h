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


// declare all the parsing functions
std::unique_ptr<ExprAST> NumberExpr(Lexer *lex);
std::unique_ptr<ExprAST> ParenExpr(Lexer *lex);
std::unique_ptr<ExprAST> Expr(Lexer *lex);
std::unique_ptr<ExprAST> BinOpRHS(Lexer *lex, int exprPrec,
                                  std::unique_ptr<ExprAST> lhs);
std::unique_ptr<PrototypeAST> Prototype(Lexer *lex);
std::unique_ptr<FunctionAST> Definition(Lexer *lex);
std::unique_ptr<FunctionAST> TopLevelExpr(Lexer *lex);
std::unique_ptr<PrototypeAST> Extern(Lexer *lex);


#endif
