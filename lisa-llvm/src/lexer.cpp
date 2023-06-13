/**
 * @file lexer.cpp
 * @version 0.1.1
 * @date 2022-12-29
 * 
 * @copyright Copyright Miracle Factory (c) 2023
 * 
 */

#include "lexer.h"


// initialize the lexer
Lexer::Lexer(const std::string& file) {
    this->file.open(file);
    if (!this->file.is_open()) {
        std::cerr << "Error: could not open file " 
            << file << std::endl;
        exit(1);
    }
    this->nl_buffer = false;
    this->ln = 1;
    this->col = 1;
}


// close the file
Lexer::~Lexer() {
    this->file.close();
}


// get one character from the file
// wrapper function for std::ifstream.get()
int Lexer::getChar() {
    if (this->nl_buffer) {
        this->ln++;
        this->col = 1;
        this->nl_buffer = false;
    }
    int c = this->file.get();
    if (c == '\n') {
        this->nl_buffer = true;
    }
    else {
        this->col++;
    }
    return c;
}


// peek the next character from the file
int Lexer::peekChar() {
    int c = this->file.peek();
    return c;
}


// skip a single line comment
void Lexer::skipLineComment() {
    // single line comment in lisa starts with %
    int c;
    do {
        c = getChar();
    } while (c != EOF && c != '\n' && c != '\r');
}


// skip a multi-line comment
int Lexer::skipBlockComment() {
    // block comment in lisa starts with %%
    // and it ends with %%
    int c;
    while((c = getChar()) != EOF) {
        if (c == '%') {
            if (peekChar() == '%') {
                getChar();
                break;
            }
        }
    }
    if (c == EOF)
        return 1;
    return 0;
}


// set the token to the corresponding keyword
void Lexer::matchKeywordToken(Token *t, const std::string& id) {
    if (id == "fn")
        setToken(t, TOK_FN, id);
    else if (id == "extern")
        setToken(t, TOK_EXTERN, id);
    else if (id == "if")
        setToken(t, TOK_IF, id);
    else if (id == "else")
        setToken(t, TOK_ELSE, id);
    else if (id == "for")
        setToken(t, TOK_FOR, id);
    else if (id == "while")
        setToken(t, TOK_WHILE, id);
    else if (id == "return")
        setToken(t, TOK_RETURN, id);
    else
        setToken(t, TOK_ID, id);
}


// setup the current token
void Lexer::setToken(Token *t, TokenType tp, const std::string& lx) {
    t->tp = tp;
    t->lx = lx;
    t->ln = this->ln;
    t->col = this->col;
}


// get the next token from the file
Token Lexer::getTok() {
    int c = ' ';
    std::string id;
    Token t;

    // Skip any whitespace.
    while (isspace(c))
        c = getChar();

    // Skip comments
    if (c == '%') {
        if (peekChar() == '%') {
            getChar();
            if (skipBlockComment()) {
                setToken(&t, TOK_ERR, "EOFinComment");
                return t;
            }
            return getTok();
        }
        else {
            skipLineComment();
            return getTok();
        }
    }

    // newline token
    // if (c == '\n') {
    //     setToken(&t, TOK_NEWLINE, "\\n");
    //     return t;
    // }

    // identifiers or keywords
    if (isalpha(c) || c == '_') { 
        id = c;
        while (isalnum((c = peekChar())) || c == '_') {
            getChar();
            id += c;
        }
        matchKeywordToken(&t, id);
    }

    // numbers (int / float / double)
    else if (isdigit(c) || c == '.') {
        string NumStr;
        NumStr += c;
        int count_dot = 0;
        while (isdigit(c = peekChar()) || c == '.') {
            getChar();
            NumStr += c;
            if (c == '.')
                count_dot++;
        }
        if (count_dot > 1)
            setToken(&t, TOK_ERR, NumStr);
        else
            setToken(&t, TOK_NUM, NumStr);
    }

    // string literals
    else if (c == '"') {
        string str;
        while ((c = getChar()) != EOF && c != '"')
            str += c;
        if (c == EOF)
            setToken(&t, TOK_ERR, "");
        else
            setToken(&t, TOK_STR, str);
    }

    // symbols
    else if (isSingleSymbol(c)) {
        id = c;
        if (isDoubleSymbol(c, peekChar())) {
            id += getChar();
            setToken(&t, TOK_SYM, id);
        }
        else
            setToken(&t, TOK_SYM, id);
    }

    // end of file
    else if (c == EOF)
        setToken(&t, TOK_EOF, "EOF");

    // unknown or illegal character
    else
        setToken(&t, TOK_ERR, "ILL");

    return t;
}


// peek the next token without consuming it
Token Lexer::peekTok() {
    std::streampos pos = this->file.tellg();
    int temp_ln = this->ln;
    int temp_col = this->col;
    Token t = getTok();
    this->file.seekg(pos);
    this->ln = temp_ln;
    this->col = temp_col;
    return t;
}


// #define TEST_LEXER
#ifdef TEST_LEXER
void printTok(Token t) {
    std::cout << "<" << getTokenTypeString(t.tp) << ", " << t.lx 
        << ", " << t.ln << ", " << t.col << ">" << std::endl;
}


int main() {
    Lexer *lex = new Lexer("../examples/simple.lisa");
    Token t;
    do {
        t = lex->getTok(lex);
        printTok(t);
    } while (t.tp != TOK_EOF);
    delete lex;
    return 0;
}
#endif
