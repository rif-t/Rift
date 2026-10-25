#pragma once

#include "ast.h"

#include <string>
#include <variant>
#include <unordered_map>

class Interpreter
{
public:
    void run(const Program& program);

private:
    void runFunction(const Function& function);
    void executeStatements(const std::vector<Statement>& statements);

    RuntimeValue evaluate(const Expression& expression);

    std::unordered_map<std::string, RuntimeValue> variables_;
    std::unordered_map<std::string, std::string> variableTypes_;
};
