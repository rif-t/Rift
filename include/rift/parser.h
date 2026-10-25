#pragma once

#include "ast.h"
#include "lexer.h"

class Parser
{
public:
    explicit Parser(const std::vector<Token>& tokens);

    Program parse();

private:
    const Token& current() const;
    const Token& consume();

    void expect(TokenType type);

    Function parseFunction();
    Statement parseStatement();
    Statement parsePrint();
    VariableDeclaration parseVariable();

    std::unique_ptr<Expression> parseExpression();
    std::unique_ptr<Expression> parseAdditive();
    std::unique_ptr<Expression> parseMultiplicative();
    std::unique_ptr<Expression> parsePower();
    std::unique_ptr<Expression> parsePrimary();

    const std::vector<Token>& tokens_;
    size_t position_ = 0;
};
