#ifndef LEXER_H
#define LEXER_H

#include <string>

// Each token returned by our lexer will either be one of the
// Token enum values or it will be an ‘unknown’ character like
// ‘+’, which is returned as its ASCII value. If the current
// token is an identifier, the IdentifierStr global variable
// holds the name of the identifier. If the current token is a
// numeric literal (like 1.0), NumVal holds its value.

// The lexer returns tokens [0-255] if it is an unknown character,
// otherwise one of these for known things.
enum Token
{
    tok_eof = -1,

    // commands
    tok_def = -2,
    tok_extern = -3,

    // primary
    tok_identifier = -4,
    tok_number = -5,
};

static std::string IdentifierStr; // Filled in if tok_identifier
static double NumVal;             // Filled in if tok_number

/// gettok - Return the next token from standard input.
int gettok()
{
    static int LastChar = ' ';

    // Skip any whitespace.
    while (isspace(LastChar))
        LastChar = getchar();

    // Identifier: [a-zA-Z][a-zA-Z0-9]*
    if (isalpha(LastChar))
    {
        IdentifierStr = LastChar;
        while (isalnum((LastChar = getchar())))
            IdentifierStr += LastChar;

        if (IdentifierStr == "def")
            return tok_def;
        if (IdentifierStr == "extern")
            return tok_extern;
        return tok_identifier;
    }

    // Number: [0-9.]+
    // Note that this isn't doing sufficient error checking: it will incorrectly read
    // "1.23.45.67" and handle it as if you typed in "1.23".
    if (isdigit(LastChar) || LastChar == '.')
    {
        std::string NumStr;
        do
        {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), 0);
        return tok_number;
    }

    // Comment
    if (LastChar == '#')
    {
        // Comment until end of line.
        do
            LastChar = getchar();
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF)
            return gettok();
    }

    // Check for end of file. Don't eat the EOF.
    if (LastChar == EOF)
        return tok_eof;

    // Otherwise, just return the character as its ascii value. (e.g. +)
    int thisChar = LastChar;
    LastChar = getchar();
    return thisChar;
}

#endif