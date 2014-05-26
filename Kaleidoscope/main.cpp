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

enum class SpecialToken {
    eof = -1,
    def = -2,
    extrn = -3,
    identifier = -4,
    number = -5
};

using Token = boost::variant<SpecialToken, int>;

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



int main(int argc, const char * argv[])
{

    // insert code here...
    std::cout << "Hello, World!\n";
    return 0;
}

