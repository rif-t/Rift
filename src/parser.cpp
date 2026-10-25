#include "parser.h"

#include <algorithm>
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
        if (current().type == TokenType::Using) {
            consume();
            if (current().type != TokenType::Identifier) throw std::runtime_error("Expected namespace name after 'using'");
            std::string namespaceName = consume().text;
            while (current().type == TokenType::Dot) {
                consume();
                if (current().type != TokenType::Identifier) throw std::runtime_error("Expected namespace component after '.'");
                namespaceName += "." + consume().text;
            }
            if (std::find(usingNamespaces_.begin(), usingNamespaces_.end(), namespaceName) == usingNamespaces_.end())
                usingNamespaces_.push_back(namespaceName);
            if (current().type == TokenType::Semicolon) consume();
            if (namespaceName == "std") usingStd_ = true;
        }
        else if (current().type == TokenType::Func) {
            program.functions.push_back(parseFunction());
        }
        else if (current().type == TokenType::NamespaceKeyword) {
            parseNamespace(program);
        }
        else {
            throw std::runtime_error(
                "Unexpected token: " + current().text
            );
        }
    }

    program.usingNamespaces = usingNamespaces_;
    return program;
}

void Parser::parseNamespace(Program& program)
{
    expect(TokenType::NamespaceKeyword);
    if (current().type != TokenType::Identifier) throw std::runtime_error("Expected namespace name");
    const std::string localName = consume().text;
    const std::string previousNamespace = currentNamespace_;
    expect(TokenType::LeftBrace);
    currentNamespace_ = previousNamespace.empty() ? localName : previousNamespace + "." + localName;
    try {
        while (current().type != TokenType::RightBrace) {
            if (current().type == TokenType::Func) program.functions.push_back(parseFunction());
            else if (current().type == TokenType::NamespaceKeyword) parseNamespace(program);
            else throw std::runtime_error("Namespaces can contain function declarations or nested namespaces");
        }
        expect(TokenType::RightBrace);
    } catch (...) {
        currentNamespace_ = previousNamespace;
        throw;
    }
    currentNamespace_ = previousNamespace;
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
    Function function;
    function.name = currentNamespace_.empty() ? name.text : currentNamespace_ + "." + name.text;
    if (current().type != TokenType::RightParen) {
        do {
            if (current().type != TokenType::Identifier) throw std::runtime_error("Expected parameter name");
            std::string parameterName = consume().text;
            expect(TokenType::Colon);
            function.parameters.emplace_back(std::move(parameterName), parseType());
            if (current().type != TokenType::Comma) break;
            consume();
        } while (true);
    }
    expect(TokenType::RightParen);
    if (current().type == TokenType::Arrow) {
        consume();
        function.returnType = parseType(true);
    }
    function.statements = parseBlock();

    return function;
}

std::string Parser::parseType(bool allowVoid)
{
    const Token& type = current();
    if (type.type != TokenType::Int && type.type != TokenType::IntegerType && type.type != TokenType::Bool &&
        type.type != TokenType::StringType && type.type != TokenType::CharType &&
        !(type.type == TokenType::Identifier && (type.text == "float" || type.text == "double" || type.text == "decimal" || type.text == "any")) &&
        !(allowVoid && type.type == TokenType::VoidType)) {
        throw std::runtime_error("Expected type");
    }
    consume();
    return type.text;
}

Statement Parser::parseStatement()
{
    if (current().type == TokenType::Return) {
        consume();
        Statement statement;
        statement.type = Statement::Type::Return;
        if (current().type != TokenType::Semicolon && current().type != TokenType::RightBrace) statement.expression = parseExpression();
        if (current().type == TokenType::Semicolon) consume();
        return statement;
    }
    if (current().type != TokenType::Identifier && current().type != TokenType::Const) {
        throw std::runtime_error(
            "Expected statement"
        );
    }

    if (current().text == "std" && peek().type == TokenType::Dot) {
        consume();
        consume();
        if (current().type == TokenType::Identifier && (current().text == "print" || current().text == "println")) {
            return parsePrint();
        }
        throw std::runtime_error("Unknown function in std namespace");
    }
    if (current().text == "print" || current().text == "println") {
        if (!usingStd_) throw std::runtime_error("Use 'std." + current().text + "(...)' or add 'using std;'");
        return parsePrint();
    }
    if (current().text == "if") return parseIf();

    if (peek().type == TokenType::LeftParen || peek().type == TokenType::Dot) {
        Statement statement;
        statement.type = Statement::Type::ExpressionStatement;
        statement.expression = parseExpression();
        return statement;
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
        case TokenType::Equal: statement.operation = "="; break;
        case TokenType::PlusEqual: statement.operation = "+"; break;
        case TokenType::MinusEqual: statement.operation = "-"; break;
        case TokenType::MultiplyEqual: statement.operation = "*"; break;
        case TokenType::DivideEqual: statement.operation = "/"; break;
        case TokenType::RemainderEqual: statement.operation = "%"; break;
        case TokenType::PowerEqual: statement.operation = "^"; break;
        case TokenType::Increment: statement.operation = "+"; break;
        case TokenType::Decrement: statement.operation = "-"; break;
        default: throw std::runtime_error("Invalid assignment operator");
        }
        if (op != TokenType::Increment && op != TokenType::Decrement) statement.expression = parseExpression();
        return statement;
    }

    if (peek().type == TokenType::ColonEqual) {
        Statement statement;
        statement.type = Statement::Type::VariableDeclaration;
        statement.variable = std::make_unique<VariableDeclaration>(parseVariable());
        return statement;
    }

    Statement statement;
    statement.type = Statement::Type::VariableDeclaration;

    statement.variable = std::make_unique<VariableDeclaration>(
        parseVariable()
    );

    return statement;
}

std::vector<Statement> Parser::parseBlock()
{
    expect(TokenType::LeftBrace);
    std::vector<Statement> statements;
    while (current().type != TokenType::RightBrace) statements.push_back(parseStatement());
    expect(TokenType::RightBrace);
    return statements;
}

Statement Parser::parseIf()
{
    Statement statement;
    statement.type = Statement::Type::If;
    consume(); // if
    auto parseCondition = [this]() {
        const bool parenthesized = current().type == TokenType::LeftParen;
        if (parenthesized) consume();
        auto condition = parseExpression();
        if (parenthesized) expect(TokenType::RightParen);
        return condition;
    };
    statement.branches.push_back({parseCondition(), parseBlock()});
    while (current().type == TokenType::Identifier && current().text == "elif") {
        consume();
        statement.branches.push_back({parseCondition(), parseBlock()});
    }
    if (current().type == TokenType::Identifier && current().text == "else") {
        consume();
        statement.elseStatements = parseBlock();
    }
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
    const bool isConst = current().type == TokenType::Const;
    if (isConst) consume();
    VariableDeclaration variable;
    variable.isConst = isConst;
    if (current().type != TokenType::Identifier) throw std::runtime_error("Expected variable name");
    variable.names.push_back(consume().text);
    while (current().type == TokenType::Comma) {
        consume();
        if (current().type != TokenType::Identifier) throw std::runtime_error("Expected variable name after ','");
        variable.names.push_back(consume().text);
    }

    variable.type = "auto";
    bool inferred = false;
    if (current().type == TokenType::ColonEqual) { consume(); inferred = true; }
    else {
        expect(TokenType::Colon);
        variable.type = parseType();
    }
    if (current().type == TokenType::Equal) {
        consume();
        do {
            variable.values.push_back(parseExpression());
            if (current().type != TokenType::Comma) break;
            consume();
        } while (true);
        if (variable.values.size() != variable.names.size())
            throw std::runtime_error("Grouped declaration must have one initializer per variable");
    } else if (inferred || isConst) {
        throw std::runtime_error("Inferred or const variables need an initializer");
    }

    return variable;
}
