/**
 * @file token.h
 * @version 0.1.1
 * @date 2022-12-29
 * 
 * @copyright Copyright Miracle Factory (c) 2023
 * 
 */
#ifndef TOKEN_H
#define TOKEN_H

#pragma once

#include <string>
#include <map>
using std::string;


enum TokenType {
    TOK_EOF,
    TOK_ERR,
    // primary
    TOK_ID,
    TOK_NUM,
    TOK_SYM,
    TOK_STR,
    // commands
    TOK_FN,
    TOK_EXTERN,
};


struct Token {
    // Token metadata
    TokenType tp;
    string lx;
    int ln;
    int col;
};


static int setToken(Token *t, TokenType tp, string lx, int ln, int col) {
    t->tp = tp;
    t->lx = lx;
    t->ln = ln;
    t->col = col;
    return 0;
}


static bool isSingleSymbol(char c) {
    switch (c) {
        case '(': // parameter list start
        case ')': // parameter list end
        case '[': // list index start
        case ']': // list index end
        case '{': // code block start
        case '}': // code block end
        case '.': // namespace
        case ',': // comma
        case ':': // assignment
        case '+': // binary add
        case '-': // binary subtract
        case '*': // multiply
        case '/': // divide
        case '^': // exponent
        case '<': // less than
        case '>': // greater than
        case '=': // equal to
        case '!': // not
        case '&': // logical and
        case '|': // logical or
        case '~': // logical xor
            return true;
        default:
            return false;
    }
}


static bool isDoubleSymbol(char c1, char c2) {
    switch (c1) {
        case '+':
            switch (c2){
                case '+': // unary add
                case ':': // add and assign
                    return true;
            }
            break;
        case '-':
            switch (c2){
                case '-': // unary subtract
                case ':': // subtract and assign
                    return true;
            }
            break;
        case '*':
            switch (c2){
                case ':': // multiply and assign
                    return true;
            }
            break;
        case '/':
            switch (c2){
                case ':': // divide and assign
                    return true;
            }
            break;
        case '<':
            switch (c2){
                case '<': // left shift
                case '=': // less than or equal to
                    return true;
            }
            break;
        case '>':
            switch (c2){
                case '>': // right shift
                case '=': // greater than or equal to
                    return true;
            }
            break;
        case '!':
            switch (c2){
                case '=': // not equal to
                    return true;
            }
            break;
        default:
            return false;
    }
    return false;
}


// binop precedence
static std::map<char, int> binopPrecedence = {
    /*
        note lisa does not natively 
        support bitwise operators
        use functions AND(), OR(), XOR(), 
        NOT(), LSHIFT(), RSHIFT() instead
    */
    {':', 5}, // assignment
    {'<', 10}, // less than
    {'>', 10}, // greater than
    {'=', 10}, // equal to
    {'!', 15}, // logical not
    {'&', 15}, // logical and
    {'|', 15}, // logical or
    {'~', 15}, // logical xor
    {'+', 20}, // binary add
    {'-', 20}, // binary subtract
    {'*', 40}, // multiply
    {'/', 40}, // divide
    {'^', 80}, // exponent
};


static int getBinopPrecedence(char op) {
    if (binopPrecedence.find(op) != binopPrecedence.end()) {
        return binopPrecedence[op];
    }
    return -1;
}


static string getTokenTypeString(TokenType tp) {
    switch (tp) {
        case TOK_EOF: return "TOK_EOF";
        case TOK_ERR: return "TOK_ERR";
        case TOK_ID: return "TOK_ID";
        case TOK_NUM: return "TOK_NUM";
        case TOK_SYM: return "TOK_SYM";
        case TOK_STR: return "TOK_STR";
        case TOK_FN: return "TOK_FN";
        case TOK_EXTERN: return "TOK_EXTERN";
        default: return "TOK_UNKNOWN";
    }
}


#endif
