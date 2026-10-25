#include "interpreter.h"
#include "interpreter_internal.h"

#include <stdexcept>

using namespace rift::runtime;

RuntimeValue Interpreter::evaluate(const Expression& expression)
{
    switch (expression.type) {
    case Expression::Type::Number:
        return expression.value;
    case Expression::Type::String:
        return expression.value;
    case Expression::Type::Character:
        return expression.value;
    case Expression::Type::Boolean:
        return expression.value;
    case Expression::Type::Identifier: {
        auto it = variables_.find(expression.name);
        if (it == variables_.end()) throw std::runtime_error("Undefined variable: " + expression.name);
        return it->second;
    }
    case Expression::Type::Unary: {
        RuntimeValue value = evaluate(*expression.left);
        if (expression.operation == "!") return !requireBoolean(value, "Logical negation operand");
        if (!std::holds_alternative<WideInteger>(value)) throw std::runtime_error("Only integers can be negated");
        auto integer = std::get<WideInteger>(value);
        return negate(integer);
    }
    case Expression::Type::Binary: {
        RuntimeValue l = evaluate(*expression.left);
        if (expression.operation == "&&") {
            const bool leftBool = requireBoolean(l, "Logical AND operand");
            if (!leftBool) return false;
            return requireBoolean(evaluate(*expression.right), "Logical AND operand");
        }
        if (expression.operation == "||") {
            const bool leftBool = requireBoolean(l, "Logical OR operand");
            if (leftBool) return true;
            return requireBoolean(evaluate(*expression.right), "Logical OR operand");
        }
        RuntimeValue r = evaluate(*expression.right);
        if (expression.operation == "==" || expression.operation == "!=" || expression.operation == "<" ||
            expression.operation == "<=" || expression.operation == ">" || expression.operation == ">=") {
            return compareValues(l, r, expression.operation);
        }
        if (expression.operation == "+" &&
            (std::holds_alternative<std::string>(l) || std::holds_alternative<char>(l)) &&
            (std::holds_alternative<std::string>(r) || std::holds_alternative<char>(r))) {
            return stringify(l) + stringify(r);
        }
        if (!std::holds_alternative<WideInteger>(l) || !std::holds_alternative<WideInteger>(r)) throw std::runtime_error("These values cannot be used with arithmetic operators");
        WideInteger a = std::get<WideInteger>(l), b = std::get<WideInteger>(r);
        return applyArithmetic(expression.operation, a, b);
    }
    case Expression::Type::Conditional: {
        const bool condition = requireBoolean(evaluate(*expression.left), "Conditional expression condition");
        return evaluate(condition ? *expression.right : *expression.third);
    }
    case Expression::Type::Call: {
        std::vector<RuntimeValue> arguments;
        arguments.reserve(expression.arguments.size());
        for (const auto& argument : expression.arguments) arguments.push_back(evaluate(*argument));
        return callFunction(expression.name, arguments);
    }
    }
    throw std::runtime_error("Unknown expression");
}
