// Abstract Syntax Tree

#ifndef AST_H
#define AST_H

#include <string>
#include <vector>

/// ExprAST - Base class for all expression nodes.
class ExprAST {
  public:
    virtual ~ExprAST() {}
};

/// NumberExprAST - Expression class for numeric literals like "1.0"
class NumberExprAST : public ExprAST {
  private:
    double value;

  public:
    NumberExprAST(double value)
    : value(value)
    { }
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
  private:
    std::string name;

  public:
    VariableExprAST(const std::string& name)
    : name(name)
    { }
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
  private:
    char op;
    std::unique_ptr<ExprAST> LHS, RHS;

  public:
    BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
    : op(op)
    , LHS(std::move(LHS))
    , RHS(std::move(RHS))
    { }
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
  private:
    std::string callee;
    std::vector<std::unique_ptr<ExprAST> > args;

  public:
    CallExprAST(const std::string& callee, std::vector<std::unique_ptr<ExprAST> > args)
    : callee(callee)
    , args(std::move(args))
    { }
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
  private:
    std::string name;
    std::vector<std::string> args;

  public:
    PrototypeAST(const std::string& name, std::vector<std::string> args)
    : name(name)
    , args(std::move(args))
    { }

    const std::string& getName() const { return name; }
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
  private:
    std::unique_ptr<PrototypeAST> prototype;
    std::unique_ptr<ExprAST> body;

  public:
    FunctionAST(std::unique_ptr<PrototypeAST> prototype, std::unique_ptr<ExprAST> body)
    : prototype(std::move(prototype))
    , body(std::move(body))
    { }
};

#endif