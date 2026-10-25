#pragma once

#include <string>
#include <vector>

enum class TokenType
{
    Identifier,
    Number,

    Func,
    Int,
    Bool,
    IntegerType,
    True,
    False,

    Colon,
    Equal,
    ColonEqual,
    Plus,
    Minus,
	Multiply,
	Divide,
	Power,

    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace
};

struct Token
{
    TokenType type;
    std::string text;
};

class Lexer
{
public:
    explicit Lexer(const std::string& source);

    std::vector<Token> tokenize();

private:
    char current() const;
    void advance();

    std::string source_;
    size_t position_ = 0;
};
