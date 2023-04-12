/**
 * @file lexer.h
 * @version 0.1.0
 * @date 2022-12-29
 * 
 * @copyright Copyright Miracle Factory (c) 2022
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
    Token getTok(Lexer *lex);
    Token peekTok(Lexer *lex);
private:
    static int getChar(Lexer *lex);
    static int peekChar(Lexer *lex);
    static void skipLineComment(Lexer *lex);
    static int skipBlockComment(Lexer *lex);
    // lexer state
    std::ifstream file; // input file
    bool nl_buffer;     // newline buffer
    int ln;             // line number
    int col;            // column number
};


#endif
