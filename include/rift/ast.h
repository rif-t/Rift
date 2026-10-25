#pragma once

#include <memory>
#include "integer.h"
#include <string>
#include <variant>
#include <vector>

using RuntimeValue = std::variant<WideInteger, bool>;

struct Expression
{
    enum class Type
    {
        Number,
        Identifier,
        Unary,
        Binary
    };

    Type type;

    RuntimeValue value = WideInteger{};
    std::string name;

    char operation = '\0';

    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
};

struct VariableDeclaration
{
    std::string name;
    std::string type;

    std::unique_ptr<Expression> value;
};

struct Statement
{
    enum class Type
    {
        VariableDeclaration,
        Print
    };

    Type type;

    std::unique_ptr<VariableDeclaration> variable;
    std::unique_ptr<Expression> expression;
};

struct Function
{
    std::string name;
    std::vector<Statement> statements;
};

struct Program
{
    std::vector<Function> functions;
};
