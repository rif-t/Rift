#include "parser.h"

#include <stdexcept>

std::unique_ptr<Expression> Parser::parseExpression()
{
    return parseConditional();
}

std::unique_ptr<Expression> Parser::parseConditional()
{
    auto condition = parseLogicalOr();
    if (current().type == TokenType::Question) {
        consume();
        auto whenTrue = parseExpression();
        expect(TokenType::Colon);
        auto whenFalse = parseConditional();
        auto expression = std::make_unique<Expression>();
        expression->type = Expression::Type::Conditional;
        expression->left = std::move(condition);
        expression->right = std::move(whenTrue);
        expression->third = std::move(whenFalse);
        return expression;
    }
    return condition;
}

std::unique_ptr<Expression> Parser::parseLogicalOr()
{
    auto left = parseLogicalAnd();
    while (current().type == TokenType::Or) {
        consume(); auto right = parseLogicalAnd();
        auto expression = std::make_unique<Expression>(); expression->type = Expression::Type::Binary; expression->operation = "||";
        expression->left = std::move(left); expression->right = std::move(right); left = std::move(expression);
    }
    return left;
}

std::unique_ptr<Expression> Parser::parseLogicalAnd()
{
    auto left = parseEquality();
    while (current().type == TokenType::And) {
        consume(); auto right = parseEquality();
        auto expression = std::make_unique<Expression>(); expression->type = Expression::Type::Binary; expression->operation = "&&";
        expression->left = std::move(left); expression->right = std::move(right); left = std::move(expression);
    }
    return left;
}

std::unique_ptr<Expression> Parser::parseEquality()
{
    auto left = parseComparison();
    while (current().type == TokenType::EqualEqual || current().type == TokenType::NotEqual) {
        const auto op = consume().type; auto right = parseComparison();
        auto expression = std::make_unique<Expression>(); expression->type = Expression::Type::Binary; expression->operation = op == TokenType::EqualEqual ? "==" : "!=";
        expression->left = std::move(left); expression->right = std::move(right); left = std::move(expression);
    }
    return left;
}

std::unique_ptr<Expression> Parser::parseComparison()
{
    auto left = parseAdditive();
    while (current().type == TokenType::Less || current().type == TokenType::LessEqual || current().type == TokenType::Greater || current().type == TokenType::GreaterEqual) {
        const auto op = consume().type; auto right = parseAdditive();
        auto expression = std::make_unique<Expression>(); expression->type = Expression::Type::Binary;
        expression->operation = op == TokenType::Less ? "<" : op == TokenType::LessEqual ? "<=" : op == TokenType::Greater ? ">" : ">=";
        expression->left = std::move(left); expression->right = std::move(right); left = std::move(expression);
    }
    return left;
}

std::unique_ptr<Expression> Parser::parseAdditive()
{
    auto left = parseMultiplicative();

    while (current().type == TokenType::Plus || current().type == TokenType::Minus) {
        const TokenType operation = consume().type;
        auto right = parseMultiplicative();

        auto expression = std::make_unique<Expression>();
        expression->type = Expression::Type::Binary;
        expression->operation = operation == TokenType::Plus ? "+" : "-";
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
        expression->operation = operation == TokenType::Multiply ? "*" : operation == TokenType::Divide ? "/" : "%";
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
        expression->operation = "^";
        expression->left = std::move(left);
        expression->right = std::move(right);
        left = std::move(expression);
    }

    return left;
}

std::unique_ptr<Expression> Parser::parsePrimary()
{
    const Token& token = current();

    if (token.type == TokenType::Int || token.type == TokenType::IntegerType || token.type == TokenType::Bool ||
        token.type == TokenType::StringType || token.type == TokenType::CharType || token.type == TokenType::VoidType ||
        (token.type == TokenType::Identifier && (token.text == "float" || token.text == "double" || token.text == "decimal" || token.text == "any"))) {
        auto expression = std::make_unique<Expression>();
        expression->type = Expression::Type::TypeName;
        expression->name = token.text;
        consume();
        return expression;
    }

    if (token.type == TokenType::Minus || token.type == TokenType::Not) {
        consume();
        auto expression = std::make_unique<Expression>();
        expression->type = Expression::Type::Unary;
        expression->operation = token.type == TokenType::Minus ? "-" : "!";
        expression->left = parsePrimary();
        return expression;
    }

    if (token.type == TokenType::Number) {
        auto expression = std::make_unique<Expression>();
        expression->type = Expression::Type::Number;
        const char suffix = token.text.empty() ? '\0' : token.text.back();
        const bool typedFloat = suffix == 'f' || suffix == 'F';
        const bool typedDouble = suffix == 'd' || suffix == 'D';
        const bool typedDecimal = suffix == 'm' || suffix == 'M';
        const std::string number = typedFloat || typedDouble || typedDecimal ? token.text.substr(0, token.text.size() - 1) : token.text;
        try {
            if (typedFloat) expression->value = std::stof(number);
            else if (typedDecimal) expression->value = Decimal::parse(number);
            else if (typedDouble || number.find('.') != std::string::npos) expression->value = std::stod(number);
            else expression->value = WideInteger{UInt128::parse(number), false, false};
        } catch (...) { throw std::runtime_error("Invalid numeric literal: " + token.text); }

        consume();

        return expression;
    }

    if (token.type == TokenType::LeftParen) {
        consume();
        auto expression = parseExpression();
        expect(TokenType::RightParen);
        return expression;
    }

    if (token.type == TokenType::StringLiteral) {
        auto expression = std::make_unique<Expression>();
        expression->type = Expression::Type::String;
        expression->value = utf8ToUtf16(token.text);
        consume();
        return expression;
    }

    if (token.type == TokenType::CharacterLiteral) {
        auto expression = std::make_unique<Expression>();
        expression->type = Expression::Type::Character;
        auto character = utf8ToUtf16(token.text);
        if (character.size() != 1) throw std::runtime_error("UTF-16 char literal must contain exactly one code unit");
        expression->value = character[0];
        consume();
        return expression;
    }

    if (token.type == TokenType::True || token.type == TokenType::False) {
        auto expression = std::make_unique<Expression>();
        expression->type = Expression::Type::Boolean;
        expression->value = token.type == TokenType::True;
        consume();
        return expression;
    }

    if (token.type == TokenType::Identifier) {
        auto expression = std::make_unique<Expression>();
        expression->name = token.text;
        consume();
        while (current().type == TokenType::Dot) {
            consume();
            if (current().type != TokenType::Identifier && current().type != TokenType::Break)
                throw std::runtime_error("Expected name after '.'");
            expression->name += "." + consume().text;
        }
        if (current().type == TokenType::LeftParen) {
            expression->type = Expression::Type::Call;
            consume();
            if (current().type != TokenType::RightParen) {
                do {
                    expression->arguments.push_back(parseExpression());
                    if (current().type != TokenType::Comma) break;
                    consume();
                } while (true);
            }
            expect(TokenType::RightParen);
        } else {
            expression->type = Expression::Type::Identifier;
        }
        return expression;
    }

    throw std::runtime_error("Expected expression at line " + std::to_string(current().line) +
        ", column " + std::to_string(current().column));
}
