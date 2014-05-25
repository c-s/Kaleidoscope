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

enum class Token {
    eof = -1,
    def = -2,
    extrn = -3,
    identifier = -4,
    number = -5
};

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
        if(IdentifierStr == "def") return Token::def;
        if(IdentifierStr == "extern") return Token::extrn;
        return Token::identifier;
    }
    
    if(isdigit(LastChar) || LastChar == '.') {
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while(isdigit(LastChar) || LastChar == '.');
        
        NumVal = strtod(NumStr.c_str(), 0);
        return Token::number;
    }
    
    if(LastChar == '#') {
        do LastChar = getchar();
        while (LastChar != EOF && LastChar!= '\n' && LastChar != '\r');
        
        if(LastChar != EOF)
            return gettok();
    }
    
    if(LastChar == EOF)
        return Token::eof;
    
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}



int main(int argc, const char * argv[])
{

    // insert code here...
    std::cout << "Hello, World!\n";
    return 0;
}

