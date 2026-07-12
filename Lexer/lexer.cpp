#include "lexer.h"

#include<iostream>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>

std::string IdentifierStr;
double NumVal;

int gettok() {
    static std::string Line;
    static size_t Pos = 0;

    while (true) {
        // Need a new line?
        if (Pos >= Line.size()) {
            if (!std::getline(std::cin, Line))
                return tok_eof;

            Line.push_back('\n');
            Pos = 0;
        }

        char LastChar = Line[Pos++];

        // Skip whitespace (except newline)
        while (LastChar != '\n' &&
               std::isspace(static_cast<unsigned char>(LastChar))) {
            if (Pos >= Line.size())
                break;
            LastChar = Line[Pos++];
        }

        // Ignore empty lines
        if (LastChar == '\n')
            continue;

        // Comments
        if (LastChar == '~') {
            while (Pos < Line.size() && Line[Pos] != '\n')
                ++Pos;
            continue;
        }

        // Identifier or keyword
        if (std::isalpha(static_cast<unsigned char>(LastChar)) ||
            LastChar == '_') {

            IdentifierStr.clear();
            IdentifierStr += LastChar;

            while (Pos < Line.size()) {
                char C = Line[Pos];
                if (!std::isalnum(static_cast<unsigned char>(C)) &&
                    C != '_')
                    break;

                IdentifierStr += C;
                ++Pos;
            }

            if (IdentifierStr == "funk")
                return tok_funk;

            if (IdentifierStr == "dih")
                return tok_dih;

            return tok_identifier;
        }

        // Number
        if (std::isdigit(static_cast<unsigned char>(LastChar)) ||
            LastChar == '.') {

            std::string NumStr;
            NumStr += LastChar;

            while (Pos < Line.size()) {
                char C = Line[Pos];

                if (!std::isdigit(static_cast<unsigned char>(C)) &&
                    C != '.')
                    break;

                NumStr += C;
                ++Pos;
            }

            NumVal = std::strtod(NumStr.c_str(), nullptr);
            return tok_number;
        }

        // Single-character token
        return LastChar;
    }
}