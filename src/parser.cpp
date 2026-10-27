#include "parser.h"

#include <algorithm>
#include <stdexcept>

namespace {
std::string tokenName(TokenType type)
{
    switch (type) {
    case TokenType::LeftParen: return "'('";
    case TokenType::RightParen: return "')'";
    case TokenType::LeftBrace: return "'{'";
    case TokenType::RightBrace: return "'}'";
    case TokenType::Colon: return "':'";
    case TokenType::Equal: return "'='";
    case TokenType::Identifier: return "identifier";
    case TokenType::Number: return "number";
    case TokenType::EndOfFile: return "end of file";
    default: return "token";
    }
}
std::string tokenDescription(const Token& token)
{
    const std::string actual = token.type == TokenType::EndOfFile ? "end of file" : "'" + token.text + "'";
    return actual + " at line " + std::to_string(token.line) + ", column " + std::to_string(token.column);
}
}

Parser::Parser(const std::vector<Token>& tokens)
    : tokens_(tokens)
{
}

const Token& Parser::current() const
{
    static const Token eof{TokenType::EndOfFile, "<end of file>", 1, 1};
    if (position_ >= tokens_.size()) return tokens_.empty() ? eof : tokens_.back();
    return tokens_[position_];
}

const Token& Parser::peek() const
{
    if (position_ + 1 >= tokens_.size()) return tokens_.empty() ? current() : tokens_.back();
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
        throw std::runtime_error("Expected " + tokenName(type) + ", found " + tokenDescription(current()));
    }

    consume();
}

Program Parser::parse()
{
    Program program;

    while (current().type != TokenType::EndOfFile) {
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
        while (current().type != TokenType::RightBrace && current().type != TokenType::EndOfFile) {
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
        throw std::runtime_error("Expected type, found " + tokenDescription(type));
    }
    consume();
    return type.text;
}

Statement Parser::parseStatement()
{
    if (current().type == TokenType::Break) {
        consume();
        Statement statement;
        statement.type = Statement::Type::Break;
        if (current().type == TokenType::Identifier) statement.name = consume().text;
        if (current().type == TokenType::Semicolon) consume();
        return statement;
    }
    if (current().type == TokenType::For) return parseFor();
    if (current().type == TokenType::Identifier && position_ + 2 < tokens_.size() &&
        tokens_[position_ + 1].type == TokenType::Colon && tokens_[position_ + 2].type == TokenType::For) {
        const std::string loopName = consume().text;
        consume();
        return parseFor(loopName);
    }
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

    if (current().text == "std" && peek().type == TokenType::Dot && position_ + 2 < tokens_.size() &&
        tokens_[position_ + 2].type == TokenType::Identifier &&
        (tokens_[position_ + 2].text == "print" || tokens_[position_ + 2].text == "println")) {
        consume(); consume();
        if (current().type == TokenType::Identifier) {
            return parsePrint();
        }
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

Statement Parser::parseFor(std::string loopName)
{
    expect(TokenType::For);
    const bool parenthesized = current().type == TokenType::LeftParen;
    if (parenthesized) consume();

    Statement loop;
    loop.type = Statement::Type::For;
    loop.loopName = std::move(loopName);

    if (current().type == TokenType::LeftBrace) {
        loop.expression = std::make_unique<Expression>();
        loop.expression->type = Expression::Type::Boolean;
        loop.expression->value = true;
    } else if (current().type == TokenType::Identifier &&
        (peek().type == TokenType::Colon || peek().type == TokenType::ColonEqual)) {
        VariableDeclaration declaration;
        declaration.names.push_back(consume().text);
        declaration.type = "auto";
        bool inferred = false;
        if (current().type == TokenType::ColonEqual) { consume(); inferred = true; }
        else { consume(); declaration.type = parseType(); }
        if (current().type == TokenType::Equal) consume();
        else if (!inferred) throw std::runtime_error("Expected '=' after for-loop variable declaration");
        declaration.values.push_back(parseExpression());
        Statement initializer;
        initializer.type = Statement::Type::VariableDeclaration;
        initializer.variable = std::make_unique<VariableDeclaration>(std::move(declaration));
        loop.initializer.push_back(std::move(initializer));

        if (current().type == TokenType::Comma) {
            consume();
            loop.expression = parseExpression();
            expect(TokenType::Semicolon);
            Statement post = parseStatement();
            loop.post.push_back(std::move(post));
        } else if (current().type == TokenType::Semicolon) {
            consume();
            if (current().type != TokenType::Semicolon) loop.expression = parseExpression();
            expect(TokenType::Semicolon);
            if (current().type != TokenType::LeftBrace && current().type != TokenType::RightParen)
                loop.post.push_back(parseStatement());
        } else {
            throw std::runtime_error("Expected ',' or ';' after for-loop initializer");
        }
    } else if (current().type == TokenType::Semicolon) {
        consume();
        if (current().type != TokenType::Semicolon) loop.expression = parseExpression();
        expect(TokenType::Semicolon);
        if (current().type != TokenType::LeftBrace && current().type != TokenType::RightParen)
            loop.post.push_back(parseStatement());
    } else {
        loop.expression = parseExpression();
        if (current().type == TokenType::Semicolon) {
            consume();
            if (current().type != TokenType::LeftBrace && current().type != TokenType::RightParen)
                loop.post.push_back(parseStatement());
        }
    }

    if (parenthesized) expect(TokenType::RightParen);
    loop.loopStatements = parseBlock();
    return loop;
}

std::vector<Statement> Parser::parseBlock()
{
    expect(TokenType::LeftBrace);
    std::vector<Statement> statements;
    while (current().type != TokenType::RightBrace && current().type != TokenType::EndOfFile) statements.push_back(parseStatement());
    if (current().type == TokenType::EndOfFile) throw std::runtime_error("Expected '}', found " + tokenDescription(current()));
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
    if (current().type == TokenType::Equal || inferred) {
        if (current().type == TokenType::Equal) consume();
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
