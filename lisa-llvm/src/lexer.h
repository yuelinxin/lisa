/**
 * @file lexer.h
 * @version 0.1.1
 * @date 2022-12-29
 * 
 * @copyright Copyright Miracle Factory (c) 2023
 * 
 */

#ifndef LEXER_H
#define LEXER_H

#pragma once

#include <iostream>
#include <fstream>
#include "token.h"


class Lexer 
{
public:
    explicit Lexer(const std::string& file);
    ~Lexer();
    Token getTok();
    Token peekTok();
    Token peekNTok(int n);
private:
    int getChar();
    int peekChar();
    void skipLineComment();
    int skipBlockComment();
    void matchKeywordToken(Token *t, const std::string& id);
    void setToken(Token *t, TokenType tp, const std::string& lx);
    // lexer state
    std::ifstream file; // input file
    bool nl_buffer;     // newline buffer
    int ln;             // line number
    int col;            // column number
};


#endif
