/**
 * @file parser.h
 * @version 0.1.2
 * @date 2023-04-08
 * 
 * @copyright Copyright Yuelin Xin (c) 2024
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
std::unique_ptr<ExprAST> IdentifierExpr(Lexer *lex);
std::unique_ptr<ExprAST> Primary(Lexer *lex);
std::unique_ptr<ExprAST> Expr(Lexer *lex);
std::unique_ptr<ExprAST> BinOpRHS(Lexer *lex, int exprPrec,
                                  std::unique_ptr<ExprAST> lhs);
std::unique_ptr<IfExprAST> IfExpr(Lexer *lex);
std::unique_ptr<ForExprAST> ForExpr(Lexer *lex);
std::unique_ptr<WhileExprAST> WhileExpr(Lexer *lex);
std::unique_ptr<ReturnExprAST> ReturnExpr(Lexer *lex);
std::unique_ptr<PrototypeAST> Prototype(Lexer *lex);
std::unique_ptr<FunctionAST> Definition(Lexer *lex);
std::unique_ptr<FunctionAST> TopLevelExpr(Lexer *lex);
std::unique_ptr<PrototypeAST> Extern(Lexer *lex);


#endif
