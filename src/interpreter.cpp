#include "interpreter.h"
#include "interpreter_internal.h"
#include "semantic_analysis.h"

#include <limits>
#include <stdexcept>

namespace rift {

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
    if (const auto* array = std::get_if<std::shared_ptr<RuntimeArray>>(&value)) return (*array)->elementType + "[]";
    if (std::holds_alternative<std::u16string>(value)) return "string";
    if (std::holds_alternative<char16_t>(value)) return "char";
    throw std::runtime_error("Cannot infer a type from a void value");
}

template <typename ScopeStack>
struct ScopePopGuard {
    ScopeStack& scopes;
    ~ScopePopGuard() { scopes.pop_back(); }
};

template <typename SlotVector>
struct SlotClearGuard {
    SlotVector& slots;
    const std::vector<std::size_t>* indices;
    ~SlotClearGuard() { if (indices) for (const auto index : *indices) slots[index] = {}; }
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
    const AnalysisResult analysis = validateProgram(program);
    analysis_ = &analysis;
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

    auto previousLocals = std::move(locals_);
    locals_.resize(analysis_->functionSlotCounts.at(&function));
    try {
        for (std::size_t i = 0; i < arguments.size(); ++i) {
            const auto& [parameterName, parameterType] = function.parameters[i];
            locals_[i] = {validateValue(arguments[i], parameterType, parameterName), parameterType, false};
        }
        executeStatements(function.statements);
        RuntimeValue result = std::monostate{};
        if (flow_.type == ExecutionFlow::Type::Return) {
            auto signal = std::move(flow_);
            flow_ = {};
            if (function.returnType.empty() || function.returnType == "void") {
                if (signal.returnValue) throw std::runtime_error("Void function '" + name + "' cannot return a value");
            } else {
                if (!signal.returnValue) throw std::runtime_error("Function '" + name + "' must return a value of type " + function.returnType);
                result = validateValue(*signal.returnValue, function.returnType, "return value");
            }
        } else if (!function.returnType.empty() && function.returnType != "void") {
            throw std::runtime_error("Function '" + name + "' completed without returning a value");
        }
        locals_ = std::move(previousLocals);
        return result;
    } catch (...) {
        flow_ = {};
        locals_ = std::move(previousLocals);
        throw;
    }
}

void Interpreter::executeStatements(const std::vector<Statement>& statements, bool clearSlots)
{
    if (flow_.type != ExecutionFlow::Type::Normal) return;
    const auto scope = analysis_->scopeSlots.find(&statements);
    SlotClearGuard slotCleanup{locals_, !clearSlots || scope == analysis_->scopeSlots.end() ? nullptr : &scope->second};
    for (const auto& statement : statements) {
        if (statement.type == Statement::Type::VariableDeclaration) {
            const auto& declaration = *statement.variable;
            for (std::size_t i = 0; i < declaration.names.size(); ++i) {
                const auto& name = declaration.names[i];
                RuntimeValue value = std::monostate{};
                std::string type = declaration.type;
                if (!declaration.values.empty()) {
                    RuntimeValue evaluated = evaluate(*declaration.values[i]);
                    if (flow_.type != ExecutionFlow::Type::Normal) return;
                    if (type == "auto") type = inferType(evaluated);
                    value = validateValue(std::move(evaluated), type, name);
                }
                const auto slot = analysis_->declarationSlots.at(&declaration) + i;
                locals_[slot] = {std::move(value), std::move(type), declaration.isConst};
            }
        } else if (statement.type == Statement::Type::Assignment) {
            if (statement.name.ends_with(".limit")) {
                const std::string target = statement.name.substr(0, statement.name.size() - 6);
                RuntimeValue evaluated = evaluate(*statement.expression);
                if (flow_.type != ExecutionFlow::Type::Normal) return;
                const std::uint64_t value = loopCount(evaluated, "Loop limit");
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
            auto& variable = locals_[analysis_->assignmentSlots.at(&statement)];
            if (variable.isConst) throw std::runtime_error("Cannot assign to const variable: " + statement.name);
            if (!statement.indices.empty()) {
                auto* array = std::get_if<std::shared_ptr<RuntimeArray>>(&variable.value);
                if (!array || !*array) throw std::runtime_error("Cannot index a non-array value");
                for (std::size_t i = 0; i < statement.indices.size(); ++i) {
                    ensureUniqueArray(*array);
                    RuntimeValue indexValue = evaluate(*statement.indices[i]);
                    if (flow_.type != ExecutionFlow::Type::Normal) return;
                    const auto index = loopCount(indexValue, "Array index");
                    if (index >= (*array)->elements.size()) throw std::runtime_error("Array index out of range");
                    auto& element = (*array)->elements[static_cast<std::size_t>(index)];
                if (i + 1 == statement.indices.size()) {
                        RuntimeValue assigned = evaluate(*statement.expression);
                        if (flow_.type != ExecutionFlow::Type::Normal) return;
                        element = validateValue(std::move(assigned), (*array)->elementType, "array element");
                    } else {
                        array = std::get_if<std::shared_ptr<RuntimeArray>>(&element);
                        if (!array || !*array) throw std::runtime_error("Cannot index a non-array value");
                    }
                }
                continue;
            }
            RuntimeValue value;
            if (statement.operation == "=") {
                value = evaluate(*statement.expression);
                if (flow_.type != ExecutionFlow::Type::Normal) return;
            } else {
                RuntimeValue currentValue = variable.value;
                if (std::holds_alternative<std::monostate>(currentValue)) throw std::runtime_error("Compound assignment to uninitialized variable: " + statement.name);
                if (std::holds_alternative<bool>(currentValue)) throw std::runtime_error("Boolean variables do not support compound arithmetic assignment");
                RuntimeValue right;
                if (statement.expression) {
                    right = evaluate(*statement.expression);
                    if (flow_.type != ExecutionFlow::Type::Normal) return;
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
            for (const auto& argument : statement.arguments) {
                values.push_back(evaluate(*argument));
                if (flow_.type != ExecutionFlow::Type::Normal) return;
            }
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
                RuntimeValue condition = evaluate(*branch.condition);
                if (flow_.type != ExecutionFlow::Type::Normal) return;
                if (requireBoolean(condition, "if condition")) {
                    executeStatements(branch.statements);
                    matched = true;
                    break;
                }
            }
            if (!matched && !statement.elseStatements.empty()) {
                executeStatements(statement.elseStatements);
            }
        } else if (statement.type == Statement::Type::For || statement.type == Statement::Type::While) {
            const bool isForLoop = statement.type == Statement::Type::For;
            const auto loopSlots = analysis_->loopScopeSlots.find(&statement);
            SlotClearGuard loopScopeCleanup{locals_, loopSlots == analysis_->loopScopeSlots.end() ? nullptr : &loopSlots->second};
            if (isForLoop) executeStatements(statement.initializer, false);
            if (flow_.type != ExecutionFlow::Type::Normal) return;
            bool runElse = true;
            {
                LoopState initialState;
                initialState.name = statement.loopName;
                initialState.supportsRestartSkip = isForLoop;
                loopStates_.push_back(std::move(initialState));
                ScopePopGuard loopStateGuard{loopStates_};
                bool completedNormally = false;
                bool shouldBreak = false;
                bool shouldRestart = false;
                const auto handleLoopFlow = [&]() {
                    if (flow_.type == ExecutionFlow::Type::Normal) return true;
                    if (flow_.type == ExecutionFlow::Type::Return) return false;
                    const std::string& currentName = loopStates_.back().name;
                    if (!flow_.target.empty() && flow_.target != currentName) return false;
                    if (flow_.type == ExecutionFlow::Type::Break) shouldBreak = true;
                    else if (flow_.type == ExecutionFlow::Type::Restart) shouldRestart = true;
                    flow_ = {};
                    return true;
                };
                while (true) {
                    if (statement.expression) {
                        RuntimeValue condition = evaluate(*statement.expression);
                        if (flow_.type != ExecutionFlow::Type::Normal) {
                            const auto conditionFlow = flow_.type;
                            if (!handleLoopFlow()) { completedNormally = false; break; }
                            if (shouldBreak) { completedNormally = false; break; }
                            if (shouldRestart) {
                                loopStates_.back().iterations = 0;
                                loopStates_.back().limit.reset();
                                shouldRestart = false;
                                runElse = false;
                                continue;
                            }
                            if (conditionFlow == ExecutionFlow::Type::Continue) {
                                if (isForLoop) executeStatements(statement.post, false);
                                if (flow_.type != ExecutionFlow::Type::Normal && !handleLoopFlow()) { completedNormally = false; break; }
                                if (shouldBreak) { completedNormally = false; break; }
                                if (shouldRestart) {
                                    loopStates_.back().iterations = 0;
                                    loopStates_.back().limit.reset();
                                    shouldRestart = false;
                                    runElse = false;
                                }
                                continue;
                            }
                        }
                        if (!requireBoolean(condition, isForLoop ? "for-loop condition" : "while condition")) {
                            completedNormally = true;
                            break;
                        }
                    }
                    auto& state = loopStates_.back();
                    if (state.limit && state.iterations >= *state.limit) {
                        completedNormally = true;
                        break;
                    }
                    shouldBreak = false;
                    shouldRestart = false;
                    if (state.skipsRemaining > 0) {
                        --state.skipsRemaining;
                        if (isForLoop) executeStatements(statement.post, false);
                        if (!handleLoopFlow()) { completedNormally = false; break; }
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
                    executeStatements(statement.loopStatements);
                    if (!handleLoopFlow()) { completedNormally = false; break; }
                    if (shouldBreak) { completedNormally = false; break; }
                    if (shouldRestart) {
                        loopStates_.back().iterations = 0;
                        loopStates_.back().limit.reset();
                        runElse = false;
                        continue;
                    }
                    if (isForLoop) {
                        executeStatements(statement.post, false);
                        if (!handleLoopFlow()) { completedNormally = false; break; }
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
                executeStatements(statement.loopElseStatements);
            }
        } else if (statement.type == Statement::Type::Break) {
            flow_.type = ExecutionFlow::Type::Break;
            flow_.target = statement.name;
        } else if (statement.type == Statement::Type::Return) {
            if (statement.expression) {
                RuntimeValue value = evaluate(*statement.expression);
                if (flow_.type != ExecutionFlow::Type::Normal) return;
                flow_.returnValue = std::move(value);
            }
            flow_.type = ExecutionFlow::Type::Return;
        } else if (statement.type == Statement::Type::ExpressionStatement) {
            evaluate(*statement.expression);
        }
        if (flow_.type != ExecutionFlow::Type::Normal) return;
    }
}

}
