#include "interpreter_internal.h"

#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace rift::runtime {
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

WideInteger applyArithmetic(const std::string& operation, WideInteger a, WideInteger b)
{
    if (operation == "+") return addIntegers(a, b);
    if (operation == "-") return addIntegers(a, negate(b));
    if (operation == "*") return multiplyIntegers(a, b);
    if (operation == "/") {
        if (b.magnitude == UInt128{}) throw std::runtime_error("Division by zero");
        WideInteger result;
        UInt128 remainder{};
        result.magnitude = UInt128::divide(a.magnitude, b.magnitude, remainder);
        result.negative = a.negative != b.negative;
        result.isUnsigned = a.isUnsigned || b.isUnsigned;
        return result;
    }
    if (operation == "%") {
        if (b.magnitude == UInt128{}) throw std::runtime_error("Remainder by zero");
        WideInteger result;
        UInt128 remainder{};
        UInt128::divide(a.magnitude, b.magnitude, remainder);
        result.magnitude = remainder;
        result.negative = a.negative;
        result.isUnsigned = a.isUnsigned || b.isUnsigned;
        return result;
    }
    if (operation == "^") return integerPower(a, b);
    throw std::runtime_error("Unknown operator");
}

int compareIntegers(const WideInteger& a, const WideInteger& b)
{
    if (a.negative != b.negative) return a.negative ? -1 : 1;
    if (a.magnitude == b.magnitude) return 0;
    const int magnitudeOrder = a.magnitude < b.magnitude ? -1 : 1;
    return a.negative ? -magnitudeOrder : magnitudeOrder;
}

bool compareValues(const RuntimeValue& left, const RuntimeValue& right, const std::string& operation)
{
    int order = 0;
    bool comparable = true;
    if (const auto* a = std::get_if<long double>(&left)) {
        if (const auto* b = std::get_if<long double>(&right)) order = *a == *b ? 0 : (*a < *b ? -1 : 1);
        else if (const auto* bInt = std::get_if<WideInteger>(&right)) {
            const long double bValue = std::stold(bInt->toString());
            order = *a == bValue ? 0 : (*a < bValue ? -1 : 1);
        }
        else comparable = false;
    } else if (const auto* a = std::get_if<WideInteger>(&left)) {
        if (const auto* b = std::get_if<WideInteger>(&right)) order = compareIntegers(*a, *b);
        else if (const auto* bFloat = std::get_if<long double>(&right)) {
            const long double aValue = std::stold(a->toString());
            order = aValue == *bFloat ? 0 : (aValue < *bFloat ? -1 : 1);
        }
        else comparable = false;
    } else if (const auto* a = std::get_if<std::string>(&left)) {
        if (const auto* b = std::get_if<std::string>(&right)) order = a->compare(*b);
        else comparable = false;
    } else if (const auto* a = std::get_if<char>(&left)) {
        if (const auto* b = std::get_if<char>(&right)) order = *a == *b ? 0 : (*a < *b ? -1 : 1);
        else comparable = false;
    } else if (const auto* a = std::get_if<bool>(&left)) {
        if (const auto* b = std::get_if<bool>(&right)) order = *a == *b ? 0 : (*a ? 1 : -1);
        else comparable = false;
    }
    if (operation == "==") return comparable && order == 0;
    if (operation == "!=") return !comparable || order != 0;
    if (!comparable || std::holds_alternative<bool>(left)) throw std::runtime_error("Values do not support ordered comparison");
    if (operation == "<") return order < 0;
    if (operation == "<=") return order <= 0;
    if (operation == ">") return order > 0;
    if (operation == ">=") return order >= 0;
    throw std::runtime_error("Unknown comparison operator");
}

bool requireBoolean(const RuntimeValue& value, const std::string& context)
{
    if (const auto* boolean = std::get_if<bool>(&value)) return *boolean;
    throw std::runtime_error(context + " must be bool");
}

RuntimeValue validateValue(RuntimeValue value, const std::string& type, const std::string& name)
{
    if (type == "any") return value;
    if (type == "float" || type == "double" || type == "decimal") {
        if (std::holds_alternative<WideInteger>(value)) value = static_cast<long double>(std::stold(std::get<WideInteger>(value).toString()));
        if (!std::holds_alternative<long double>(value)) throw std::runtime_error("Value for '" + name + "' must be numeric");
        return value;
    }
    if (type == "string") {
        if (!std::holds_alternative<std::string>(value)) throw std::runtime_error("Value for '" + name + "' must be string");
        return value;
    }
    if (type == "char") {
        if (!std::holds_alternative<char>(value)) throw std::runtime_error("Value for '" + name + "' must be char");
        return value;
    }
    if (type == "bool") {
        if (!std::holds_alternative<bool>(value)) throw std::runtime_error("Value for '" + name + "' must be bool");
        return value;
    }
    if (!std::holds_alternative<WideInteger>(value)) throw std::runtime_error("Value for '" + name + "' must be an integer");
    auto integer = std::get<WideInteger>(value);
    const bool isUnsigned = type.starts_with("uint");
    if (integer.negative && isUnsigned) throw std::runtime_error("Unsigned integer cannot be negative");
    if (!integerFitsType(integer, type)) throw std::runtime_error("Value for '" + name + "' is outside the range of " + type);
    integer.isUnsigned = isUnsigned;
    return integer;
}

std::string stringify(const RuntimeValue& value)
{
    if (const auto* integer = std::get_if<WideInteger>(&value)) return integer->toString();
    if (const auto* decimal = std::get_if<long double>(&value)) { std::ostringstream output; output << std::setprecision(std::numeric_limits<long double>::digits10 + 1) << *decimal; return output.str(); }
    if (const auto* boolean = std::get_if<bool>(&value)) return *boolean ? "true" : "false";
    if (const auto* string = std::get_if<std::string>(&value)) return *string;
    if (const auto* character = std::get_if<char>(&value)) return std::string(1, *character);
    throw std::runtime_error("Void function has no printable value");
}

rift::fmt::Argument formatArgument(const RuntimeValue& value)
{
    if (const auto* integer = std::get_if<WideInteger>(&value)) return {integer->toString(), true, *integer};
    return {stringify(value), false, {}};
}
}
