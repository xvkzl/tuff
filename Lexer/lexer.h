#pragma once

#include <string>

enum Token {
    tok_eof = -1,

    tok_funk = -2,
    tok_dih = -3,

    tok_identifier = -4,
    tok_number = -5,
};

extern std::string IdentifierStr;
extern double NumVal;

int gettok();