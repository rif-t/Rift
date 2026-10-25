#pragma once

#include <memory>
#include "integer.h"
#include <string>
#include <variant>
#include <utility>
#include <vector>

using RuntimeValue = std::variant<WideInteger, bool, std::string, char, std::monostate>;

struct Expression
{
    enum class Type
    {
        Number,
        String,
        Character,
        Boolean,
        Identifier,
        Unary,
        Binary,
        Conditional,
        Call
    };

    Type type;

    RuntimeValue value = WideInteger{};
    std::string name;

    std::string operation;

    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    std::unique_ptr<Expression> third;
    std::vector<std::unique_ptr<Expression>> arguments;
};

struct VariableDeclaration
{
    std::string name;
    std::string type;

    std::unique_ptr<Expression> value;
};

struct Statement;

struct IfBranch
{
    std::unique_ptr<Expression> condition;
    std::vector<Statement> statements;
};

struct Statement
{
    enum class Type
    {
        VariableDeclaration,
        Assignment,
        Print,
        If,
        Return,
        ExpressionStatement
    };

    Type type;

    std::unique_ptr<VariableDeclaration> variable;
    std::string name;
    std::string operation = "=";
    std::unique_ptr<Expression> expression;
    std::vector<std::unique_ptr<Expression>> arguments;
    bool newline = true;
    std::vector<IfBranch> branches;
    std::vector<Statement> elseStatements;
};

struct Function
{
    std::string name;
    std::vector<std::pair<std::string, std::string>> parameters;
    std::string returnType;
    std::vector<Statement> statements;
};

struct Program
{
    std::vector<Function> functions;
    std::vector<std::string> usingNamespaces;
};
