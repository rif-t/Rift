#include "lexer.h"

#include <cctype>

Lexer::Lexer(const std::string& source)
    : source_(source)
{
}

char Lexer::current() const
{
    if (position_ >= source_.size()) {
        return '\0';
    }

    return source_[position_];
}

void Lexer::advance()
{
    position_++;
}

std::vector<Token> Lexer::tokenize()
{
    std::vector<Token> tokens;

    while (position_ < source_.size()) {
        char c = current();

        // Whitespace
        if (std::isspace(static_cast<unsigned char>(c))) {
            advance();
            continue;
        }

        // Identifiers and keywords
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::string text;

            while (
                std::isalnum(static_cast<unsigned char>(current())) ||
                current() == '_'
                ) {
                text += current();
                advance();
            }

            if (text == "func") {
                tokens.push_back({ TokenType::Func, text });
            }
            else if (text == "int") {
                tokens.push_back({ TokenType::Int, text });
            }
            else {
                tokens.push_back({ TokenType::Identifier, text });
            }

            continue;
        }

        // Numbers
        if (std::isdigit(static_cast<unsigned char>(c))) {
            std::string number;

            while (std::isdigit(static_cast<unsigned char>(current()))) {
                number += current();
                advance();
            }

            tokens.push_back({ TokenType::Number, number });

            continue;
        }

        // (
        if (c == '(') {
            tokens.push_back({ TokenType::LeftParen, "(" });
            advance();
            continue;
        }

        // )
        if (c == ')') {
            tokens.push_back({ TokenType::RightParen, ")" });
            advance();
            continue;
        }

        // {
        if (c == '{') {
            tokens.push_back({ TokenType::LeftBrace, "{" });
            advance();
            continue;
        }

        // }
        if (c == '}') {
            tokens.push_back({ TokenType::RightBrace, "}" });
            advance();
            continue;
        }

        // :
        if (c == ':') {
            advance();

            // :=
            if (current() == '=') {
                tokens.push_back({ TokenType::ColonEqual, ":=" });
                advance();
            }
            else {
                tokens.push_back({ TokenType::Colon, ":" });
            }

            continue;
        }

        // =
        if (c == '=') {
            tokens.push_back({ TokenType::Equal, "=" });
            advance();
            continue;
        }

        // +
        if (c == '+') {
            tokens.push_back({ TokenType::Plus, "+" });
            advance();
            continue;
        }

        // -
        if (c == '-') {
            tokens.push_back({ TokenType::Minus, "-" });
            advance();
            continue;
        }

        // * and **
        if (c == '*') {
            advance();

            if (current() == '*') {
                tokens.push_back({ TokenType::Power, "**" });
                advance();
            }
            else {
                tokens.push_back({ TokenType::Multiply, "*" });
            }

            continue;
        }

        // /
        if (c == '/') {
            tokens.push_back({ TokenType::Divide, "/" });
            advance();
            continue;
        }

        // Unknown character
        advance();
    }

    return tokens;
}
