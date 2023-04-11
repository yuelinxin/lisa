/**
 * @file lexer.cpp
 * @version 0.1.0
 * @date 2022-12-29
 * 
 * @copyright Copyright Miracle Factory (c) 2022
 * 
 */

#include "lexer.h"


// initialize the lexer
Lexer::Lexer(std::string file) {
    this->file.open(file);
    if (!this->file.is_open()) {
        std::cerr << "Error: could not open file " 
            << file << std::endl;
        exit(1);
    }
    this->nl_buffer = 0;
    this->ln = 1;
    this->col = 1;
}


// close the file
Lexer::~Lexer() {
    this->file.close();
}


// get one character from the file
// wrapper function for std::ifstream.get()
int Lexer::getChar(Lexer *lex) {
    if (lex->nl_buffer) {
        lex->ln++;
        lex->col = 1;
        lex->nl_buffer = false;
    }
    int c = lex->file.get();
    if (c == '\n') {
        lex->nl_buffer = true;
    }
    else {
        lex->col++;
    }
    return c;
}


// peek the next character from the file
int Lexer::peekChar(Lexer *lex) {
    int c = lex->file.peek();
    return c;
}


// skip a single line comment
void Lexer::skipLineComment(Lexer *lex) {
    // single line comment in lisa starts with %
    int c;
    do {
        c = getChar(lex);
    } while (c != EOF && c != '\n' && c != '\r');
}


// skip a multi-line comment
int Lexer::skipBlockComment(Lexer *lex) {
    // block comment in lisa starts with %%
    // and it ends with %%
    int c;
    while((c = getChar(lex)) != EOF) {
        if (c == '%') {
            if (peekChar(lex) == '%') {
                getChar(lex);
                break;
            }
        }
    }
    if (c == EOF)
        return 1;
    return 0;
}


// get the next token from the file
Token Lexer::getTok(Lexer *lex) {
    int c = ' ';
    std::string id;
    Token t;

    // Skip any whitespace.
    while (isspace(c))
        c = getChar(lex);

    // Skip comments
    if (c == '%') {
        if (peekChar(lex) == '%') {
            getChar(lex);
            if (skipBlockComment(lex)) {
                setToken(&t, TOK_ERR, "EOFinComment", 
                    lex->ln, lex->col);
                return t;
            }
            return getTok(lex);
        }
        else {
            skipLineComment(lex);
            return getTok(lex);
        }
    }

    // identifiers or keywords
    if (isalpha(c) || c == '_') { 
        id = c;
        while (isalnum((c = peekChar(lex))) || c == '_') {
            getChar(lex);
            id += c;
        }
        if (id == "fn")
            setToken(&t, TOK_FN, id, lex->ln, lex->col);
        else if (id == "extern")
            setToken(&t, TOK_EXTERN, id, lex->ln, lex->col);
        else
            setToken(&t, TOK_ID, id, lex->ln, lex->col);
    }

    // numbers (int / float / double)
    else if (isdigit(c) || c == '.') {
        string NumStr;
        NumStr += c;
        int count_dot = 0;
        while (isdigit(c = peekChar(lex)) || c == '.') {
            getChar(lex);
            NumStr += c;
            if (c == '.')
                count_dot++;
        }
        if (count_dot > 1)
            setToken(&t, TOK_ERR, NumStr, lex->ln, lex->col);
        else
            setToken(&t, TOK_NUM, NumStr, lex->ln, lex->col);
    }

    // string literals
    else if (c == '"') {
        string str;
        while ((c = getChar(lex)) != EOF && c != '"')
            str += c;
        if (c == EOF)
            setToken(&t, TOK_ERR, "", lex->ln, lex->col);
        else
            setToken(&t, TOK_STR, str, lex->ln, lex->col);
    }

    // symbols
    else if (isSingleSymbol(c)) {
        id = c;
        if (isDoubleSymbol(c, peekChar(lex))) {
            id += getChar(lex);
            setToken(&t, TOK_SYM, id, lex->ln, lex->col);
        }
        else
            setToken(&t, TOK_SYM, id, lex->ln, lex->col);
    }

    // end of file
    else if (c == EOF)
        setToken(&t, TOK_EOF, "EOF", lex->ln, lex->col);

    // unknown or illegal character
    else
        setToken(&t, TOK_ERR, "ILL", lex->ln, lex->col);

    return t;
}


// peek the next token without consuming it
Token Lexer::peekTok(Lexer *lex) {
    std::streampos pos = lex->file.tellg();
    int temp_ln = lex->ln;
    int temp_col = lex->col;
    Token t = getTok(lex);
    lex->file.seekg(pos);
    lex->ln = temp_ln;
    lex->col = temp_col;
    return t;
}


// #ifndef PRODUCTION
// void printTok(Token t) {
//     std::cout << "<" << getTokenTypeString(t.tp) << ", " << t.lx 
//         << ", " << t.ln << ", " << t.col << ">" << std::endl;
// }


// int main() {
//     Lexer *lex = new Lexer("test.lisa");
//     Token t;
//     do {
//         t = lex->getTok(lex);
//         printTok(t);
//     } while (t.tp != TOK_EOF);
//     delete lex;
//     return 0;
// }
// #endif
