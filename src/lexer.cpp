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
    if (current() == '\n') { ++line_; column_ = 1; }
    else ++column_;
    position_++;
}

std::vector<Token> Lexer::tokenize()
{
    std::vector<Token> tokens;

    while (position_ < source_.size()) {
        const std::size_t tokenLine = line_;
        const std::size_t tokenColumn = column_;
        auto emit = [&](TokenType type, const std::string& text) {
            tokens.push_back({type, text, tokenLine, tokenColumn});
        };
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
                emit(TokenType::Func, text);
            }
            else if (text == "using") {
                emit(TokenType::Using, text);
            }
            else if (text == "return") {
                emit(TokenType::Return, text);
            }
            else if (text == "for") emit(TokenType::For, text);
            else if (text == "break") emit(TokenType::Break, text);
            else if (text == "namespace") {
                emit(TokenType::NamespaceKeyword, text);
            }
            else if (text == "const") emit(TokenType::Const, text);
            else if (text == "int") {
                emit(TokenType::Int, text);
            }
            else if (text == "bool") {
                emit(TokenType::Bool, text);
            }
            else if (text == "string") {
                emit(TokenType::StringType, text);
            }
            else if (text == "char") {
                emit(TokenType::CharType, text);
            }
            else if (text == "void") {
                emit(TokenType::VoidType, text);
            }
            else if (text == "float" || text == "double" || text == "decimal") emit(TokenType::Identifier, text);
            else if (text == "true") {
                emit(TokenType::True, text);
            }
            else if (text == "false") {
                emit(TokenType::False, text);
            }
            else if (text == "and") {
                emit(TokenType::And, text);
            }
            else if (text == "or") {
                emit(TokenType::Or, text);
            }
            else if (text == "not") {
                emit(TokenType::Not, text);
            }
            else if (text == "int8" || text == "int16" || text == "int32" || text == "int64" || text == "int128" ||
                     text == "uint8" || text == "uint16" || text == "uint32" || text == "uint64" || text == "uint128") {
                emit(TokenType::IntegerType, text);
            }
            else {
                emit(TokenType::Identifier, text);
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
            if (current() == '.' && position_ + 1 < source_.size() && std::isdigit(static_cast<unsigned char>(source_[position_ + 1]))) {
                number += current(); advance();
                while (std::isdigit(static_cast<unsigned char>(current()))) { number += current(); advance(); }
            }
            if (current() == 'f' || current() == 'F' || current() == 'd' || current() == 'D' || current() == 'm' || current() == 'M') {
                number += current(); advance();
            }

            emit(TokenType::Number, number);

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
            emit(TokenType::StringLiteral, text);
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
                value.assign(1, ch);
            } else {
                value += ch;
                if (static_cast<unsigned char>(ch) >= 0x80) {
                    const unsigned char lead = static_cast<unsigned char>(ch);
                    const unsigned continuation = (lead & 0xe0) == 0xc0 ? 1 : (lead & 0xf0) == 0xe0 ? 2 : (lead & 0xf8) == 0xf0 ? 3 : 0;
                    if (continuation == 0) throw std::runtime_error("Invalid UTF-8 character literal");
                    for (unsigned i = 0; i < continuation; ++i) {
                        if (position_ >= source_.size()) throw std::runtime_error("Truncated UTF-8 character literal");
                        value += current(); advance();
                    }
                }
            }
            if (current() != '\'') throw std::runtime_error("Character literal must contain exactly one character");
            advance();
            emit(TokenType::CharacterLiteral, value);
            continue;
        }

        if (c == ',') {
            emit(TokenType::Comma, ",");
            advance();
            continue;
        }

        if (c == '.') {
            emit(TokenType::Dot, ".");
            advance();
            continue;
        }

        if (c == ';') {
            emit(TokenType::Semicolon, ";");
            advance();
            continue;
        }

        // (
        if (c == '(') {
            emit(TokenType::LeftParen, "(");
            advance();
            continue;
        }

        // )
        if (c == ')') {
            emit(TokenType::RightParen, ")");
            advance();
            continue;
        }

        // {
        if (c == '{') {
            emit(TokenType::LeftBrace, "{");
            advance();
            continue;
        }

        // }
        if (c == '}') {
            emit(TokenType::RightBrace, "}");
            advance();
            continue;
        }

        // :
        if (c == ':') {
            advance();

            // :=
            if (current() == '=') {
                emit(TokenType::ColonEqual, ":=");
                advance();
            }
            else {
                emit(TokenType::Colon, ":");
            }

            continue;
        }

        // =
        if (c == '=') {
            advance();
            if (current() == '=') { emit(TokenType::EqualEqual, "=="); advance(); }
            else emit(TokenType::Equal, "=");
            continue;
        }

        if (c == '!') {
            advance();
            if (current() == '=') { emit(TokenType::NotEqual, "!="); advance(); }
            else emit(TokenType::Not, "!");
            continue;
        }
        if (c == '<' || c == '>') {
            const char comparison = c;
            advance();
            const bool equal = current() == '=';
            if (equal) advance();
            emit(comparison == '<' ? (equal ? TokenType::LessEqual : TokenType::Less) : (equal ? TokenType::GreaterEqual : TokenType::Greater), equal ? std::string(1, comparison) + "=" : std::string(1, comparison));
            continue;
        }
        if (c == '&' || c == '|') {
            const char logical = c;
            advance();
            if (current() != logical) throw std::runtime_error("Use && for logical AND and || for logical OR");
            advance();
            emit(logical == '&' ? TokenType::And : TokenType::Or, logical == '&' ? "&&" : "||");
            continue;
        }
        if (c == '?') {
            emit(TokenType::Question, "?");
            advance();
            continue;
        }

        // +
        if (c == '+') {
            advance();
            if (current() == '+') { emit(TokenType::Increment, "++"); advance(); }
            else if (current() == '=') { emit(TokenType::PlusEqual, "+="); advance(); }
            else emit(TokenType::Plus, "+");
            continue;
        }

        // -
        if (c == '-') {
            advance();
            if (current() == '>') { emit(TokenType::Arrow, "->"); advance(); }
            else if (current() == '-') { emit(TokenType::Decrement, "--"); advance(); }
            else if (current() == '=') { emit(TokenType::MinusEqual, "-="); advance(); }
            else emit(TokenType::Minus, "-");
            continue;
        }

        // * and **
        if (c == '*') {
            advance();

            if (current() == '*') {
                emit(TokenType::Power, "**");
                advance();
            }
            else if (current() == '=') { emit(TokenType::MultiplyEqual, "*="); advance(); }
            else {
                emit(TokenType::Multiply, "*");
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
            else if (current() == '=') { emit(TokenType::DivideEqual, "/="); advance(); }
            else emit(TokenType::Divide, "/");
            continue;
        }

        if (c == '%') {
            advance();
            if (current() == '=') { emit(TokenType::RemainderEqual, "%="); advance(); }
            else emit(TokenType::Remainder, "%");
            continue;
        }

        if (c == '^') {
            advance();
            if (current() == '=') { emit(TokenType::PowerEqual, "^="); advance(); }
            else emit(TokenType::Power, "^");
            continue;
        }

        throw std::runtime_error("Unexpected character '" + std::string(1, c) + "' at line " +
            std::to_string(line_) + ", column " + std::to_string(column_));
    }

    tokens.push_back({TokenType::EndOfFile, "<end of file>", line_, column_});
    return tokens;
}
