#include "parser.h"

#include <stdexcept>

namespace rift {

std::unique_ptr<Expression> Parser::parseExpression()
{
    const auto start = position_;
    auto expression = parseConditional();
    setSpan(*expression, start, position_);
    return expression;
}

void Parser::setSpan(Expression& expression, std::size_t start, std::size_t end) const
{
    const auto& first = tokens_[start];
    const auto& last = tokens_[end - 1];
    expression.source = {{first.line, first.column}, {last.endLine, last.endColumn}};
    const auto fillMissing = [&](auto&& self, Expression& node) -> void {
        for (auto* child : {node.left.get(), node.right.get(), node.third.get()}) {
            if (!child) continue;
            if (child->source.start.line == 1 && child->source.start.column == 1 &&
                child->source.end.line == 1 && child->source.end.column == 1)
                child->source = expression.source;
            self(self, *child);
        }
        for (auto& argument : node.arguments) {
            if (argument->source.start.line == 1 && argument->source.start.column == 1 &&
                argument->source.end.line == 1 && argument->source.end.column == 1)
                argument->source = expression.source;
            self(self, *argument);
        }
        for (auto& index : node.indices) {
            if (index->source.start.line == 1 && index->source.start.column == 1 &&
                index->source.end.line == 1 && index->source.end.column == 1)
                index->source = expression.source;
            self(self, *index);
        }
    };
    fillMissing(fillMissing, expression);
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
    auto left = parseUnary();

    while (current().type == TokenType::Multiply || current().type == TokenType::Divide || current().type == TokenType::Remainder) {
        const TokenType operation = consume().type;
        auto right = parseUnary();

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
    auto left = parsePostfix();

    if (current().type == TokenType::Power) {
        consume();
        auto right = parseUnary();

        auto expression = std::make_unique<Expression>();
        expression->type = Expression::Type::Binary;
        expression->operation = "^";
        expression->left = std::move(left);
        expression->right = std::move(right);
        left = std::move(expression);
    }

    return left;
}

std::unique_ptr<Expression> Parser::parseUnary()
{
    if (current().type != TokenType::Minus && current().type != TokenType::Not)
        return parsePower();
    const auto start = position_;
    const TokenType operation = consume().type;
    auto expression = std::make_unique<Expression>();
    expression->type = Expression::Type::Unary;
    expression->operation = operation == TokenType::Minus ? "-" : "!";
    expression->left = parseUnary();
    setSpan(*expression, start, position_);
    return expression;
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
        while (current().type == TokenType::LeftBracket && peek().type == TokenType::RightBracket) {
            consume();
            consume();
            expression->name += "[]";
        }
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

    if (token.type == TokenType::LeftBracket) {
        auto expression = std::make_unique<Expression>();
        expression->type = Expression::Type::Array;
        consume();
        if (current().type != TokenType::RightBracket) {
            do {
                expression->arguments.push_back(parseExpression());
                if (current().type != TokenType::Comma) break;
                consume();
            } while (true);
        }
        expect(TokenType::RightBracket);
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
        expression->type = Expression::Type::Identifier;
        consume();
        return expression;
    }

    throw std::runtime_error("Expected expression at line " + std::to_string(current().line) +
        ", column " + std::to_string(current().column));
}

std::unique_ptr<Expression> Parser::parsePostfix()
{
    auto expression = parsePrimary();
    while (true) {
        if (current().type == TokenType::LeftBracket) {
            auto indexed = std::make_unique<Expression>();
            indexed->type = Expression::Type::Index;
            indexed->left = std::move(expression);
            do {
                consume();
                indexed->indices.push_back(parseExpression());
                expect(TokenType::RightBracket);
            } while (current().type == TokenType::LeftBracket);
        expression = std::move(indexed);
        } else if (current().type == TokenType::Dot) {
            consume();
            if (current().type != TokenType::Identifier && current().type != TokenType::Break)
                throw std::runtime_error("Expected name after '.'");
            const std::string memberName = current().text;
            if (expression->type == Expression::Type::Identifier &&
                (memberName == "iteration" || memberName == "limit" || memberName == "break" || memberName == "continue" || memberName == "restart")) {
                expression->name += "." + consume().text;
                continue;
            }
            auto member = std::make_unique<Expression>();
            member->type = Expression::Type::Member;
            member->name = consume().text;
            member->left = std::move(expression);
            expression = std::move(member);
        } else if (current().type == TokenType::LeftParen) {
            auto call = std::make_unique<Expression>();
            call->type = Expression::Type::Call;
            if (expression->type == Expression::Type::Member && expression->name == "append") {
                call->left = std::move(expression->left);
                call->name = "append";
            } else {
                auto* part = expression.get();
                while (part->type == Expression::Type::Member) {
                    call->name = "." + part->name + call->name;
                    part = part->left.get();
                }
                if (part->type != Expression::Type::Identifier)
                    throw std::runtime_error("Only named functions and array methods can be called");
                call->name = part->name + call->name;
            }
            consume();
            if (current().type != TokenType::RightParen) {
                do {
                    call->arguments.push_back(parseExpression());
                    if (current().type != TokenType::Comma) break;
                    consume();
                } while (true);
            }
            expect(TokenType::RightParen);
            expression = std::move(call);
        } else break;
    }
    return expression;
}

}
