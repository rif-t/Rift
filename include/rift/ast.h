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
        Boolean,
        Identifier,
        Unary,
        Binary,
        Conditional
    };

    Type type;

    RuntimeValue value = WideInteger{};
    std::string name;

    std::string operation;

    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    std::unique_ptr<Expression> third;
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
        If
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
    std::vector<Statement> statements;
};

struct Program
{
    std::vector<Function> functions;
};
