//
//  main.cpp
//  Kaleidoscope
//
//  Created by Chang-Soon on 5/3/14.
//  Copyright (c) 2014 Chang-Soon. All rights reserved.
//

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
#include <boost/variant/variant.hpp>
#include <boost/variant/get.hpp>

enum class SpecialToken {
    eof = -1,
    def = -2,
    extrn = -3,
    identifier = -4,
    number = -5
};

using Token = boost::variant<SpecialToken, char>;

static std::string IdentifierStr;
static double NumVal;

static Token gettok() {
    static int LastChar = ' ';
    
    while(isspace(LastChar))
        LastChar = getchar();
    
    if(isalpha(LastChar)) {
        IdentifierStr = LastChar;
        while(isalnum((LastChar = getchar())))
            IdentifierStr += LastChar;
        if(IdentifierStr == "def") return SpecialToken::def;
        if(IdentifierStr == "extern") return SpecialToken::extrn;
        return SpecialToken::identifier;
    }
    
    if(isdigit(LastChar) || LastChar == '.') {
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while(isdigit(LastChar) || LastChar == '.');
        
        NumVal = strtod(NumStr.c_str(), 0);
        return SpecialToken::number;
    }
    
    if(LastChar == '#') {
        do LastChar = getchar();
        while (LastChar != EOF && LastChar!= '\n' && LastChar != '\r');
        
        if(LastChar != EOF)
            return gettok();
    }
    
    if(LastChar == EOF)
        return SpecialToken::eof;
    
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}

class ExprAST {
public:
    virtual ~ExprAST() {}
};

class NumberExprAST : public ExprAST {
    double Val;
public:
    NumberExprAST(double val) : Val {val} {}
};

class VariableExprAST : public ExprAST {
    std::string Name;
public:
    VariableExprAST(const std::string &name) : Name {name} {}
};

class BinaryExprAST : public ExprAST {
    char Op;
    ExprAST *LHS, *RHS;
public:
    BinaryExprAST(char op, ExprAST *lhs, ExprAST *rhs)
    : Op{op}, LHS {lhs}, RHS {rhs} {}
};

class CallExprAST : public ExprAST {
    std:: string Callee;
    std::vector<ExprAST*> Args;
public:
    CallExprAST(const std::string &callee, std::vector<ExprAST*> &args)
    : Callee {callee}, Args {args} {}
};

class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;
    
public:
    PrototypeAST(const std::string &name, const std::vector<std::string> &args)
    : Name {name}, Args {args} {}
};

class FunctionAST {
    PrototypeAST *Proto;
    ExprAST *Body;
public:
    FunctionAST(PrototypeAST *proto, ExprAST *body)
    : Proto {proto}, Body {body} {}
};

/// CurTok/getNextToken - Provide a simple token buffer. CurTok is the current
/// token the parser is looking at. getNextToken reads another token from the Lexer and updates CurTok with its results.
static Token CurTok;
static Token getNextToken() {
    return CurTok = gettok();
}

/// Error* - These are little helper functions for error handling.
ExprAST *Error(const char *Str) { fprintf(stderr, "Error: %s\n", Str); return nullptr;}
PrototypeAST *ErrorP(const char *Str) { Error(Str); return nullptr; }
FunctionAST *ErrorF(const char *Str) { Error(Str); return nullptr; }

/// numberexpr ::= number
static ExprAST *ParseNumberExpr() {
    ExprAST *Result = new NumberExprAST(NumVal);
    getNextToken();
    return Result;
}

static ExprAST* ParseExpression();

/// paranexpr ::= ')' expression ')'
static ExprAST *ParseParenExpr() {
    getNextToken();
    ExprAST *V = ParseExpression();
    if(!V) return nullptr;
    
    if(CurTok.type() == typeid(char) && boost::get<char>(CurTok) != ')')
        return Error("expected ')'");
    getNextToken(); // eat ).
    return V;
}

/// identifierexpr
///    ::= identifier
///    ::= identifier '(' expression* ')'
static ExprAST *ParseIdentifierExpr() {
    std::string IdName = IdentifierStr;
    
    getNextToken();
    
    if(CurTok.type() == typeid(char) && boost::get<char>(CurTok) !=  '(')
        return new VariableExprAST(IdName);
    
    // Call
    getNextToken();
    std::vector<ExprAST*> Args;
    if(CurTok.type() == typeid(char) && boost::get<char>(CurTok) != ')') {
        while(true) {
            ExprAST *Arg = ParseExpression();
            if(!Arg) return nullptr;
            Args.push_back(Arg);
            
            if(CurTok.type() == typeid(char) && boost::get<char>(CurTok) == ')') break;
            if(CurTok.type() == typeid(char) && boost::get<char>(CurTok) != ',')
                return Error("Expected ')' or ',' in argument list");
            getNextToken();
        }
    }
    
    getNextToken();
    return new CallExprAST(IdName, Args);
}

/// primary
/// ::= identifierexpr
/// ::= numberexpr
/// ::= parenexpr
static ExprAST *ParsePrimary() {
    class selector : public boost::static_visitor<ExprAST*> {
    public:
        ExprAST* operator()(SpecialToken token) const {
            switch(token) {
                case SpecialToken::identifier:
                    return ParseIdentifierExpr();
                case SpecialToken::number:
                    return ParseNumberExpr();
                default:
                    return Error("unknown token when expecting an expression");
            }
        }
        
        ExprAST* operator()(char token) const {
            switch(token) {
                case '(':
                    return ParseParenExpr();
                default:
                    return Error("unknown token when expecting an expression");
            }
        }
    };
    
    return boost::apply_visitor(selector(), CurTok);
}

/// BinopPrecedence - This holds the precedence for each binary operator that is
/// defined,
static std::map<char, int> BinopPrecedence;

static int GetTokPrecedence() {
    class selector : public boost::static_visitor<int> {
    public:
        char operator()(SpecialToken token) const {
            return -1;
        }
        char operator()(char token) const {
            if(!isascii(token)) return -1;
            int TokPrec = BinopPrecedence[token];
            if(TokPrec <= 0) return -1;
            return TokPrec;
        }
    };
    
    return boost::apply_visitor(selector(), CurTok);
}

static ExprAST* ParseBinOpRHS(int ExprPre, ExprAST *LHS);

/// expression
/// ::= primary binoprhs
///
static ExprAST* ParseExpression() {
    ExprAST *LHS = ParsePrimary();
    if(!LHS) return nullptr;
    
    return ParseBinOpRHS(0, LHS);
}

// binoprhs
/// ::= ('+' primary)*
static ExprAST* ParseBinOpRHS(int ExprPrec, ExprAST *LHS) {
    // if this is a binop, find its precedence.
    while(true) {
        int TokPrec = GetTokPrecedence();
        
        // if this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done.
        if(TokPrec < ExprPrec)
            return LHS;
        Token BinOp = CurTok;
        getNextToken();
        
        ExprAST* RHS = ParsePrimary();
        if(!RHS) return nullptr;
    }
}

int main(int argc, const char * argv[])
{
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40;
    // insert code here...
    std::cout << "Hello, World!\n";
    return 0;
}

