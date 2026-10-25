#include "interpreter.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <cmath>
#include <stdexcept>
#include <type_traits>
#include <variant>

void Interpreter::run(const Program& program)
{
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
    variables_.clear();
    const auto variableCount = std::count_if(
        function.statements.begin(),
        function.statements.end(),
        [](const Statement& statement) {
            return statement.type == Statement::Type::VariableDeclaration;
        }
    );
    variables_.reserve(variableCount);

    for (const auto& statement : function.statements) {
        if (statement.type == Statement::Type::VariableDeclaration) {
            const auto& variable = *statement.variable;

            RuntimeValue value = evaluate(*variable.value);

            if (variable.type == "bool" && !std::holds_alternative<bool>(value)) {
                throw std::runtime_error("Value for '" + variable.name + "' must be bool");
            }
            if (variable.type != "bool" && std::holds_alternative<bool>(value)) {
                throw std::runtime_error("Value for '" + variable.name + "' must be an integer");
            }

            const bool isUnsigned = variable.type.starts_with("uint");
            if (isUnsigned) {
                std::uint64_t unsignedValue = 0;
                if (const auto* value64 = std::get_if<std::int64_t>(&value)) {
                    if (*value64 < 0) throw std::runtime_error("Unsigned integer cannot be negative");
                    unsignedValue = static_cast<std::uint64_t>(*value64);
                } else if (const auto* value64 = std::get_if<std::uint64_t>(&value)) {
                    unsignedValue = *value64;
                }
                value = unsignedValue;
            }

            variables_[variable.name] = value;
        }
        else if (statement.type == Statement::Type::Print) {
            RuntimeValue value = evaluate(*statement.expression);
            std::visit([](const auto& item) { std::cout << item << '\n'; }, value);
        }
    }
}

RuntimeValue Interpreter::evaluate(const Expression& expression)
{
    switch (expression.type) {
    case Expression::Type::Number:
        return expression.value;

    case Expression::Type::Identifier:
    {
        auto it = variables_.find(expression.name);

        if (it == variables_.end()) {
            throw std::runtime_error(
                "Undefined variable: " + expression.name
            );
        }

        return it->second;
    }

    case Expression::Type::Binary:
    {
        RuntimeValue leftValue = evaluate(*expression.left);
        RuntimeValue rightValue = evaluate(*expression.right);
        if (std::holds_alternative<bool>(leftValue) || std::holds_alternative<bool>(rightValue)) {
            throw std::runtime_error("Boolean values cannot be used with arithmetic operators");
        }
        const bool unsignedMode = std::holds_alternative<std::uint64_t>(leftValue) || std::holds_alternative<std::uint64_t>(rightValue);
        const std::int64_t leftSigned = std::holds_alternative<std::int64_t>(leftValue) ? std::get<std::int64_t>(leftValue) : static_cast<std::int64_t>(std::get<std::uint64_t>(leftValue));
        const std::int64_t rightSigned = std::holds_alternative<std::int64_t>(rightValue) ? std::get<std::int64_t>(rightValue) : static_cast<std::int64_t>(std::get<std::uint64_t>(rightValue));
        const std::uint64_t leftUnsigned = std::holds_alternative<std::uint64_t>(leftValue) ? std::get<std::uint64_t>(leftValue) : static_cast<std::uint64_t>(leftSigned);
        const std::uint64_t rightUnsigned = std::holds_alternative<std::uint64_t>(rightValue) ? std::get<std::uint64_t>(rightValue) : static_cast<std::uint64_t>(rightSigned);

        switch (expression.operation) {
        case '+':
            return unsignedMode ? RuntimeValue(leftUnsigned + rightUnsigned) : RuntimeValue(leftSigned + rightSigned);
        case '-':
            return unsignedMode ? RuntimeValue(leftUnsigned - rightUnsigned) : RuntimeValue(leftSigned - rightSigned);
        case '*':
            return unsignedMode ? RuntimeValue(leftUnsigned * rightUnsigned) : RuntimeValue(leftSigned * rightSigned);
        case '/':
            if ((unsignedMode && rightUnsigned == 0) || (!unsignedMode && rightSigned == 0)) {
                throw std::runtime_error("Division by zero");
            }
            return unsignedMode ? RuntimeValue(leftUnsigned / rightUnsigned) : RuntimeValue(leftSigned / rightSigned);
        case '^':
            if (!unsignedMode && rightSigned < 0) {
                throw std::runtime_error("Exponent must be non-negative for integer values");
            }
            {
                std::uint64_t result = 1;
                std::uint64_t base = unsignedMode ? leftUnsigned : static_cast<std::uint64_t>(leftSigned);
                std::uint64_t exponent = unsignedMode ? rightUnsigned : static_cast<std::uint64_t>(rightSigned);
                while (exponent > 0) {
                    if (exponent % 2 != 0) result *= base;
                    exponent /= 2;
                    if (exponent > 0) base *= base;
                }
                return unsignedMode ? RuntimeValue(result) : RuntimeValue(static_cast<std::int64_t>(result));
            }

        default:
            throw std::runtime_error("Unknown operator");
        }
    }
    }

    throw std::runtime_error("Unknown expression");
}
