#ifndef PARSER_H
#define PARSER_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ast.h"
#include "lexer.h"

// Forward declarations
std::unique_ptr<ExprAST> ParseExpression();
std::unique_ptr<ExprAST> ParseBinOpRHS(int, std::unique_ptr<ExprAST>);

// Every function in our parser will assume that CurTok
// is the current token that needs to be parsed.
static int CurTok;
int getNextToken()
{
    return CurTok = gettok();
}

/// LogError* - These are little helper functions for error handling.
std::unique_ptr<ExprAST> LogError(const char *str)
{
    fprintf(stderr, "Error: %s\n", str);
    return nullptr;
}
std::unique_ptr<PrototypeAST> LogErrorP(const char *str)
{
    LogError(str);
    return nullptr;
}

/// numberexpr ::= number
/// expects to be called when the current token is a tok_number token.
std::unique_ptr<ExprAST> ParseNumberExpr()
{
    auto result = std::make_unique<NumberExprAST>(NumVal);
    getNextToken(); // consume the number
    return std::move(result);
}

/// parenexpr ::= '(' expression ')'
/// expects that the current token is a ‘(‘ token.
std::unique_ptr<ExprAST> ParseParenExpr()
{
    getNextToken(); // consume (.
    auto expression = ParseExpression();
    if (!expression)
        return nullptr;

    if (CurTok != ')')
        return LogError("expected ')'");
    getNextToken(); // consume ).
    return expression;
}

/// identifierexpr
///     ::= identifier                      # variable references
///     ::= identifier '(' expression* ')'  # function calls
/// expects to be called if the current token is a tok_identifier token.
std::unique_ptr<ExprAST> ParseIdentifierExpr()
{
    std::string idName = IdentifierStr;

    getNextToken(); // consume the identifier.

    if (CurTok != '(') // Simple variable ref.
        return std::make_unique<VariableExprAST>(idName);

    // Make the function call
    getNextToken(); // consume (.
    std::vector<std::unique_ptr<ExprAST>> args;
    if (CurTok != ')')
    {
        while (true)
        {
            if (auto arg = ParseExpression())
                args.push_back(std::move(arg));
            else
                return nullptr;

            if (CurTok == ')')
                break;

            if (CurTok != ',')
                return LogError("Expected ')' or ',' in argument list");

            getNextToken();
        }
    }

    // Consume the ')'.
    getNextToken();

    return std::make_unique<CallExprAST>(idName, std::move(args));
}

/// primary
///     ::= identifierexpr
///     ::= numberexpr
///     ::= parenexpr
/// uses look-ahead to determine which sort of expression is being inspected, and then parses it.
std::unique_ptr<ExprAST> ParsePrimary()
{
    switch (CurTok)
    {
    default:
        return LogError("Unknown token when expecting an expression.");
    case tok_identifier:
        return ParseIdentifierExpr();
    case tok_number:
        return ParseNumberExpr();
    case '(':
        return ParseParenExpr();
    }
}

// Binary expression parsing
// ======================================================================================

/// BinopPrecedence - This holds the precedence for each binary operator that is defined.
static std::map<char, int> BinopPrecedence;

bool BinaryOperatorsInstalled()
{
    return !BinopPrecedence.empty();
}

// 1 is lowest precedence.
void InstallBinaryOperators()
{
    if (BinaryOperatorsInstalled())
    {
        return;
    }
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40; // highest
}

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
int GetTokPrecendence()
{
    if (!isascii(CurTok))
        return -1;

    // Make sure it's a declared binop.
    if (!BinaryOperatorsInstalled())
    {
        LogError("Binary operators are not installed yet.");
        return -2;
    }
    int tokPrec = BinopPrecedence[CurTok];
    if (tokPrec <= 0)
        return -1;
    return tokPrec;
}

// ======================================================================================

/// expression
///     ::= primary binoprhs
std::unique_ptr<ExprAST> ParseExpression()
{
    auto LHS = ParsePrimary();
    if (!LHS)
        return nullptr;

    return ParseBinOpRHS(0, std::move(LHS));
}

/// binoprhs
///     ::= ('+' primary)*
std::unique_ptr<ExprAST> ParseBinOpRHS(int exprPrec, std::unique_ptr<ExprAST> LHS)
{
    // If this is a binop, find its precedence.
    while (true)
    {
        int tokPrec = GetTokPrecendence();

        // If this is a binop that binds at least as tightly as the current binop,
        // go ahead and consume it, otherwise we are done.
        if (tokPrec < exprPrec)
        {
            return LHS; // done
        }

        // Okay, we know this is a binop.
        int binOp = CurTok;
        getNextToken(); // consume binop

        // Parse the primary expression after the binary operator.
        std::unique_ptr<ExprAST> RHS = ParsePrimary();
        if (!RHS)
            return nullptr;

        // If binOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int nextPrec = GetTokPrecendence();
        if (tokPrec < nextPrec)
        {
            RHS = ParseBinOpRHS(tokPrec + 1, std::move(RHS));
            if (!RHS)
                return nullptr;
        }

        // Merge LHS/RHS.
        LHS = std::make_unique<BinaryExprAST>(binOp, std::move(LHS), std::move(RHS));
    } // loop around to the top of the while loop
}

/// prototype
///     ::= id '(' id* ')'
std::unique_ptr<PrototypeAST> ParsePrototype()
{
    if (CurTok != tok_identifier)
        return LogErrorP("Expected function name in prototype");

    std::string fnName = IdentifierStr;
    getNextToken();

    if (CurTok != '(')
        return LogErrorP("Expected '(' in prototype");

    // Read the list of argument names.
    std::vector<std::string> argNames;
    while (getNextToken() == tok_identifier)
        argNames.push_back(IdentifierStr);
    if (CurTok != ')')
        return LogErrorP("Expected ')' in prototype");

    // success.
    getNextToken(); // consume ')'.

    return std::make_unique<PrototypeAST>(fnName, std::move(argNames));
}

/// definition ::= 'def' prototype expression
std::unique_ptr<FunctionAST> ParseDefinition()
{
    getNextToken(); // consume 'def'.
    std::unique_ptr<PrototypeAST> prototype = ParsePrototype();
    if (!prototype)
        return nullptr;

    if (std::unique_ptr<ExprAST> expression = ParseExpression())
        return std::make_unique<FunctionAST>(std::move(prototype), std::move(expression));
    return nullptr;
}

/// external := 'extern' prototype
std::unique_ptr<PrototypeAST> ParseExtern()
{
    getNextToken(); // consume 'extern'.
    return ParsePrototype();
}

/// toplevelexpr ::= expression
std::unique_ptr<FunctionAST> ParseTopLevelExpr()
{
    if (std::unique_ptr<ExprAST> expression = ParseExpression())
    {
        // Make an anonymous prototype.
        auto prototype = std::make_unique<PrototypeAST>("", std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(prototype), std::move(expression));
    }
    return nullptr;
}

// Top-Level parsing

void HandleDefinition() {
    if (ParseDefinition()) {
        fprintf(stderr, "Parsed a function definition.\n");
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

void HandleExtern() {
    if (ParseExtern()) {
        fprintf(stderr, "Parsed an extern\n");
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

void HandleTopLevelExpression() {
    // Evaluate a top-level expression into an anonymous function.
    if (ParseTopLevelExpr()) {
        fprintf(stderr, "Parsed a top-level expr\n");
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

void MainLoop() {
    while (true) {
        fprintf(stderr, "ready> ");
        switch (CurTok)
        {
        case tok_eof:
            return;
        case ';': // ignore top-level semicolons.
            getNextToken();
            break;
        case tok_def:
            HandleDefinition();
            break;
        case tok_extern:
            HandleExtern();
            break;
        default:
            HandleTopLevelExpression();
            break;
        }
    }
}

#endif