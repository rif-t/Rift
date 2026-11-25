#pragma once

#include "ast.h"
#include "semantic_analysis.h"

#include <string>
#include <variant>
#include <optional>
#include <cstdint>
#include <vector>

namespace rift {

class Interpreter
{
public:
    void run(const Program& program);

private:
    void executeStatements(const std::vector<Statement>& statements, bool clearSlots = true);
    RuntimeValue callFunction(const std::string& name, const std::vector<RuntimeValue>& arguments, const Function* selected = nullptr);

    RuntimeValue evaluate(const Expression& expression);
    RuntimeValue evaluateImpl(const Expression& expression);

    struct Variable { RuntimeValue value; std::string type; bool isConst = false; };
    struct LoopState {
        std::string name;
        std::uint64_t iterations = 0;
        std::optional<std::uint64_t> limit;
        std::uint64_t skipsRemaining = 0;
        bool supportsRestartSkip = true;
    };
    struct ExecutionFlow {
        enum class Type { Normal, Return, Break, Continue, Restart } type = Type::Normal;
        std::string target;
        std::optional<RuntimeValue> returnValue;
    };
    struct CallFrame { std::vector<Variable> locals; };
    struct CallFrameGuard {
        Interpreter& interpreter;
        ~CallFrameGuard() { interpreter.callFrames_[--interpreter.callDepth_]->locals.clear(); }
    };
    std::vector<std::unique_ptr<CallFrame>> callFrames_;
    std::size_t callDepth_ = 0;
    std::vector<std::string> usingNamespaces_;
    std::vector<LoopState> loopStates_;
    const AnalysisResult* analysis_ = nullptr;
    ExecutionFlow flow_;
    std::vector<Variable>& locals() { return callFrames_[callDepth_ - 1]->locals; }
};

}
