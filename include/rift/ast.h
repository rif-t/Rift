#pragma once

#include <memory>
#include "integer.h"
#include "decimal.h"
#include "utf.h"
#include <string>
#include <variant>
#include <utility>
#include <vector>

namespace rift {

using RuntimeValue = std::variant<WideInteger, float, double, Decimal, bool, std::string, std::u16string, char16_t, std::monostate>;

struct SourcePosition { std::size_t line = 1; std::size_t column = 1; };
struct SourceSpan { SourcePosition start; SourcePosition end; };

struct Function;

struct Expression
{
    enum class Type
    {
        Number,
        String,
        Character,
        Boolean,
        TypeName,
        Identifier,
        Unary,
        Binary,
        Conditional,
        Call
    };

    Type type;
    SourceSpan source;

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
    std::vector<std::string> names;
    std::string type;
    bool isConst = false;
    std::vector<std::unique_ptr<Expression>> values;
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
        For,
        While,
        Break,
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
    std::string loopName;
    std::vector<Statement> initializer;
    std::vector<Statement> post;
    std::vector<Statement> loopStatements;
    std::vector<Statement> loopElseStatements;
};

struct Function
{
    std::string name;
    std::vector<std::pair<std::string, std::string>> parameters;
    std::string returnType;
    std::vector<Statement> statements;
};

struct ImportDeclaration
{
    std::string path;
    std::string alias;
};

struct Program
{
    std::vector<Function> functions;
    std::vector<std::string> usingNamespaces;
    std::vector<std::string> topLevelNamespaces;
    std::vector<ImportDeclaration> imports;
};

}
