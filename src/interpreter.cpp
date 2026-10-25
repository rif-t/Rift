#include "interpreter.h"
#include "interpreter_internal.h"

#include <stdexcept>

using namespace rift::runtime;

void Interpreter::run(const Program& program)
{
    functions_.clear();
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
    const auto found = functions_.find(name);
    if (found == functions_.end()) throw std::runtime_error("Undefined function: " + name);
    const Function& function = *found->second;
    if (arguments.size() != function.parameters.size()) {
        throw std::runtime_error("Function '" + name + "' expects " + std::to_string(function.parameters.size()) + " argument(s)");
    }

    auto previousVariables = std::move(variables_);
    auto previousTypes = std::move(variableTypes_);
    variables_.clear();
    variableTypes_.clear();
    try {
        for (std::size_t i = 0; i < arguments.size(); ++i) {
            const auto& [parameterName, parameterType] = function.parameters[i];
            variables_[parameterName] = validateValue(arguments[i], parameterType, parameterName);
            variableTypes_[parameterName] = parameterType;
        }
        executeStatements(function.statements);
        if (!function.returnType.empty() && function.returnType != "void") {
            throw std::runtime_error("Function '" + name + "' completed without returning a value");
        }
        variables_ = std::move(previousVariables);
        variableTypes_ = std::move(previousTypes);
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
            variables_ = std::move(previousVariables);
            variableTypes_ = std::move(previousTypes);
            throw;
        }
        variables_ = std::move(previousVariables);
        variableTypes_ = std::move(previousTypes);
        return result;
    } catch (...) {
        variables_ = std::move(previousVariables);
        variableTypes_ = std::move(previousTypes);
        throw;
    }
}

void Interpreter::executeStatements(const std::vector<Statement>& statements)
{
    for (const auto& statement : statements) {
        if (statement.type == Statement::Type::VariableDeclaration) {
            const auto& declaration = *statement.variable;
            RuntimeValue value = validateValue(evaluate(*declaration.value), declaration.type, declaration.name);
            variables_[declaration.name] = value;
            variableTypes_[declaration.name] = declaration.type;
        } else if (statement.type == Statement::Type::Assignment) {
            auto declaredType = variableTypes_.find(statement.name);
            if (declaredType == variableTypes_.end()) throw std::runtime_error("Cannot assign to undefined variable: " + statement.name);
            RuntimeValue value;
            if (statement.operation == "=") {
                value = evaluate(*statement.expression);
            } else {
                RuntimeValue currentValue = variables_.at(statement.name);
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
            value = validateValue(std::move(value), declaredType->second, statement.name);
            variables_[statement.name] = value;
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
                    executeStatements(branch.statements);
                    matched = true;
                    break;
                }
            }
            if (!matched) executeStatements(statement.elseStatements);
        } else if (statement.type == Statement::Type::Return) {
            ReturnSignal signal;
            if (statement.expression) signal.value = evaluate(*statement.expression);
            throw signal;
        } else if (statement.type == Statement::Type::ExpressionStatement) {
            evaluate(*statement.expression);
        }
    }
}
