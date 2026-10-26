#include "interpreter_internal.h"

#include <iomanip>
#include <limits>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace rift::runtime {
namespace {
unsigned effectiveWidth(const WideInteger& value)
{
    return value.bitWidth ? value.bitWidth : 32;
}

void checkIntegerWidth(const WideInteger& value)
{
    const std::string type = std::string(value.isUnsigned ? "uint" : "int") + std::to_string(effectiveWidth(value));
    if (!integerFitsType(value, type)) throw std::runtime_error("Integer overflow for " + type);
}
}

WideInteger addIntegers(WideInteger a, WideInteger b)
{
    WideInteger result;
    result.bitWidth = static_cast<std::uint8_t>(std::max(effectiveWidth(a), effectiveWidth(b)));
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
    checkIntegerWidth(result);
    return result;
}

WideInteger negate(WideInteger value)
{
    if (!(value.magnitude == UInt128{})) value.negative = !value.negative;
    if (value.bitWidth) checkIntegerWidth(value);
    return value;
}

WideInteger multiplyIntegers(WideInteger a, WideInteger b)
{
    WideInteger result;
    result.bitWidth = static_cast<std::uint8_t>(std::max(effectiveWidth(a), effectiveWidth(b)));
    bool overflow = false;
    result.magnitude = UInt128::multiply(a.magnitude, b.magnitude, overflow);
    if (overflow) throw std::runtime_error("128-bit integer overflow");
    result.negative = a.negative != b.negative;
    result.isUnsigned = a.isUnsigned || b.isUnsigned;
    if (result.isUnsigned && (a.negative || b.negative)) throw std::runtime_error("Cannot mix negative and unsigned integer values");
    checkIntegerWidth(result);
    return result;
}

WideInteger integerPower(WideInteger base, WideInteger exponent)
{
    if (exponent.negative) throw std::runtime_error("Exponent must be non-negative for integer values");
    WideInteger result{UInt128{0, 1}, false, base.isUnsigned, static_cast<std::uint8_t>(effectiveWidth(base))};
    checkIntegerWidth(result);
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
    if (operation == "-") {
        if ((a.isUnsigned || b.isUnsigned) && !a.negative && !b.negative) {
            if (a.magnitude < b.magnitude) throw std::runtime_error("Unsigned integer underflow");
            WideInteger result;
            result.magnitude = UInt128::subtract(a.magnitude, b.magnitude);
            result.bitWidth = static_cast<std::uint8_t>(std::max(effectiveWidth(a), effectiveWidth(b)));
            result.isUnsigned = a.isUnsigned || b.isUnsigned;
            checkIntegerWidth(result);
            return result;
        }
        if (a.isUnsigned || b.isUnsigned) throw std::runtime_error("Cannot mix negative and unsigned integer values");
        WideInteger result;
        result.bitWidth = static_cast<std::uint8_t>(std::max(effectiveWidth(a), effectiveWidth(b)));
        if (a.negative != b.negative) {
            bool overflow = false;
            result.magnitude = UInt128::add(a.magnitude, b.magnitude, overflow);
            if (overflow) throw std::runtime_error("128-bit integer overflow");
            result.negative = a.negative;
        } else if (a.magnitude < b.magnitude) {
            result.magnitude = UInt128::subtract(b.magnitude, a.magnitude);
            result.negative = !a.negative;
        } else {
            result.magnitude = UInt128::subtract(a.magnitude, b.magnitude);
            result.negative = a.negative;
        }
        if (result.magnitude == UInt128{}) result.negative = false;
        checkIntegerWidth(result);
        return result;
    }
    if (operation == "*") return multiplyIntegers(a, b);
    if (operation == "/") {
        if (b.magnitude == UInt128{}) throw std::runtime_error("Division by zero");
        WideInteger result;
        result.bitWidth = static_cast<std::uint8_t>(std::max(effectiveWidth(a), effectiveWidth(b)));
        UInt128 remainder{};
        result.magnitude = UInt128::divide(a.magnitude, b.magnitude, remainder);
        result.negative = a.negative != b.negative;
        result.isUnsigned = a.isUnsigned || b.isUnsigned;
        if (result.isUnsigned && (a.negative || b.negative)) throw std::runtime_error("Cannot mix negative and unsigned integer values");
        checkIntegerWidth(result);
        return result;
    }
    if (operation == "%") {
        if (b.magnitude == UInt128{}) throw std::runtime_error("Remainder by zero");
        WideInteger result;
        result.bitWidth = static_cast<std::uint8_t>(std::max(effectiveWidth(a), effectiveWidth(b)));
        UInt128 remainder{};
        UInt128::divide(a.magnitude, b.magnitude, remainder);
        result.magnitude = remainder;
        result.negative = a.negative;
        result.isUnsigned = a.isUnsigned || b.isUnsigned;
        if (result.isUnsigned && (a.negative || b.negative)) throw std::runtime_error("Cannot mix negative and unsigned integer values");
        checkIntegerWidth(result);
        return result;
    }
    if (operation == "^") return integerPower(a, b);
    throw std::runtime_error("Unknown operator");
}

RuntimeValue applyNumericArithmetic(const std::string& operation, const RuntimeValue& left, const RuntimeValue& right)
{
    const auto* leftInteger = std::get_if<WideInteger>(&left);
    const auto* rightInteger = std::get_if<WideInteger>(&right);
    const auto* leftFloat = std::get_if<float>(&left);
    const auto* rightFloat = std::get_if<float>(&right);
    const auto* leftDouble = std::get_if<double>(&left);
    const auto* rightDouble = std::get_if<double>(&right);
    const auto* leftDecimal = std::get_if<Decimal>(&left);
    const auto* rightDecimal = std::get_if<Decimal>(&right);
    if ((!leftInteger && !leftFloat && !leftDouble && !leftDecimal) || (!rightInteger && !rightFloat && !rightDouble && !rightDecimal))
        throw std::runtime_error("Arithmetic operators require numeric values");
    if (leftInteger && rightInteger) return applyArithmetic(operation, *leftInteger, *rightInteger);

    auto asDecimal = [](const RuntimeValue& value) {
        if (const auto* d = std::get_if<Decimal>(&value)) return *d;
        if (const auto* i = std::get_if<WideInteger>(&value)) return Decimal::parse(i->toString());
        if (const auto* f = std::get_if<float>(&value)) return Decimal::fromFloat(*f);
        return Decimal::fromDouble(std::get<double>(value));
    };
    if (leftDecimal || rightDecimal) return decimalArithmetic(operation, asDecimal(left), asDecimal(right));

    const bool useDouble = leftDouble || rightDouble;
    auto asDouble = [](const RuntimeValue& value) {
        if (const auto* d = std::get_if<double>(&value)) return *d;
        if (const auto* f = std::get_if<float>(&value)) return static_cast<double>(*f);
        if (const auto* i = std::get_if<WideInteger>(&value)) return std::stod(i->toString());
        throw std::runtime_error("Invalid floating-point operand");
    };
    if (!useDouble) {
        const float a = leftFloat ? *leftFloat : static_cast<float>(std::stold(leftInteger->toString()));
        const float b = rightFloat ? *rightFloat : static_cast<float>(std::stold(rightInteger->toString()));
        float result;
        if (operation == "+") result = a + b;
        else if (operation == "-") result = a - b;
        else if (operation == "*") result = a * b;
        else if (operation == "/") { if (b == 0) throw std::runtime_error("Division by zero"); result = a / b; }
        else if (operation == "%") { if (b == 0) throw std::runtime_error("Remainder by zero"); result = std::fmod(a, b); }
        else if (operation == "^") result = std::pow(a, b);
        else throw std::runtime_error("Unknown floating-point operator");
        if (!std::isfinite(result)) throw std::runtime_error("Floating-point overflow");
        return result;
    }
    const double a = asDouble(left), b = asDouble(right);
    double result;
    if (operation == "+") result = a + b;
    else if (operation == "-") result = a - b;
    else if (operation == "*") result = a * b;
    else if (operation == "/") { if (b == 0) throw std::runtime_error("Division by zero"); result = a / b; }
    else if (operation == "%") { if (b == 0) throw std::runtime_error("Remainder by zero"); result = std::fmod(a, b); }
    else if (operation == "^") result = std::pow(a, b);
    else throw std::runtime_error("Operator '" + operation + "' is not supported for floating-point values");
    if (!std::isfinite(result)) throw std::runtime_error("Floating-point overflow");
    return result;
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
    const auto isNumeric = [](const RuntimeValue& v) { return std::holds_alternative<WideInteger>(v) || std::holds_alternative<float>(v) || std::holds_alternative<double>(v) || std::holds_alternative<Decimal>(v); };
    if (isNumeric(left) && isNumeric(right) && (std::holds_alternative<Decimal>(left) || std::holds_alternative<Decimal>(right))) {
        auto asDecimal = [](const RuntimeValue& v) {
            if (const auto* d = std::get_if<Decimal>(&v)) return *d;
            if (const auto* i = std::get_if<WideInteger>(&v)) return Decimal::parse(i->toString());
            if (const auto* f = std::get_if<float>(&v)) return Decimal::fromFloat(*f);
            return Decimal::fromDouble(std::get<double>(v));
        };
        order = asDecimal(left).compare(asDecimal(right));
    } else if (isNumeric(left) && isNumeric(right) && (std::holds_alternative<float>(left) || std::holds_alternative<double>(left) || std::holds_alternative<float>(right) || std::holds_alternative<double>(right))) {
        auto asDouble = [](const RuntimeValue& v) {
            if (const auto* d = std::get_if<double>(&v)) return *d;
            if (const auto* f = std::get_if<float>(&v)) return static_cast<double>(*f);
            return std::stod(std::get<WideInteger>(v).toString());
        };
        const double a = asDouble(left), b = asDouble(right);
        order = a == b ? 0 : (a < b ? -1 : 1);
    } else if ((isNumeric(left) || isNumeric(right)) &&
               !(std::holds_alternative<WideInteger>(left) && std::holds_alternative<WideInteger>(right))) comparable = false;
    else if (const auto* a = std::get_if<WideInteger>(&left)) {
        if (const auto* b = std::get_if<WideInteger>(&right)) order = compareIntegers(*a, *b);
        else comparable = false;
    } else if (const auto* a = std::get_if<std::u16string>(&left)) {
        if (const auto* b = std::get_if<std::u16string>(&right)) order = a->compare(*b);
        else comparable = false;
    } else if (const auto* a = std::get_if<char16_t>(&left)) {
        if (const auto* b = std::get_if<char16_t>(&right)) order = *a == *b ? 0 : (*a < *b ? -1 : 1);
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
    const bool numeric = std::holds_alternative<WideInteger>(value) || std::holds_alternative<float>(value) ||
        std::holds_alternative<double>(value) || std::holds_alternative<Decimal>(value);
    if (type == "float" || type == "double" || type == "decimal") {
        if (!numeric) throw std::runtime_error("Value for '" + name + "' must be numeric");
        if (type == "float") {
            float converted;
            if (const auto* i = std::get_if<WideInteger>(&value)) converted = static_cast<float>(std::stold(i->toString()));
            else if (const auto* f = std::get_if<float>(&value)) converted = *f;
            else if (const auto* d = std::get_if<double>(&value)) converted = static_cast<float>(*d);
            else converted = static_cast<float>(std::get<Decimal>(value).toLongDouble());
            if (!std::isfinite(converted)) throw std::runtime_error("Value for '" + name + "' is outside the range of float");
            return converted;
        }
        if (type == "double") {
            double converted;
            if (const auto* i = std::get_if<WideInteger>(&value)) converted = std::stod(i->toString());
            else if (const auto* f = std::get_if<float>(&value)) converted = *f;
            else if (const auto* d = std::get_if<double>(&value)) converted = *d;
            else converted = static_cast<double>(std::get<Decimal>(value).toLongDouble());
            if (!std::isfinite(converted)) throw std::runtime_error("Value for '" + name + "' is outside the range of double");
            return converted;
        }
        if (const auto* d = std::get_if<Decimal>(&value)) return *d;
        if (const auto* i = std::get_if<WideInteger>(&value)) return Decimal::parse(i->toString());
        if (const auto* f = std::get_if<float>(&value)) return Decimal::fromFloat(*f);
        return Decimal::fromDouble(std::get<double>(value));
    }
    if (type == "string") {
        if (!std::holds_alternative<std::u16string>(value)) throw std::runtime_error("Value for '" + name + "' must be string");
        return value;
    }
    if (type == "char") {
        if (!std::holds_alternative<char16_t>(value)) throw std::runtime_error("Value for '" + name + "' must be char");
        return value;
    }
    if (type == "bool") {
        if (!std::holds_alternative<bool>(value)) throw std::runtime_error("Value for '" + name + "' must be bool");
        return value;
    }
    WideInteger integer;
    if (const auto* source = std::get_if<WideInteger>(&value)) {
        integer = *source;
    } else if (const auto* decimal = std::get_if<Decimal>(&value)) {
        integer.magnitude = decimal->coefficient;
        for (unsigned i = 0; i < decimal->scale; ++i) {
            UInt128 remainder{};
            integer.magnitude = UInt128::divide(integer.magnitude, {0, 10}, remainder);
        }
        integer.negative = decimal->negative && !(integer.magnitude == UInt128{});
    } else if (const auto* floating = std::get_if<float>(&value)) {
        const long double number = std::trunc(static_cast<long double>(*floating));
        const long double upper = std::ldexp(1.0L, type.starts_with("uint") ? 128 : 127);
        if (!std::isfinite(number) || (type.starts_with("uint") ? (number < 0 || number >= upper) : (number < -upper || number >= upper)))
            throw std::runtime_error("Numeric conversion is outside the 128-bit integer range");
        std::ostringstream text; text << std::fixed << std::setprecision(0) << number;
        const std::string digits = text.str();
        const bool negative = !digits.empty() && digits.front() == '-';
        integer.magnitude = UInt128::parse(negative ? digits.substr(1) : digits);
        integer.negative = negative && !(integer.magnitude == UInt128{});
    } else if (const auto* floating = std::get_if<double>(&value)) {
        const long double number = std::trunc(static_cast<long double>(*floating));
        const long double upper = std::ldexp(1.0L, type.starts_with("uint") ? 128 : 127);
        if (!std::isfinite(number) || (type.starts_with("uint") ? (number < 0 || number >= upper) : (number < -upper || number >= upper)))
            throw std::runtime_error("Numeric conversion is outside the 128-bit integer range");
        std::ostringstream text; text << std::fixed << std::setprecision(0) << number;
        const std::string digits = text.str();
        const bool negative = !digits.empty() && digits.front() == '-';
        integer.magnitude = UInt128::parse(negative ? digits.substr(1) : digits);
        integer.negative = negative && !(integer.magnitude == UInt128{});
    } else {
        throw std::runtime_error("Value for '" + name + "' must be numeric");
    }
    const bool isUnsigned = type.starts_with("uint");
    if (integer.negative && isUnsigned) throw std::runtime_error("Unsigned integer cannot be negative");
    if (!integerFitsType(integer, type)) throw std::runtime_error("Value for '" + name + "' is outside the range of " + type);
    integer.isUnsigned = isUnsigned;
    const std::string canonicalType = type == "int" ? "int32" : type;
    integer.bitWidth = static_cast<std::uint8_t>(std::stoi(canonicalType.substr(isUnsigned ? 4 : 3)));
    return integer;
}

std::string stringify(const RuntimeValue& value)
{
    if (const auto* integer = std::get_if<WideInteger>(&value)) return integer->toString();
    if (const auto* f = std::get_if<float>(&value)) { std::ostringstream output; output << std::setprecision(std::numeric_limits<float>::max_digits10) << *f; return output.str(); }
    if (const auto* d = std::get_if<double>(&value)) { std::ostringstream output; output << std::setprecision(std::numeric_limits<double>::max_digits10) << *d; return output.str(); }
    if (const auto* decimal = std::get_if<Decimal>(&value)) return decimal->toString();
    if (const auto* boolean = std::get_if<bool>(&value)) return *boolean ? "true" : "false";
    if (const auto* typeName = std::get_if<std::string>(&value)) return *typeName;
    if (const auto* string = std::get_if<std::u16string>(&value)) return utf16ToUtf8(*string);
    if (const auto* character = std::get_if<char16_t>(&value)) return utf16ToUtf8(std::u16string(1, *character));
    throw std::runtime_error("Void function has no printable value");
}

rift::fmt::Argument formatArgument(const RuntimeValue& value)
{
    if (const auto* integer = std::get_if<WideInteger>(&value)) return {integer->toString(), true, *integer};
    return {stringify(value), false, {}};
}
}
