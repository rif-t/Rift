#pragma once

#include <string>
#include <vector>

enum class TokenType
{
    Identifier,
    Number,
    StringLiteral,
    CharacterLiteral,

    Func,
    Using,
    Return,
    Int,
    Bool,
    StringType,
    CharType,
    VoidType,
    IntegerType,
    True,
    False,

    Colon,
    Namespace,
    Dot,
    Semicolon,
    Equal,
    ColonEqual,
    Plus,
    Minus,
	Multiply,
	Divide,
    Power,
    Remainder,
    Comma,
    PlusEqual,
    MinusEqual,
    MultiplyEqual,
    DivideEqual,
    RemainderEqual,
    PowerEqual,
    Increment,
    Decrement,
    EqualEqual,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    And,
    Or,
    Not,
    Question,
    Arrow,

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
