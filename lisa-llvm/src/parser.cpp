/**
 * @file parser.cpp
 * @version 0.1.0
 * @date 2022-12-29
 * 
 * @copyright Copyright Miracle Factory (c) 2022
 * 
 */

#include "parser.h"
#include <map>
#include <memory>
using namespace std;


#define MATCH_TOK(type, lexeme) t.tp == type && t.lx == lexeme
#define GET_TOK t = GetTok(lex); if (t.tp == TOK_ERR) {parseError("token error", t); return nullptr;}
#define PEEK_TOK t = PeekTok(lex); if (t.tp == TOK_ERR) {parseError("token error", t); return nullptr;}
#define ERROR(msg) {parseError(msg, t); return nullptr;}


// error printing function
void parseError(const char *str, Token t) {
    fprintf(stderr, 
            "\033[1;31mError:\033[0m \"%s\": %s (line %d, column %d)\n", 
            t.lx.c_str(), str, t.ln, t.col);
}


// getTok wrapper function
Token GetTok(Lexer *lex) {
    Token t = lex->getTok(lex);
    // cout << "token: " << t.lx << ", " 
    // << getTokenTypeString(t.tp) << endl;
    return t;
}


// peekTok wrapper function
Token PeekTok(Lexer *lex) {
    Token t = lex->peekTok(lex);
    return t;
}


// number expression -> number
unique_ptr<ExprAST> NumberExpr(Lexer *lex) {
    Token t;
    GET_TOK;
    auto result = make_unique<NumberExprAST>(t.lx);
    return move(result);
}


// paren expression -> "(" expression ")"
unique_ptr<ExprAST> ParenExpr(Lexer *lex) {
    Token t;
    GET_TOK;
    if (!(MATCH_TOK(TOK_SYM, "(")))
        ERROR("Expected '('");
    auto res = Expr(lex);
    if (!res)
        return nullptr;
    GET_TOK;
    if (!(MATCH_TOK(TOK_SYM, ")")))
        ERROR("Expected ')'");
    return res;
}


// identifier expression -> ID | ID "(" expression* ")"
unique_ptr<ExprAST> IdentifierExpr(Lexer *lex) {
    Token t;
    GET_TOK; // t is ID
    string idName = t.lx;
    if (t.tp != TOK_ID)
        ERROR("Expected identifier");
    PEEK_TOK; // t is "(" or something else
    if (!(MATCH_TOK(TOK_SYM, "(")))
        return make_unique<VariableExprAST>(idName);
    GET_TOK; // t is "("
    PEEK_TOK; // t is ")" or expression
    vector<unique_ptr<ExprAST> > args;
    if (MATCH_TOK(TOK_SYM, ")")) {
        GET_TOK; // t is ")"
    }
    else {
        while (true) {
            auto arg = Expr(lex);
            if (!arg)
                return nullptr;
            args.push_back(move(arg));
            PEEK_TOK; // t is ")" or ","
            if (MATCH_TOK(TOK_SYM, ")")) {
                GET_TOK; // t is ")"
                break;
            }
            if (!(MATCH_TOK(TOK_SYM, ",")))
                ERROR("Expected ')' or ',' in argument list");
            GET_TOK; // t is ","
        }
    }
    return make_unique<CallExprAST>(idName, move(args));
}


// primary -> number expr | paren expr | identifier expr
unique_ptr<ExprAST> Primary(Lexer *lex) {
    Token t;
    PEEK_TOK;
    if (t.tp == TOK_NUM)
        return NumberExpr(lex);
    if (MATCH_TOK(TOK_SYM, "("))
        return ParenExpr(lex);
    if (t.tp == TOK_ID)
        return IdentifierExpr(lex);
    if (t.tp == TOK_EOF)
        return nullptr;
    else
        ERROR("Illegal token when expecting an expression");
}


// expression -> primary binoprhs
unique_ptr<ExprAST> Expr(Lexer *lex) {
    auto lhs = Primary(lex);
    if (!lhs)
        return nullptr;
    return BinOpRHS(lex, 0, move(lhs));
}


// binoprhs -> ("+" primary)*
unique_ptr<ExprAST> BinOpRHS(Lexer *lex, int exprPrec, 
                                    unique_ptr<ExprAST> lhs) {
    while (true) {
        Token t;
        PEEK_TOK; // t is operator
        int tokPrec = getBinopPrecedence(t.lx[0]);
        if (tokPrec < exprPrec)
            return lhs;
        GET_TOK // t is operator
        char binop = t.lx[0];
        auto rhs = Primary(lex);
        if (!rhs)
            return nullptr;
        PEEK_TOK; // t is operator
        int nextPrec = getBinopPrecedence(t.lx[0]);
        if (tokPrec < nextPrec) {
            rhs = BinOpRHS(lex, tokPrec + 1, move(rhs));
            if (!rhs)
                return nullptr;
        }
        lhs = make_unique<BinaryExprAST>(binop, move(lhs), move(rhs));
    }
}


// prototype -> ID "(" ID* ")"
unique_ptr<PrototypeAST> Prototype(Lexer *lex) {
    Token t;
    GET_TOK // t is ID
    if (t.tp != TOK_ID)
        ERROR("Expected function name in prototype");
    string fnName = t.lx;
    GET_TOK; // t is "("
    if (!(MATCH_TOK(TOK_SYM, "(")))
        ERROR("Expected '(' in prototype");
    vector<string> argNames;
    GET_TOK; // t is ")" or ID
    if (!(MATCH_TOK(TOK_SYM, ")"))) {
        while (true) {
            if (t.tp != TOK_ID)
                ERROR("Expected identifier in argument list");
            argNames.push_back(t.lx);
            GET_TOK;
            if (MATCH_TOK(TOK_SYM, ")"))
                break;
            if (!(MATCH_TOK(TOK_SYM, ",")))
                ERROR("Expected ',' between arguments");
            GET_TOK;
        }
    }
    if (!(MATCH_TOK(TOK_SYM, ")")))
        ERROR("Expected ')' in prototype");
    return make_unique<PrototypeAST>(fnName, move(argNames));
}


// definition -> "fn" prototype "{" expression "}"
unique_ptr<FunctionAST> Definition(Lexer *lex) {
    Token t;
    GET_TOK; // t is "fn"
    if (!(MATCH_TOK(TOK_FN, "fn")))
        ERROR("Expected 'fn' in definition");
    auto proto = Prototype(lex);
    if (!proto)
        return nullptr;
    GET_TOK; // t is "{"
    if (!(MATCH_TOK(TOK_SYM, "{")))
        ERROR("Expected '{' in definition");
    auto e = Expr(lex);
    if (!e)
        ERROR("Expected expression in definition");
    GET_TOK; // t is "}"
    if (!(MATCH_TOK(TOK_SYM, "}")))
        ERROR("Expected '}' in definition");
    return make_unique<FunctionAST>(move(proto), move(e));
}


// external -> "extern" prototype
unique_ptr<PrototypeAST> Extern(Lexer *lex) {
    Token t;
    GET_TOK; // t is "extern"
    if (!(MATCH_TOK(TOK_EXTERN, "extern")))
        ERROR("Expected 'extern' in extern");
    return Prototype(lex);
}


// toplevel expr -> expression
unique_ptr<FunctionAST> TopLevelExpr(Lexer *lex) {
    if (auto e = Expr(lex)) {
        auto proto = make_unique<PrototypeAST>("", vector<string>());
        return make_unique<FunctionAST>(move(proto), move(e));
    }
    return nullptr;
}


// #ifndef PRODUCTION
// int main() {
//     Lexer *lex = new Lexer("simple.lisa");
//     cout << "Lisa Compiler Ready >" << endl;
//     mainLoop(lex);
//     cout << "Parsing Complete." << endl;
//     delete lex;
//     return 0;
// }
// #endif
