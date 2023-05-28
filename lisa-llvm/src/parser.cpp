/**
 * @file parser.cpp
 * @version 0.1.1
 * @date 2022-12-29
 * 
 * @copyright Copyright Miracle Factory (c) 2023
 * 
 */

#include "parser.h"
#include <memory>


#define MATCH_TOK(type, lexeme) t.tp == (type) && t.lx == (lexeme)
#define GET_TOK t = GetTok(lex); if (t.tp == TOK_ERR) {parseError("token error", t); return nullptr;}
#define PEEK_TOK t = PeekTok(lex); if (t.tp == TOK_ERR) {parseError("token error", t); return nullptr;}
#define ERROR(msg) {parseError((msg), t); return nullptr;}


// error printing function
void parseError(const char *str, const Token& t) {
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
std::unique_ptr<ExprAST> NumberExpr(Lexer *lex) {
    Token t;
    GET_TOK
    auto result = std::make_unique<NumberExprAST>(t.lx);
    return std::move(result);
}


// paren expression -> "(" expression ")"
std::unique_ptr<ExprAST> ParenExpr(Lexer *lex) {
    Token t;
    GET_TOK
    if (!(MATCH_TOK(TOK_SYM, "(")))
        ERROR("Expected '('")
    auto res = Expr(lex);
    if (!res)
        return nullptr;
    GET_TOK
    if (!(MATCH_TOK(TOK_SYM, ")")))
        ERROR("Expected ')'")
    return res;
}


// identifier expression -> ID | ID "(" expression* ")"
std::unique_ptr<ExprAST> IdentifierExpr(Lexer *lex) {
    Token t;
    GET_TOK // t is ID
    string idName = t.lx;
    if (t.tp != TOK_ID)
        ERROR("Expected identifier")
    PEEK_TOK // t is "(" or something else
    if (!(MATCH_TOK(TOK_SYM, "(")))
        return std::make_unique<VariableExprAST>(idName);
    GET_TOK // t is "("
    PEEK_TOK // t is ")" or expression
    std::vector<std::unique_ptr<ExprAST> > args;
    if (MATCH_TOK(TOK_SYM, ")")) {
        GET_TOK // t is ")"
    }
    else {
        while (true) {
            auto arg = Expr(lex);
            if (!arg)
                return nullptr;
            args.push_back(std::move(arg));
            PEEK_TOK // t is ")" or ","
            if (MATCH_TOK(TOK_SYM, ")")) {
                GET_TOK // t is ")"
                break;
            }
            if (!(MATCH_TOK(TOK_SYM, ",")))
                ERROR("Expected ')' or ',' in argument list")
            GET_TOK // t is ","
        }
    }
    return std::make_unique<CallExprAST>(idName, std::move(args));
}


// primary -> number expr | paren expr | identifier expr
std::unique_ptr<ExprAST> Primary(Lexer *lex) {
    Token t;
    PEEK_TOK
    if (t.tp == TOK_NUM)
        return NumberExpr(lex);
    if (MATCH_TOK(TOK_SYM, "("))
        return ParenExpr(lex);
    if (t.tp == TOK_ID)
        return IdentifierExpr(lex);
    if (t.tp == TOK_EOF)
        return nullptr;
    else
        ERROR("Illegal token when expecting an expression")
}


// expression -> primary binoprhs
std::unique_ptr<ExprAST> Expr(Lexer *lex) {
    auto lhs = Primary(lex);
    if (!lhs)
        return nullptr;
    return BinOpRHS(lex, 0, std::move(lhs));
}


// binoprhs -> ("+" primary)*
std::unique_ptr<ExprAST> BinOpRHS(Lexer *lex, int exprPrec,
                                  std::unique_ptr<ExprAST> lhs) {
    while (true) {
        Token t;
        PEEK_TOK // t is operator
        int tokPrec = getBinopPrecedence(t.lx[0]);
        if (tokPrec < exprPrec)
            return lhs;
        GET_TOK // t is operator
        char binop = t.lx[0];
        auto rhs = Primary(lex);
        if (!rhs)
            return nullptr;
        PEEK_TOK // t is operator
        int nextPrec = getBinopPrecedence(t.lx[0]);
        if (tokPrec < nextPrec) {
            rhs = BinOpRHS(lex, tokPrec + 1, std::move(rhs));
            if (!rhs)
                return nullptr;
        }
        lhs = std::make_unique<BinaryExprAST>(binop, std::move(lhs), std::move(rhs));
    }
}


// prototype -> ID "(" ID* ")"
std::unique_ptr<PrototypeAST> Prototype(Lexer *lex) {
    Token t;
    GET_TOK // t is ID
    if (t.tp != TOK_ID)
        ERROR("Expected function name in prototype")
    std::string fnName = t.lx;
    GET_TOK // t is "("
    if (!(MATCH_TOK(TOK_SYM, "(")))
        ERROR("Expected '(' in prototype")
    std::vector<std::string> argNames;
    GET_TOK // t is ")" or ID
    if (!(MATCH_TOK(TOK_SYM, ")"))) {
        while (true) {
            if (t.tp != TOK_ID)
                ERROR("Expected identifier in argument list")
            argNames.push_back(t.lx);
            GET_TOK
            if (MATCH_TOK(TOK_SYM, ")"))
                break;
            if (!(MATCH_TOK(TOK_SYM, ",")))
                ERROR("Expected ',' between arguments")
            GET_TOK
        }
    }
    if (!(MATCH_TOK(TOK_SYM, ")")))
        ERROR("Expected ')' in prototype")
    return std::make_unique<PrototypeAST>(fnName, std::move(argNames));
}


// definition -> "fn" prototype "{" expression "}"
std::unique_ptr<FunctionAST> Definition(Lexer *lex) {
    Token t;
    GET_TOK // t is "fn"
    if (!(MATCH_TOK(TOK_FN, "fn")))
        ERROR("Expected 'fn' in definition")
    auto proto = Prototype(lex);
    if (!proto)
        return nullptr;
    GET_TOK // t is "{"
    if (!(MATCH_TOK(TOK_SYM, "{")))
        ERROR("Expected '{' in definition")
    auto e = Expr(lex);
    if (!e)
        ERROR("Expected expression in definition")
    GET_TOK // t is "}"
    if (!(MATCH_TOK(TOK_SYM, "}")))
        ERROR("Expected '}' in definition")
    return std::make_unique<FunctionAST>(std::move(proto), std::move(e));
}


// external -> "extern" prototype
std::unique_ptr<PrototypeAST> Extern(Lexer *lex) {
    Token t;
    GET_TOK // t is "extern"
    if (!(MATCH_TOK(TOK_EXTERN, "extern")))
        ERROR("Expected 'extern' in extern")
    return Prototype(lex);
}


// toplevel expr -> expression
std::unique_ptr<FunctionAST> TopLevelExpr(Lexer *lex) {
    if (auto e = Expr(lex)) {
        auto proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(proto), std::move(e));
    }
    return nullptr;
}


#ifdef TEST_PARSER
int main() {
    Lexer *lex = new Lexer("simple.lisa");
    cout << "Lisa Compiler Ready >" << endl;
    mainLoop(lex);
    cout << "Parsing Complete." << endl;
    delete lex;
    return 0;
}
#endif
