#include "interpreter.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
WideInteger addIntegers(WideInteger a, WideInteger b)
{
    WideInteger result;
    result.isUnsigned = a.isUnsigned || b.isUnsigned;
    if (result.isUnsigned && (a.negative || b.negative)) throw std::runtime_error("Cannot mix negative and unsigned integer values");
    if (a.negative == b.negative) {
        bool overflow = false;
        result.magnitude = UInt128::add(a.magnitude, b.magnitude, overflow);
        if (overflow) throw std::runtime_error("128-bit integer overflow");
        result.negative = a.negative;
    } else if (a.magnitude < b.magnitude) {
        result.magnitude = UInt128::subtract(b.magnitude, a.magnitude);
        result.negative = b.negative;
    } else {
        result.magnitude = UInt128::subtract(a.magnitude, b.magnitude);
        result.negative = a.negative;
    }
    if (result.isUnsigned && result.negative) throw std::runtime_error("Cannot produce a negative unsigned integer");
    return result;
}

WideInteger negate(WideInteger value)
{
    if (!(value.magnitude == UInt128{})) value.negative = !value.negative;
    return value;
}

WideInteger multiplyIntegers(WideInteger a, WideInteger b)
{
    WideInteger result;
    bool overflow = false;
    result.magnitude = UInt128::multiply(a.magnitude, b.magnitude, overflow);
    if (overflow) throw std::runtime_error("128-bit integer overflow");
    result.negative = a.negative != b.negative;
    result.isUnsigned = a.isUnsigned || b.isUnsigned;
    if (result.isUnsigned && result.negative) throw std::runtime_error("Cannot produce a negative unsigned integer");
    return result;
}

WideInteger integerPower(WideInteger base, WideInteger exponent)
{
    if (exponent.negative) throw std::runtime_error("Exponent must be non-negative for integer values");
    WideInteger result{UInt128{0, 1}, false, base.isUnsigned || exponent.isUnsigned};
    while (!(exponent.magnitude == UInt128{})) {
        if (exponent.magnitude.low & 1) result = multiplyIntegers(result, base);
        UInt128 remainder{};
        exponent.magnitude = UInt128::divide(exponent.magnitude, {0, 2}, remainder);
        if (!(exponent.magnitude == UInt128{})) base = multiplyIntegers(base, base);
    }
    return result;
}
}

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
    const auto variableCount = std::count_if(function.statements.begin(), function.statements.end(), [](const Statement& s) { return s.type == Statement::Type::VariableDeclaration; });
    variables_.reserve(variableCount);

    for (const auto& statement : function.statements) {
        if (statement.type == Statement::Type::VariableDeclaration) {
            const auto& declaration = *statement.variable;
            RuntimeValue value = evaluate(*declaration.value);
            if (declaration.type == "bool") {
                if (!std::holds_alternative<bool>(value)) throw std::runtime_error("Value for '" + declaration.name + "' must be bool");
            } else {
                if (std::holds_alternative<bool>(value)) throw std::runtime_error("Value for '" + declaration.name + "' must be an integer");
                auto integer = std::get<WideInteger>(value);
                const bool isUnsigned = declaration.type.starts_with("uint");
                if (integer.negative && isUnsigned) throw std::runtime_error("Unsigned integer cannot be negative");
                if (!integerFitsType(integer, declaration.type)) throw std::runtime_error("Value for '" + declaration.name + "' is outside the range of " + declaration.type);
                integer.isUnsigned = isUnsigned;
                value = integer;
            }
            variables_[declaration.name] = value;
        } else if (statement.type == Statement::Type::Print) {
            RuntimeValue value = evaluate(*statement.expression);
            if (const auto* boolean = std::get_if<bool>(&value)) std::cout << (*boolean ? "true" : "false") << '\n';
            else std::cout << std::get<WideInteger>(value).toString() << '\n';
        }
    }
}

RuntimeValue Interpreter::evaluate(const Expression& expression)
{
    switch (expression.type) {
    case Expression::Type::Number:
        return expression.value;
    case Expression::Type::Identifier: {
        auto it = variables_.find(expression.name);
        if (it == variables_.end()) throw std::runtime_error("Undefined variable: " + expression.name);
        return it->second;
    }
    case Expression::Type::Unary: {
        RuntimeValue value = evaluate(*expression.left);
        if (std::holds_alternative<bool>(value)) throw std::runtime_error("Boolean values cannot be negated");
        auto integer = std::get<WideInteger>(value);
        return negate(integer);
    }
    case Expression::Type::Binary: {
        RuntimeValue l = evaluate(*expression.left), r = evaluate(*expression.right);
        if (std::holds_alternative<bool>(l) || std::holds_alternative<bool>(r)) throw std::runtime_error("Boolean values cannot be used with arithmetic operators");
        WideInteger a = std::get<WideInteger>(l), b = std::get<WideInteger>(r);
        switch (expression.operation) {
        case '+': return addIntegers(a, b);
        case '-': return addIntegers(a, negate(b));
        case '*': return multiplyIntegers(a, b);
        case '/': {
            if (b.magnitude == UInt128{}) throw std::runtime_error("Division by zero");
            WideInteger result;
            UInt128 remainder{};
            result.magnitude = UInt128::divide(a.magnitude, b.magnitude, remainder);
            result.negative = a.negative != b.negative;
            result.isUnsigned = a.isUnsigned || b.isUnsigned;
            return result;
        }
        case '^': return integerPower(a, b);
        default: throw std::runtime_error("Unknown operator");
        }
    }
    }
    throw std::runtime_error("Unknown expression");
}
