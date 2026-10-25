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

const Token& Parser::peek() const
{
    if (position_ + 1 >= tokens_.size()) {
        throw std::runtime_error("Unexpected end of file");
    }
    return tokens_[position_ + 1];
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

    if (current().text == "print" || current().text == "println") {
        return parsePrint();
    }

    if (peek().type == TokenType::Equal || peek().type == TokenType::PlusEqual ||
        peek().type == TokenType::MinusEqual || peek().type == TokenType::MultiplyEqual ||
        peek().type == TokenType::DivideEqual || peek().type == TokenType::RemainderEqual ||
        peek().type == TokenType::PowerEqual || peek().type == TokenType::Increment ||
        peek().type == TokenType::Decrement) {
        Statement statement;
        statement.type = Statement::Type::Assignment;
        statement.name = consume().text;
        const TokenType op = consume().type;
        switch (op) {
        case TokenType::Equal: statement.operation = '='; break;
        case TokenType::PlusEqual: statement.operation = '+'; break;
        case TokenType::MinusEqual: statement.operation = '-'; break;
        case TokenType::MultiplyEqual: statement.operation = '*'; break;
        case TokenType::DivideEqual: statement.operation = '/'; break;
        case TokenType::RemainderEqual: statement.operation = '%'; break;
        case TokenType::PowerEqual: statement.operation = '^'; break;
        case TokenType::Increment: statement.operation = '+'; break;
        case TokenType::Decrement: statement.operation = '-'; break;
        default: throw std::runtime_error("Invalid assignment operator");
        }
        if (op != TokenType::Increment && op != TokenType::Decrement) statement.expression = parseExpression();
        return statement;
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
    Statement statement;
    statement.type = Statement::Type::Print;
    statement.newline = consume().text == "println";

    expect(TokenType::LeftParen);
    if (current().type != TokenType::RightParen) {
        do {
            statement.arguments.push_back(parseExpression());
            if (current().type != TokenType::Comma) break;
            consume();
        } while (true);
    }

    expect(TokenType::RightParen);
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

    if (type.type != TokenType::Int && type.type != TokenType::IntegerType && type.type != TokenType::Bool &&
        type.type != TokenType::StringType && type.type != TokenType::CharType) {
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

    while (current().type == TokenType::Multiply || current().type == TokenType::Divide || current().type == TokenType::Remainder) {
        const TokenType operation = consume().type;
        auto right = parsePower();

        auto expression = std::make_unique<Expression>();
        expression->type = Expression::Type::Binary;
        expression->operation = operation == TokenType::Multiply ? '*' : operation == TokenType::Divide ? '/' : '%';
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

    if (token.type == TokenType::StringLiteral) {
        auto expression = std::make_unique<Expression>();
        expression->type = Expression::Type::String;
        expression->value = token.text;
        consume();
        return expression;
    }

    if (token.type == TokenType::CharacterLiteral) {
        auto expression = std::make_unique<Expression>();
        expression->type = Expression::Type::Character;
        expression->value = token.text[0];
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
