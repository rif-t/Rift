#pragma once

#include <memory>
#include "integer.h"
#include <string>
#include <variant>
#include <vector>

using RuntimeValue = std::variant<WideInteger, bool, std::string, char>;

struct Expression
{
    enum class Type
    {
        Number,
        String,
        Character,
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
        Assignment,
        Print
    };

    Type type;

    std::unique_ptr<VariableDeclaration> variable;
    std::string name;
    char operation = '=';
    std::unique_ptr<Expression> expression;
    std::vector<std::unique_ptr<Expression>> arguments;
    bool newline = true;
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
