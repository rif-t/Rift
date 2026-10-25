#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

using RuntimeValue = std::variant<std::int64_t, std::uint64_t, bool>;

struct Expression
{
    enum class Type
    {
        Number,
        Identifier,
        Binary
    };

    Type type;

    RuntimeValue value = std::int64_t{0};
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
