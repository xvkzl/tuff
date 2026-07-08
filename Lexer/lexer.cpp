#include "lexer.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>


std::string IdentifierStr;
double NumVal;

int gettok() {
    static int LastChar = ' ';

    // Skip whitespace.
    while (isspace(static_cast<unsigned char>(LastChar)))
        LastChar = getchar();
    // Identifier: [a-zA-Z][a-zA-Z0-9]*
    if (isalpha(LastChar) || LastChar == '_') {
        IdentifierStr.clear();
        
        do {
            IdentifierStr += static_cast<char>(LastChar);
            LastChar = getchar();
        } while (isalnum(LastChar) || LastChar == '_');
    
        if (IdentifierStr == "funk")
            return tok_funk;
    
        if (IdentifierStr == "dih")
            return tok_dih;
    
        return tok_identifier;
    }

    if (LastChar == '~') {
    // Comment until end of line.
    do
        LastChar = getchar();
    while (LastChar != EOF &&
           LastChar != '\n' &&
           LastChar != '\r');

    if (LastChar != EOF)
        return gettok();
    }   


    if (isdigit(LastChar) || LastChar == '.') {   // Number: [0-9.]+
      std::string NumStr;
      do {
        NumStr += LastChar;
        LastChar = getchar();
      } while (isdigit(LastChar) || LastChar == '.');

      NumVal = strtod(NumStr.c_str(), 0);
      return tok_number;
    }

    // EOF
    if (LastChar == EOF)
        return tok_eof;

    // Return single-character tokens.
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}