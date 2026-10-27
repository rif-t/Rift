#pragma once

#include "ast.h"

#include <string>
#include <variant>
#include <unordered_map>
#include <optional>
#include <cstdint>
#include <vector>

class Interpreter
{
public:
    void run(const Program& program);

private:
    void executeStatements(const std::vector<Statement>& statements);
    RuntimeValue callFunction(const std::string& name, const std::vector<RuntimeValue>& arguments, const Function* selected = nullptr);

    RuntimeValue evaluate(const Expression& expression);

    struct Variable { RuntimeValue value; std::string type; bool isConst = false; };
    struct LoopState {
        std::string name;
        std::uint64_t iterations = 0;
        std::optional<std::uint64_t> limit;
        std::uint64_t skipsRemaining = 0;
        bool supportsRestartSkip = true;
    };
    using Scope = std::unordered_map<std::string, Variable>;
    std::vector<Scope> scopes_;
    std::unordered_map<std::string, std::vector<const Function*>> functions_;
    std::vector<std::string> usingNamespaces_;
    std::vector<LoopState> loopStates_;
};
