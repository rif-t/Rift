#include "interpreter.h"
#include "interpreter_internal.h"
#include "semantic_analysis.h"

#include <limits>
#include <stdexcept>

using namespace rift::runtime;

namespace {
std::string inferType(const RuntimeValue& value)
{
    if (const auto* integer = std::get_if<WideInteger>(&value)) {
        const unsigned bits = integer->bitWidth ? integer->bitWidth : 32;
        if (integer->isUnsigned) return "uint" + std::to_string(bits);
        return bits == 32 ? "int" : "int" + std::to_string(bits);
    }
    if (std::holds_alternative<float>(value)) return "float";
    if (std::holds_alternative<double>(value)) return "double";
    if (std::holds_alternative<Decimal>(value)) return "decimal";
    if (std::holds_alternative<bool>(value)) return "bool";
    if (std::holds_alternative<std::u16string>(value)) return "string";
    if (std::holds_alternative<char16_t>(value)) return "char";
    throw std::runtime_error("Cannot infer a type from a void value");
}

template <typename ScopeStack>
struct ScopePopGuard {
    ScopeStack& scopes;
    ~ScopePopGuard() { scopes.pop_back(); }
};

std::uint64_t loopCount(const RuntimeValue& value, const std::string& purpose)
{
    const auto* integer = std::get_if<WideInteger>(&value);
    if (!integer || integer->negative || integer->magnitude.high != 0)
        throw std::runtime_error(purpose + " must be a non-negative integer no larger than uint64");
    return integer->magnitude.low;
}
}

void Interpreter::run(const Program& program)
{
    validateProgram(program);
    functions_.clear();
    usingNamespaces_ = program.usingNamespaces;
    for (const auto& function : program.functions) functions_[function.name].push_back(&function);
    callFunction("main", {});
}

RuntimeValue Interpreter::callFunction(const std::string& name, const std::vector<RuntimeValue>& arguments, const Function* selected)
{
    auto found = functions_.find(name);
    if (!selected && found == functions_.end() && name.find('.') == std::string::npos) {
        for (const auto& namespaceName : usingNamespaces_) {
            if (namespaceName == "std") continue;
            auto candidate = functions_.find(namespaceName + "." + name);
            if (candidate == functions_.end()) continue;
            if (found != functions_.end()) throw std::runtime_error("Ambiguous function '" + name + "'; qualify it with its namespace");
            found = candidate;
        }
    }
    if (!selected && found == functions_.end()) throw std::runtime_error("Undefined function: " + name);
    const Function& function = selected ? *selected : *found->second.front();
    if (arguments.size() != function.parameters.size()) {
        throw std::runtime_error("Function '" + name + "' expects " + std::to_string(function.parameters.size()) + " argument(s)");
    }

    auto previousScopes = std::move(scopes_);
    scopes_.clear();
    scopes_.emplace_back();
    try {
        for (std::size_t i = 0; i < arguments.size(); ++i) {
            const auto& [parameterName, parameterType] = function.parameters[i];
            scopes_.back()[parameterName] = {validateValue(arguments[i], parameterType, parameterName), parameterType, false};
        }
        executeStatements(function.statements);
        if (!function.returnType.empty() && function.returnType != "void") {
            throw std::runtime_error("Function '" + name + "' completed without returning a value");
        }
        scopes_ = std::move(previousScopes);
        return std::monostate{};
    } catch (const ReturnSignal& signal) {
        RuntimeValue result = std::monostate{};
        try {
            if (function.returnType.empty() || function.returnType == "void") {
                if (signal.value) throw std::runtime_error("Void function '" + name + "' cannot return a value");
            } else {
                if (!signal.value) throw std::runtime_error("Function '" + name + "' must return a value of type " + function.returnType);
                result = validateValue(*signal.value, function.returnType, "return value");
            }
        } catch (...) {
            scopes_ = std::move(previousScopes);
            throw;
        }
        scopes_ = std::move(previousScopes);
        return result;
    } catch (...) {
        scopes_ = std::move(previousScopes);
        throw;
    }
}

void Interpreter::executeStatements(const std::vector<Statement>& statements)
{
    for (const auto& statement : statements) {
        if (statement.type == Statement::Type::VariableDeclaration) {
            const auto& declaration = *statement.variable;
            for (std::size_t i = 0; i < declaration.names.size(); ++i) {
                const auto& name = declaration.names[i];
                if (scopes_.back().contains(name)) throw std::runtime_error("Variable already declared in this scope: " + name);
                RuntimeValue value = std::monostate{};
                std::string type = declaration.type;
                if (!declaration.values.empty()) {
                    RuntimeValue evaluated = evaluate(*declaration.values[i]);
                    if (type == "auto") type = inferType(evaluated);
                    value = validateValue(std::move(evaluated), type, name);
                }
                scopes_.back()[name] = {std::move(value), std::move(type), declaration.isConst};
            }
        } else if (statement.type == Statement::Type::Assignment) {
            if (statement.name.ends_with(".limit")) {
                const std::string target = statement.name.substr(0, statement.name.size() - 6);
                const std::uint64_t value = loopCount(evaluate(*statement.expression), "Loop limit");
                auto state = loopStates_.rbegin();
                if (target == "loop") {
                    if (state == loopStates_.rend()) throw std::runtime_error("No active loop for 'loop.limit'");
                } else {
                    while (state != loopStates_.rend() && state->name != target) ++state;
                    if (state == loopStates_.rend()) throw std::runtime_error("No active loop named '" + target + "'");
                }
                state->limit = value;
                continue;
            }
            auto scope = scopes_.rbegin();
            while (scope != scopes_.rend() && !scope->contains(statement.name)) ++scope;
            if (scope == scopes_.rend()) throw std::runtime_error("Cannot assign to undefined variable: " + statement.name);
            auto& variable = scope->at(statement.name);
            if (variable.isConst) throw std::runtime_error("Cannot assign to const variable: " + statement.name);
            RuntimeValue value;
            if (statement.operation == "=") {
                value = evaluate(*statement.expression);
            } else {
                RuntimeValue currentValue = variable.value;
                if (std::holds_alternative<std::monostate>(currentValue)) throw std::runtime_error("Compound assignment to uninitialized variable: " + statement.name);
                if (std::holds_alternative<bool>(currentValue)) throw std::runtime_error("Boolean variables do not support compound arithmetic assignment");
                RuntimeValue right;
                if (statement.expression) {
                    right = evaluate(*statement.expression);
                } else {
                    right = WideInteger{UInt128{0, 1}, false, false};
                }
                if (statement.operation == "+" && std::holds_alternative<std::u16string>(currentValue) &&
                    (std::holds_alternative<std::u16string>(right) || std::holds_alternative<char16_t>(right))) {
                    std::u16string combined = std::get<std::u16string>(currentValue);
                    if (const auto* text = std::get_if<std::u16string>(&right)) combined += *text;
                    else combined.push_back(std::get<char16_t>(right));
                    value = std::move(combined);
                } else {
                    value = applyNumericArithmetic(statement.operation, currentValue, right);
                }
            }
            value = validateValue(std::move(value), variable.type, statement.name);
            variable.value = std::move(value);
        } else if (statement.type == Statement::Type::Print) {
            std::vector<RuntimeValue> values;
            values.reserve(statement.arguments.size());
            for (const auto& argument : statement.arguments) values.push_back(evaluate(*argument));
            std::string output;
            if (!values.empty() && std::holds_alternative<std::u16string>(values.front())) {
                std::vector<rift::fmt::Argument> formatArguments;
                formatArguments.reserve(values.size() - 1);
                for (std::size_t i = 1; i < values.size(); ++i) formatArguments.push_back(formatArgument(values[i]));
                output = rift::fmt::format(utf16ToUtf8(std::get<std::u16string>(values.front())), formatArguments);
            } else {
                for (const auto& value : values) output += stringify(value);
            }
            rift::fmt::write(output, statement.newline);
        } else if (statement.type == Statement::Type::If) {
            bool matched = false;
            for (const auto& branch : statement.branches) {
                if (requireBoolean(evaluate(*branch.condition), "if condition")) {
                    scopes_.emplace_back();
                    ScopePopGuard guard{scopes_};
                    executeStatements(branch.statements);
                    matched = true;
                    break;
                }
            }
            if (!matched && !statement.elseStatements.empty()) {
                scopes_.emplace_back();
                ScopePopGuard guard{scopes_};
                executeStatements(statement.elseStatements);
            }
        } else if (statement.type == Statement::Type::For || statement.type == Statement::Type::While) {
            const bool isForLoop = statement.type == Statement::Type::For;
            scopes_.emplace_back();
            ScopePopGuard loopScope{scopes_};
            if (isForLoop) executeStatements(statement.initializer);
            bool runElse = true;
            {
                LoopState initialState;
                initialState.name = statement.loopName;
                initialState.supportsRestartSkip = isForLoop;
                loopStates_.push_back(std::move(initialState));
                ScopePopGuard loopStateGuard{loopStates_};
                bool completedNormally = false;
                while (true) {
                    if (statement.expression && !requireBoolean(evaluate(*statement.expression), isForLoop ? "for-loop condition" : "while condition")) {
                        completedNormally = true;
                        break;
                    }
                    auto& state = loopStates_.back();
                    if (state.limit && state.iterations >= *state.limit) {
                        completedNormally = true;
                        break;
                    }
                    bool shouldBreak = false;
                    bool shouldRestart = false;
                    const auto handleLoopSignal = [&](const LoopControlSignal& signal) {
                        const std::string& currentName = loopStates_.back().name;
                        if (!signal.target.empty() && signal.target != currentName) return false;
                        if (signal.action == LoopAction::Break) shouldBreak = true;
                        else if (signal.action == LoopAction::Restart) shouldRestart = true;
                        return true;
                    };
                    if (state.skipsRemaining > 0) {
                        --state.skipsRemaining;
                        if (isForLoop) {
                            try {
                                executeStatements(statement.post);
                            } catch (const LoopControlSignal& signal) {
                                if (!handleLoopSignal(signal)) throw;
                            }
                        }
                        if (shouldBreak) { completedNormally = false; break; }
                        if (shouldRestart) {
                            loopStates_.back().iterations = 0;
                            loopStates_.back().limit.reset();
                            runElse = false;
                        }
                        continue;
                    }
                    if (state.iterations == std::numeric_limits<std::uint64_t>::max()) throw std::runtime_error("Loop iteration counter overflow");
                    ++state.iterations;
                    scopes_.emplace_back();
                    {
                        ScopePopGuard bodyScope{scopes_};
                        try {
                            executeStatements(statement.loopStatements);
                        } catch (const LoopControlSignal& signal) {
                            if (!handleLoopSignal(signal)) throw;
                        }
                    }
                    if (shouldBreak) { completedNormally = false; break; }
                    if (shouldRestart) {
                        loopStates_.back().iterations = 0;
                        loopStates_.back().limit.reset();
                        runElse = false;
                        continue;
                    }
                    if (isForLoop) {
                        try {
                            executeStatements(statement.post);
                        } catch (const LoopControlSignal& signal) {
                            if (!handleLoopSignal(signal)) throw;
                        }
                        if (shouldBreak) { completedNormally = false; break; }
                        if (shouldRestart) {
                            loopStates_.back().iterations = 0;
                            loopStates_.back().limit.reset();
                            runElse = false;
                            continue;
                        }
                    }
                }
                if (!completedNormally) runElse = false;
            }
            if (runElse) {
                scopes_.emplace_back();
                ScopePopGuard elseScope{scopes_};
                executeStatements(statement.loopElseStatements);
            }
        } else if (statement.type == Statement::Type::Break) {
            throw LoopControlSignal{statement.name, LoopAction::Break};
        } else if (statement.type == Statement::Type::Return) {
            ReturnSignal signal;
            if (statement.expression) signal.value = evaluate(*statement.expression);
            throw signal;
        } else if (statement.type == Statement::Type::ExpressionStatement) {
            evaluate(*statement.expression);
        }
    }
}
