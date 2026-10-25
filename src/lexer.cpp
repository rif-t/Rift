#include "lexer.h"

#include <cctype>
#include <stdexcept>

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
            else if (text == "using") {
                tokens.push_back({ TokenType::Using, text });
            }
            else if (text == "int") {
                tokens.push_back({ TokenType::Int, text });
            }
            else if (text == "bool") {
                tokens.push_back({ TokenType::Bool, text });
            }
            else if (text == "string") {
                tokens.push_back({ TokenType::StringType, text });
            }
            else if (text == "char") {
                tokens.push_back({ TokenType::CharType, text });
            }
            else if (text == "true") {
                tokens.push_back({ TokenType::True, text });
            }
            else if (text == "false") {
                tokens.push_back({ TokenType::False, text });
            }
            else if (text == "and") {
                tokens.push_back({ TokenType::And, text });
            }
            else if (text == "or") {
                tokens.push_back({ TokenType::Or, text });
            }
            else if (text == "not") {
                tokens.push_back({ TokenType::Not, text });
            }
            else if (text == "int8" || text == "int16" || text == "int32" || text == "int64" || text == "int128" ||
                     text == "uint8" || text == "uint16" || text == "uint32" || text == "uint64" || text == "uint128") {
                tokens.push_back({ TokenType::IntegerType, text });
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

        if (c == '"') {
            advance();
            std::string text;
            while (position_ < source_.size() && current() != '"') {
                char ch = current();
                advance();
                if (ch == '\\') {
                    if (position_ >= source_.size()) throw std::runtime_error("Unterminated escape sequence in string");
                    ch = current();
                    advance();
                    switch (ch) {
                    case 'n': text += '\n'; break;
                    case 'r': text += '\r'; break;
                    case 't': text += '\t'; break;
                    case '\\': text += '\\'; break;
                    case '"': text += '"'; break;
                    default: throw std::runtime_error("Unknown string escape sequence");
                    }
                } else {
                    text += ch;
                }
            }
            if (position_ >= source_.size()) throw std::runtime_error("Unterminated string literal");
            advance();
            tokens.push_back({TokenType::StringLiteral, text});
            continue;
        }

        if (c == '\'') {
            advance();
            std::string value;
            if (position_ >= source_.size() || current() == '\'') throw std::runtime_error("Character literal must contain exactly one character");
            char ch = current();
            advance();
            if (ch == '\\') {
                if (position_ >= source_.size()) throw std::runtime_error("Unterminated character escape");
                ch = current(); advance();
                switch (ch) {
                case 'n': ch = '\n'; break;
                case 'r': ch = '\r'; break;
                case 't': ch = '\t'; break;
                case '\\': ch = '\\'; break;
                case '\'': ch = '\''; break;
                case '0': ch = '\0'; break;
                default: throw std::runtime_error("Unknown character escape sequence");
                }
            }
            if (current() != '\'') throw std::runtime_error("Character literal must contain exactly one character");
            advance();
            tokens.push_back({TokenType::CharacterLiteral, std::string(1, ch)});
            continue;
        }

        if (c == ',') {
            tokens.push_back({TokenType::Comma, ","});
            advance();
            continue;
        }

        if (c == '.') {
            tokens.push_back({TokenType::Dot, "."});
            advance();
            continue;
        }

        if (c == ';') {
            tokens.push_back({TokenType::Semicolon, ";"});
            advance();
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
            else if (current() == ':') {
                tokens.push_back({TokenType::Namespace, "::"});
                advance();
            }
            else {
                tokens.push_back({ TokenType::Colon, ":" });
            }

            continue;
        }

        // =
        if (c == '=') {
            advance();
            if (current() == '=') { tokens.push_back({TokenType::EqualEqual, "=="}); advance(); }
            else tokens.push_back({ TokenType::Equal, "=" });
            continue;
        }

        if (c == '!') {
            advance();
            if (current() == '=') { tokens.push_back({TokenType::NotEqual, "!="}); advance(); }
            else tokens.push_back({TokenType::Not, "!"});
            continue;
        }
        if (c == '<' || c == '>') {
            const char comparison = c;
            advance();
            const bool equal = current() == '=';
            if (equal) advance();
            tokens.push_back({comparison == '<' ? (equal ? TokenType::LessEqual : TokenType::Less) : (equal ? TokenType::GreaterEqual : TokenType::Greater), equal ? std::string(1, comparison) + "=" : std::string(1, comparison)});
            continue;
        }
        if (c == '&' || c == '|') {
            const char logical = c;
            advance();
            if (current() != logical) throw std::runtime_error("Use && for logical AND and || for logical OR");
            advance();
            tokens.push_back({logical == '&' ? TokenType::And : TokenType::Or, logical == '&' ? "&&" : "||"});
            continue;
        }
        if (c == '?') {
            tokens.push_back({TokenType::Question, "?"});
            advance();
            continue;
        }

        // +
        if (c == '+') {
            advance();
            if (current() == '+') { tokens.push_back({ TokenType::Increment, "++" }); advance(); }
            else if (current() == '=') { tokens.push_back({ TokenType::PlusEqual, "+=" }); advance(); }
            else tokens.push_back({ TokenType::Plus, "+" });
            continue;
        }

        // -
        if (c == '-') {
            advance();
            if (current() == '-') { tokens.push_back({ TokenType::Decrement, "--" }); advance(); }
            else if (current() == '=') { tokens.push_back({ TokenType::MinusEqual, "-=" }); advance(); }
            else tokens.push_back({ TokenType::Minus, "-" });
            continue;
        }

        // * and **
        if (c == '*') {
            advance();

            if (current() == '*') {
                tokens.push_back({ TokenType::Power, "**" });
                advance();
            }
            else if (current() == '=') { tokens.push_back({ TokenType::MultiplyEqual, "*=" }); advance(); }
            else {
                tokens.push_back({ TokenType::Multiply, "*" });
            }

            continue;
        }

        // /
        if (c == '/') {
            advance();
            if (current() == '/') {
                while (position_ < source_.size() && current() != '\n') advance();
            }
            else if (current() == '*') {
                advance();
                std::size_t depth = 1;
                while (position_ < source_.size() && depth > 0) {
                    if (current() == '/' && position_ + 1 < source_.size() && source_[position_ + 1] == '*') {
                        advance(); advance(); ++depth;
                    } else if (current() == '*' && position_ + 1 < source_.size() && source_[position_ + 1] == '/') {
                        advance(); advance(); --depth;
                    } else {
                        advance();
                    }
                }
                if (depth != 0) throw std::runtime_error("Unterminated block comment");
            }
            else if (current() == '=') { tokens.push_back({ TokenType::DivideEqual, "/=" }); advance(); }
            else tokens.push_back({ TokenType::Divide, "/" });
            continue;
        }

        if (c == '%') {
            advance();
            if (current() == '=') { tokens.push_back({ TokenType::RemainderEqual, "%=" }); advance(); }
            else tokens.push_back({ TokenType::Remainder, "%" });
            continue;
        }

        if (c == '^') {
            advance();
            if (current() == '=') { tokens.push_back({ TokenType::PowerEqual, "^=" }); advance(); }
            else tokens.push_back({ TokenType::Power, "^" });
            continue;
        }

        // Unknown character
        advance();
    }

    return tokens;
}
