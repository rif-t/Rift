#include "parser.h"

#include <stdexcept>

Parser::Parser(const std::vector<Token>& tokens)
    : tokens_(tokens)
{
}

const Token& Parser::current() const
{
    if (position_ >= tokens_.size()) {
        throw std::runtime_error("Unexpected end of file");
    }

    return tokens_[position_];
}

const Token& Parser::consume()
{
    const Token& token = current();
    position_++;

    return token;
}

void Parser::expect(TokenType type)
{
    if (current().type != type) {
        throw std::runtime_error(
            "Unexpected token: " + current().text
        );
    }

    consume();
}

Program Parser::parse()
{
    Program program;

    while (position_ < tokens_.size()) {
        if (current().type == TokenType::Func) {
            program.functions.push_back(parseFunction());
        }
        else {
            throw std::runtime_error(
                "Unexpected token: " + current().text
            );
        }
    }

    return program;
}

Function Parser::parseFunction()
{
    expect(TokenType::Func);

    const Token& name = current();

    if (name.type != TokenType::Identifier) {
        throw std::runtime_error("Expected function name");
    }

    consume();

    expect(TokenType::LeftParen);
    expect(TokenType::RightParen);
    expect(TokenType::LeftBrace);

    Function function;
    function.name = name.text;

    while (current().type != TokenType::RightBrace) {
        function.statements.push_back(parseStatement());
    }

    expect(TokenType::RightBrace);

    return function;
}

Statement Parser::parseStatement()
{
    if (current().type != TokenType::Identifier) {
        throw std::runtime_error(
            "Expected statement"
        );
    }

    if (current().text == "print") {
        return parsePrint();
    }

    Statement statement;
    statement.type = Statement::Type::VariableDeclaration;

    statement.variable = std::make_unique<VariableDeclaration>(
        parseVariable()
    );

    return statement;
}

Statement Parser::parsePrint()
{
    consume(); // print

    expect(TokenType::LeftParen);

    auto expression = parseExpression();

    expect(TokenType::RightParen);

    Statement statement;
    statement.type = Statement::Type::Print;
    statement.expression = std::move(expression);

    return statement;
}

VariableDeclaration Parser::parseVariable()
{
    const Token& name = current();

    if (name.type != TokenType::Identifier) {
        throw std::runtime_error("Expected variable name");
    }

    consume();

    expect(TokenType::Colon);

    const Token& type = current();

    if (type.type != TokenType::Int && type.type != TokenType::IntegerType && type.type != TokenType::Bool) {
        throw std::runtime_error("Expected type");
    }

    consume();

    expect(TokenType::Equal);

    auto value = parseExpression();

    VariableDeclaration variable;

    variable.name = name.text;
    variable.type = type.text;
    variable.value = std::move(value);

    return variable;
}

std::unique_ptr<Expression> Parser::parseExpression()
{
    return parseAdditive();
}

std::unique_ptr<Expression> Parser::parseAdditive()
{
    auto left = parseMultiplicative();

    while (current().type == TokenType::Plus || current().type == TokenType::Minus) {
        const TokenType operation = consume().type;
        auto right = parseMultiplicative();

        auto expression = std::make_unique<Expression>();
        expression->type = Expression::Type::Binary;
        expression->operation = operation == TokenType::Plus ? '+' : '-';
        expression->left = std::move(left);
        expression->right = std::move(right);
        left = std::move(expression);
    }

    return left;
}

std::unique_ptr<Expression> Parser::parseMultiplicative()
{
    auto left = parsePower();

    while (current().type == TokenType::Multiply || current().type == TokenType::Divide) {
        const TokenType operation = consume().type;
        auto right = parsePower();

        auto expression = std::make_unique<Expression>();
        expression->type = Expression::Type::Binary;
        expression->operation = operation == TokenType::Multiply ? '*' : '/';
        expression->left = std::move(left);
        expression->right = std::move(right);
        left = std::move(expression);
    }

    return left;
}

std::unique_ptr<Expression> Parser::parsePower()
{
    auto left = parsePrimary();

    if (current().type == TokenType::Power) {
        consume();
        auto right = parsePower();

        auto expression = std::make_unique<Expression>();
        expression->type = Expression::Type::Binary;
        expression->operation = '^';
        expression->left = std::move(left);
        expression->right = std::move(right);
        left = std::move(expression);
    }

    return left;
}

std::unique_ptr<Expression> Parser::parsePrimary()
{
    const Token& token = current();

    if (token.type == TokenType::Minus) {
        consume();
        auto expression = std::make_unique<Expression>();
        expression->type = Expression::Type::Unary;
        expression->operation = '-';
        expression->left = parsePrimary();
        return expression;
    }

    if (token.type == TokenType::Number) {
        auto expression = std::make_unique<Expression>();

        expression->type = Expression::Type::Number;
        expression->value = WideInteger{UInt128::parse(token.text), false, false};

        consume();

        return expression;
    }

    if (token.type == TokenType::True || token.type == TokenType::False) {
        auto expression = std::make_unique<Expression>();
        expression->type = Expression::Type::Number;
        expression->value = token.type == TokenType::True;
        consume();
        return expression;
    }

    if (token.type == TokenType::Identifier) {
        auto expression = std::make_unique<Expression>();

        expression->type = Expression::Type::Identifier;
        expression->name = token.text;

        consume();

        return expression;
    }

    throw std::runtime_error("Expected expression");
}
