#include "interpreter.h"
#include "interpreter_internal.h"

#include <stdexcept>

using namespace rift::runtime;

namespace {
std::string inferType(const RuntimeValue& value)
{
    if (std::holds_alternative<WideInteger>(value)) return "int";
    if (std::holds_alternative<long double>(value)) return "double";
    if (std::holds_alternative<bool>(value)) return "bool";
    if (std::holds_alternative<std::string>(value)) return "string";
    if (std::holds_alternative<char>(value)) return "char";
    throw std::runtime_error("Cannot infer a type from a void value");
}
}

void Interpreter::run(const Program& program)
{
    functions_.clear();
    usingNamespaces_ = program.usingNamespaces;
    for (const auto& function : program.functions) {
        if (!functions_.emplace(function.name, &function).second) throw std::runtime_error("Duplicate function: " + function.name);
    }
    for (const auto& function : program.functions) {
        if (function.name == "main") {
            runFunction(function);
            return;
        }
    }
    throw std::runtime_error("No main function");
}

void Interpreter::runFunction(const Function& function)
{
    callFunction(function.name, {});
}

RuntimeValue Interpreter::callFunction(const std::string& name, const std::vector<RuntimeValue>& arguments)
{
    auto found = functions_.find(name);
    if (found == functions_.end() && name.find('.') == std::string::npos) {
        for (const auto& namespaceName : usingNamespaces_) {
            if (namespaceName == "std") continue;
            auto candidate = functions_.find(namespaceName + "." + name);
            if (candidate == functions_.end()) continue;
            if (found != functions_.end()) throw std::runtime_error("Ambiguous function '" + name + "'; qualify it with its namespace");
            found = candidate;
        }
    }
    if (found == functions_.end()) throw std::runtime_error("Undefined function: " + name);
    const Function& function = *found->second;
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
                scopes_.back()[name] = {value, type, declaration.isConst};
            }
        } else if (statement.type == Statement::Type::Assignment) {
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
                WideInteger right;
                if (statement.expression) {
                    RuntimeValue evaluated = evaluate(*statement.expression);
                    if (std::holds_alternative<bool>(evaluated)) throw std::runtime_error("Boolean values cannot be used with arithmetic operators");
                    right = std::get<WideInteger>(evaluated);
                } else {
                    right = WideInteger{UInt128{0, 1}, false, false};
                }
                value = applyArithmetic(statement.operation, std::get<WideInteger>(currentValue), right);
            }
            value = validateValue(std::move(value), variable.type, statement.name);
            variable.value = value;
        } else if (statement.type == Statement::Type::Print) {
            std::vector<RuntimeValue> values;
            values.reserve(statement.arguments.size());
            for (const auto& argument : statement.arguments) values.push_back(evaluate(*argument));
            std::string output;
            if (!values.empty() && std::holds_alternative<std::string>(values.front())) {
                std::vector<rift::fmt::Argument> formatArguments;
                formatArguments.reserve(values.size() - 1);
                for (std::size_t i = 1; i < values.size(); ++i) formatArguments.push_back(formatArgument(values[i]));
                output = rift::fmt::format(std::get<std::string>(values.front()), formatArguments);
            } else {
                for (const auto& value : values) output += stringify(value);
            }
            rift::fmt::write(output, statement.newline);
        } else if (statement.type == Statement::Type::If) {
            bool matched = false;
            for (const auto& branch : statement.branches) {
                if (requireBoolean(evaluate(*branch.condition), "if condition")) {
                    scopes_.emplace_back();
                    executeStatements(branch.statements);
                    scopes_.pop_back();
                    matched = true;
                    break;
                }
            }
            if (!matched && !statement.elseStatements.empty()) {
                scopes_.emplace_back();
                executeStatements(statement.elseStatements);
                scopes_.pop_back();
            }
        } else if (statement.type == Statement::Type::Return) {
            ReturnSignal signal;
            if (statement.expression) signal.value = evaluate(*statement.expression);
            throw signal;
        } else if (statement.type == Statement::Type::ExpressionStatement) {
            evaluate(*statement.expression);
        }
    }
}
