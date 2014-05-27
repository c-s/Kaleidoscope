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

/// gettok - return the next token from standard output.
static Token gettok() {
    static char LastChar = ' ';
    
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
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');
        
        if(LastChar != EOF)
            return gettok();
    }
    
    if(LastChar == EOF)
        return SpecialToken::eof;
    
    char ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}

//===----------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------===//
namespace {
    /// ExprAST - Base class for all expression nodes.
    class ExprAST {
    public:
        virtual ~ExprAST() {}
    };
    
    /// NumberExprAST - Expression class for numeric literals like "1.0".
    class NumberExprAST : public ExprAST {
        double Val;
    public:
        NumberExprAST(double val) : Val {val} {}
    };
    
    /// VariableExprAST - Expression class for referencing a variable, like "a".
    class VariableExprAST : public ExprAST {
        std::string Name;
    public:
        VariableExprAST(const std::string &name) : Name {name} {}
    };
    
    /// BinaryExprAST - Expression class for a binary operator.
    class BinaryExprAST : public ExprAST {
        char Op;
        ExprAST *LHS, *RHS;
    public:
        BinaryExprAST(char op, ExprAST *lhs, ExprAST *rhs)
        : Op{op}, LHS {lhs}, RHS {rhs} {}
    };
    
    /// CallExprAST - Expression class for function calls.
    class CallExprAST : public ExprAST {
        std:: string Callee;
        std::vector<ExprAST*> Args;
    public:
        CallExprAST(const std::string &callee, std::vector<ExprAST*> &args)
        : Callee {callee}, Args {args} {}
    };
    
    /// PrototypeAST - This class represents the "prototype" for a function,
    /// which captures its name, and its argument names (thus implicitly the number
    /// of arguments the function takes).
    class PrototypeAST {
        std::string Name;
        std::vector<std::string> Args;
    public:
        PrototypeAST(const std::string &name, const std::vector<std::string> &args)
        : Name {name}, Args {args} {}
    };
    
    /// FunctionAST - This class represents a function definition itself.
    class FunctionAST {
        PrototypeAST *Proto;
        ExprAST *Body;
    public:
        FunctionAST(PrototypeAST *proto, ExprAST *body)
        : Proto {proto}, Body {body} {}
    };
} // end anonymous namespace

//===----------------------------------------------===//
// Parse
//===----------------------------------------------===//

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
    
    if(CurTok.type() != typeid(char) || (CurTok.type() == typeid(char) && boost::get<char>(CurTok) != ')'))
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
    
    if(CurTok.type() != typeid(char) || (CurTok.type() == typeid(char) && boost::get<char>(CurTok) !=  '('))
        return new VariableExprAST(IdName);
    
    // Call
    getNextToken();
    std::vector<ExprAST*> Args;
    if(CurTok.type() != typeid(char) || (CurTok.type() == typeid(char) && boost::get<char>(CurTok) != ')')) {
        while(true) {
            ExprAST *Arg = ParseExpression();
            if(!Arg) return nullptr;
            Args.push_back(Arg);
            
            if(CurTok.type() == typeid(char) && boost::get<char>(CurTok) == ')') break;
            if(CurTok.type() != typeid(char) || (CurTok.type() == typeid(char) && boost::get<char>(CurTok) != ','))
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
                    std::cerr << "1: token is " << static_cast<int>(token) << std::endl;
                    return Error("unknown token when expecting an expression");
            }
        }
        
        ExprAST* operator()(char token) const {
            switch(token) {
                case '(':
                    return ParseParenExpr();
                default:
                    std::cerr << "2: token is " << token << std::endl;

                    return Error("unknown token when expecting an expression");
            }
        }
    };
    
    return boost::apply_visitor(selector(), CurTok);
}

/// BinopPrecedence - This holds the precedence for each binary operator that is
/// defined,
static std::map<char, int> BinopPrecedence;

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
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
        
        // if BinOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int NextPrec = GetTokPrecedence();
        if(TokPrec < NextPrec) {
            RHS = ParseBinOpRHS(TokPrec+1, RHS);
            if(!RHS) return nullptr;
        }
        // Merge LHS/RHS
        LHS = new BinaryExprAST(boost::get<char>(BinOp), LHS, RHS);
    } // Loop around to the top of the while loop.
}

/// prototype
/// ::= id '(' id* ')'
static PrototypeAST *ParsePrototype() {
    if(CurTok.type() != typeid(SpecialToken) ||
       (CurTok.type() == typeid(SpecialToken) && boost::get<SpecialToken>(CurTok) != SpecialToken::identifier))
        return ErrorP("Expected function name in prototype");
    std::string FnName = IdentifierStr;
    getNextToken();
    
    if(CurTok.type() != typeid(char) ||
       (CurTok.type() == typeid(char) && boost::get<char>(CurTok) != '('))
        return ErrorP("Expected '(' in prototype");
    
    // read the list of argument names.
    std::vector<std::string> ArgNames;
    while(true) {
        getNextToken();
        if(!(CurTok.type() == typeid(SpecialToken) &&
           boost::get<SpecialToken>(CurTok) == SpecialToken::identifier)) break;
        ArgNames.push_back(IdentifierStr);
    }
    if(CurTok.type() != typeid(char) || (CurTok.type() == typeid(char) && boost::get<char>(CurTok) != ')'))
        return ErrorP("Expected ')' in prototype");
    
    // success
    getNextToken(); // eat ')'.
    
    return new PrototypeAST(FnName, ArgNames);
}

/// definition ::= 'def' prototype expression
static FunctionAST *ParseDefinition() {
    getNextToken(); // eat def.
    PrototypeAST *Proto = ParsePrototype();
    if(!Proto) return nullptr;
    
    if(ExprAST *E = ParseExpression())
        return new FunctionAST(Proto, E);
    return nullptr;
}

/// external ::= 'extern' prototype
static PrototypeAST *ParseExtern() {
    getNextToken(); // eat extern.
    return ParsePrototype();
}

/// topLevelexpr ::= expression
static FunctionAST *ParseTopLevelExpr() {
    if(ExprAST *E = ParseExpression()) {
        // make an anonymous proto.
        PrototypeAST *Proto = new PrototypeAST("", std::vector<std::string>());
        return new FunctionAST(Proto, E);
    }
    return nullptr;
}

static void HandleDefinition() {
    if(ParseDefinition()) {
        std::cerr << "Parsed a function definition." << std::endl;
    } else {
        // skip token for error recovery.
        getNextToken();
    }
}

static void HandleExtern() {
    if(ParseExtern()) {
        std::cerr << "Parsed an extern." << std::endl;
    } else {
        // skip token for error recovery.
        getNextToken();
    }
}

static void HandleTopLevelExpression() {
    // evaluate a top-level expression into an anonymous function.
    if(ParseTopLevelExpr()) {
        std::cerr << "Parsed a top-level expr." << std::endl;
    } else {
        // skip token for error recovery.
        getNextToken();
    }
}

/// top ::= definition | external | expression | ';'
static void MainLoop() {
    class selector : public boost::static_visitor<void> {
    public:
        void operator()(SpecialToken token) const {
            switch(token) {
                case SpecialToken::eof: return;
                case SpecialToken::def: HandleDefinition(); break;
                case SpecialToken::extrn: HandleExtern(); break;
                default: HandleTopLevelExpression(); break;
            }
        }
        void operator()(char token) const {
            switch(token) {
                case ';': getNextToken(); break;
                default: HandleTopLevelExpression(); break;
            }
        }
    };
    
    while(true) {
        std::cerr << "ready> ";
        boost::apply_visitor(selector(), CurTok);
    }
}

int main(int argc, const char * argv[])
{
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40;

    // prime the first token.
    std::cerr << "ready> ";
    getNextToken();
    
    // run the main "interpreter loop" now.
    MainLoop();
    
    return 0;
}

