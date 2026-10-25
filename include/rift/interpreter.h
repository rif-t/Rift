#pragma once

#include "ast.h"

#include <string>
#include <variant>
#include <unordered_map>
#include <vector>

class Interpreter
{
public:
    void run(const Program& program);

private:
    void runFunction(const Function& function);
    void executeStatements(const std::vector<Statement>& statements);
    RuntimeValue callFunction(const std::string& name, const std::vector<RuntimeValue>& arguments);

    RuntimeValue evaluate(const Expression& expression);

    struct Variable { RuntimeValue value; std::string type; bool isConst = false; };
    using Scope = std::unordered_map<std::string, Variable>;
    std::vector<Scope> scopes_;
    std::unordered_map<std::string, const Function*> functions_;
    std::vector<std::string> usingNamespaces_;
};
