#include "interpreter.h"
#include "interpreter_internal.h"

#include <cmath>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

using namespace rift::runtime;

namespace {
WideInteger integerFromBits(unsigned bits, bool isUnsigned, bool negative = false, bool maximum = true)
{
    WideInteger value;
    value.isUnsigned = isUnsigned;
    value.bitWidth = static_cast<std::uint8_t>(bits);
    if (isUnsigned && !maximum) return value;
    value.negative = negative && !isUnsigned;
    if (bits == 128) {
        if (isUnsigned) value.magnitude = {UINT64_MAX, UINT64_MAX};
        else value.magnitude = negative ? UInt128{UINT64_C(1) << 63, 0} : UInt128{(UINT64_C(1) << 63) - 1, UINT64_MAX};
    } else if (bits == 64) {
        if (isUnsigned) value.magnitude.low = UINT64_MAX;
        else value.magnitude.low = negative ? (UINT64_C(1) << 63) : (UINT64_C(1) << 63) - 1;
    } else {
        const unsigned magnitudeBits = isUnsigned ? bits : bits - 1;
        value.magnitude.low = (UINT64_C(1) << magnitudeBits) - (maximum ? 1 : 0);
    }
    return value;
}

std::string valueType(const RuntimeValue& value)
{
    if (const auto* integer = std::get_if<WideInteger>(&value)) {
        const unsigned bits = integer->bitWidth ? integer->bitWidth : 32;
        return std::string(integer->isUnsigned ? "uint" : "int") + std::to_string(bits);
    }
    if (std::holds_alternative<float>(value)) return "float";
    if (std::holds_alternative<double>(value)) return "double";
    if (std::holds_alternative<Decimal>(value)) return "decimal";
    if (std::holds_alternative<bool>(value)) return "bool";
    if (std::holds_alternative<std::u16string>(value)) return "string";
    if (std::holds_alternative<char16_t>(value)) return "char";
    return "void";
}

std::uint64_t loopCount(const RuntimeValue& value, const std::string& purpose)
{
    const auto* integer = std::get_if<WideInteger>(&value);
    if (!integer || integer->negative || integer->magnitude.high != 0)
        throw std::runtime_error(purpose + " must be a non-negative integer no larger than uint64");
    return integer->magnitude.low;
}

std::size_t typeSize(const std::string& type)
{
    if (type == "bool" || type == "int8" || type == "uint8") return 1;
    if (type == "char") return 2;
    if (type == "int16" || type == "uint16") return 2;
    if (type == "int" || type == "int32" || type == "uint32" || type == "float") return 4;
    if (type == "int64" || type == "uint64" || type == "double") return 8;
    if (type == "int128" || type == "uint128" || type == "decimal") return 16;
    if (type == "string") return 8; // logical string handle; payload is separately allocated
    throw std::runtime_error("std.types.sizeof does not support type '" + type + "'");
}

RuntimeValue typeBuiltin(const std::string& name, const std::vector<RuntimeValue>& args)
{
    const auto requireCount = [&args, &name](std::size_t count) {
        if (args.size() != count) throw std::runtime_error("std.types." + name + " expects " + std::to_string(count) + " argument(s)");
    };
    const auto requireType = [&args]() -> std::string {
        if (!std::holds_alternative<std::string>(args[0])) throw std::runtime_error("Expected a type name or string");
        return std::get<std::string>(args[0]);
    };
    if (name == "to_string") { requireCount(1); return utf8ToUtf16(stringify(args[0])); }
    if (name == "type_of" || name == "typeof") { requireCount(1); return valueType(args[0]); }
    if (name == "is_integer") { requireCount(1); return std::holds_alternative<WideInteger>(args[0]); }
    if (name == "is_numeric") { requireCount(1); return std::holds_alternative<WideInteger>(args[0]) || std::holds_alternative<float>(args[0]) || std::holds_alternative<double>(args[0]) || std::holds_alternative<Decimal>(args[0]); }
    if (name == "is_bool") { requireCount(1); return std::holds_alternative<bool>(args[0]); }
    if (name == "size_of" || name == "sizeof") {
        requireCount(1);
        const std::string type = std::holds_alternative<std::string>(args[0]) ? requireType() : valueType(args[0]);
        return WideInteger{UInt128{0, typeSize(type)}, false, true};
    }
    if (name == "max_value" || name == "min_value") {
        requireCount(1);
        const std::string type = std::holds_alternative<std::string>(args[0]) ? requireType() : valueType(args[0]);
        if (type == "bool") return name == "max_value";
        if (type == "char") return WideInteger{UInt128{0, name == "max_value" ? 0xffffu : 0u}, false, true};
        if (type == "string") return WideInteger{UInt128{0, name == "max_value" ? UINT32_MAX : 0u}, false, true};
        if (type == "float") return name == "max_value" ? std::numeric_limits<float>::max() : -std::numeric_limits<float>::max();
        if (type == "double") return name == "max_value" ? std::numeric_limits<double>::max() : -std::numeric_limits<double>::max();
        if (type == "decimal") return Decimal{{name == "max_value" ? (UINT64_C(1) << 63) - 1 : UINT64_C(1) << 63, name == "max_value" ? UINT64_MAX : 0}, name == "min_value", 0};
        bool isUnsigned = type.starts_with("uint");
        if (type == "int") return integerFromBits(32, false, name == "min_value", name == "max_value");
        if (type != "uint8" && type != "uint16" && type != "uint32" && type != "uint64" && type != "uint128" &&
            type != "int8" && type != "int16" && type != "int32" && type != "int64" && type != "int128")
            throw std::runtime_error("Unknown numeric type: " + type);
        const unsigned bits = static_cast<unsigned>(std::stoi(type.substr(isUnsigned ? 4 : 3)));
        return integerFromBits(bits, isUnsigned, name == "min_value", name == "max_value");
    }
    if (name.starts_with("to_uint") || name.starts_with("to_int")) {
        requireCount(1);
        const std::string type = name == "to_int" ? "int" : name == "to_uint" ? "uint32" : name.substr(3);
        return validateValue(args[0], type, "conversion");
    }
    if (name == "to_float" || name == "to_double" || name == "to_decimal") {
        requireCount(1);
        const std::string type = name.substr(3);
        return validateValue(args[0], type, "conversion");
    }
    throw std::runtime_error("Unknown std.types function: " + name);
}
}

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
    case Expression::Type::TypeName:
        return expression.name;
    case Expression::Type::Identifier: {
        const auto dot = expression.name.rfind('.');
        if (dot != std::string::npos) {
            const std::string target = expression.name.substr(0, dot);
            const std::string member = expression.name.substr(dot + 1);
            auto state = loopStates_.rbegin();
            if (target == "loop") {
                if (state == loopStates_.rend()) throw std::runtime_error("No active loop for 'loop." + member + "'");
            } else if (member == "iteration" || member == "limit" || member == "break" || member == "continue" || member == "restart") {
                while (state != loopStates_.rend() && state->name != target) ++state;
                if (state == loopStates_.rend()) throw std::runtime_error("No active loop named '" + target + "'");
            }
            if (member == "iteration") return WideInteger{UInt128{0, state->iterations}, false, true, 64};
            if (member == "limit") {
                if (!state->limit) throw std::runtime_error("Loop has no iteration limit");
                return WideInteger{UInt128{0, *state->limit}, false, true, 64};
            }
            if (member == "break") throw LoopControlSignal{target, LoopAction::Break};
            if (member == "continue") throw LoopControlSignal{target, LoopAction::Continue};
            if (member == "restart") throw LoopControlSignal{target, LoopAction::Restart};
        }
        for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
            auto it = scope->find(expression.name);
            if (it != scope->end()) {
                if (std::holds_alternative<std::monostate>(it->second.value)) throw std::runtime_error("Variable used before initialization: " + expression.name);
                return it->second.value;
            }
        }
        throw std::runtime_error("Undefined variable: " + expression.name);
    }
    case Expression::Type::Unary: {
        RuntimeValue value = evaluate(*expression.left);
        if (expression.operation == "!") return !requireBoolean(value, "Logical negation operand");
        if (std::holds_alternative<float>(value)) return -std::get<float>(value);
        if (std::holds_alternative<double>(value)) return -std::get<double>(value);
        if (std::holds_alternative<Decimal>(value)) { auto decimal = std::get<Decimal>(value); if (!(decimal.coefficient == UInt128{})) decimal.negative = !decimal.negative; return decimal; }
        if (!std::holds_alternative<WideInteger>(value)) throw std::runtime_error("Only numeric values can be negated");
        auto integer = std::get<WideInteger>(value);
        if (integer.isUnsigned) throw std::runtime_error("Unsigned integer values cannot be negated");
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
            (std::holds_alternative<std::u16string>(l) || std::holds_alternative<char16_t>(l)) &&
            (std::holds_alternative<std::u16string>(r) || std::holds_alternative<char16_t>(r))) {
            const auto toUtf16 = [](const RuntimeValue& value) {
                if (const auto* string = std::get_if<std::u16string>(&value)) return *string;
                return std::u16string(1, std::get<char16_t>(value));
            };
            return toUtf16(l) + toUtf16(r);
        }
        return applyNumericArithmetic(expression.operation, l, r);
    }
    case Expression::Type::Conditional: {
        const bool condition = requireBoolean(evaluate(*expression.left), "Conditional expression condition");
        RuntimeValue result = evaluate(condition ? *expression.right : *expression.third);
        const std::string& type = expression.resolvedType;
        if (type == "float" || type == "double" || type == "decimal" || type == "int" || type.starts_with("int") || type.starts_with("uint"))
            return validateValue(std::move(result), type, "conditional expression");
        return result;
    }
    case Expression::Type::Call: {
        if (expression.name.ends_with(".skip") || expression.name.ends_with(".break_if")) {
            const bool isSkip = expression.name.ends_with(".skip");
            const std::string target = expression.name.substr(0, expression.name.rfind('.'));
            if (expression.arguments.size() != 1) throw std::runtime_error("Loop control expects exactly one argument");
            RuntimeValue argument = evaluate(*expression.arguments.front());
            auto state = loopStates_.rbegin();
            if (target == "loop") {
                if (state == loopStates_.rend()) throw std::runtime_error("No active loop for 'loop' control");
            } else {
                while (state != loopStates_.rend() && state->name != target) ++state;
                if (state == loopStates_.rend()) throw std::runtime_error("No active loop named '" + target + "'");
            }
            if (isSkip) {
                const std::uint64_t count = loopCount(argument, "loop.skip count");
                if (count > std::numeric_limits<std::uint64_t>::max() - state->skipsRemaining) throw std::runtime_error("loop.skip count overflow");
                state->skipsRemaining += count;
                return std::monostate{};
            }
            if (requireBoolean(argument, "loop.break_if argument")) throw LoopControlSignal{target, LoopAction::Break};
            return std::monostate{};
        }
        std::string builtinName;
        const std::string explicitPrefix = "std.types.";
            if (expression.name.starts_with(explicitPrefix)) {
            builtinName = expression.name.substr(explicitPrefix.size());
        } else if (expression.name.starts_with("types.") &&
                   std::find(usingNamespaces_.begin(), usingNamespaces_.end(), "std") != usingNamespaces_.end()) {
            builtinName = expression.name.substr(6);
        } else if (expression.name.find('.') == std::string::npos &&
                   std::find(usingNamespaces_.begin(), usingNamespaces_.end(), "std.types") != usingNamespaces_.end()) {
            static const std::vector<std::string> typeFunctions = {
                "size_of", "sizeof", "max_value", "min_value", "to_string", "type_of", "typeof", "is_integer", "is_numeric", "is_bool",
                "to_int8", "to_int16", "to_int32", "to_int64", "to_int128", "to_uint8", "to_uint16", "to_uint32", "to_uint64", "to_uint128",
                "to_int", "to_uint", "to_float", "to_double", "to_decimal"
            };
            if (std::find(typeFunctions.begin(), typeFunctions.end(), expression.name) != typeFunctions.end()) builtinName = expression.name;
        }
        if (!builtinName.empty()) {
            if ((builtinName == "type_of" || builtinName == "typeof") && expression.arguments.size() == 1 &&
                expression.arguments[0]->type == Expression::Type::TypeName)
                return expression.arguments[0]->name;
            std::vector<RuntimeValue> arguments;
            arguments.reserve(expression.arguments.size());
            const bool inspectVariableType = (builtinName == "size_of" || builtinName == "sizeof" || builtinName == "type_of" || builtinName == "typeof") &&
                expression.arguments.size() == 1 &&
                expression.arguments[0]->type == Expression::Type::Identifier;
            if (inspectVariableType) {
                const auto& variableName = expression.arguments[0]->name;
                bool foundVariable = false;
                for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
                    auto variable = scope->find(variableName);
                    if (variable != scope->end()) {
                        if (std::holds_alternative<std::monostate>(variable->second.value))
                            throw std::runtime_error("Variable used before initialization: " + variableName);
                        if (variable->second.type == "any") arguments.push_back(variable->second.value);
                        else arguments.emplace_back(variable->second.type);
                        foundVariable = true;
                        break;
                    }
                }
                if (!foundVariable) arguments.push_back(evaluate(*expression.arguments[0]));
            } else {
                for (const auto& argument : expression.arguments) arguments.push_back(evaluate(*argument));
            }
            if ((builtinName == "type_of" || builtinName == "typeof") && inspectVariableType && !arguments.empty() &&
                std::holds_alternative<std::string>(arguments[0])) return arguments[0];
            RuntimeValue result = typeBuiltin(builtinName, arguments);
            return result;
        }
        std::vector<RuntimeValue> arguments;
        arguments.reserve(expression.arguments.size());
        for (const auto& argument : expression.arguments) arguments.push_back(evaluate(*argument));
        return callFunction(expression.name, arguments, expression.resolvedFunction);
    }
    }
    throw std::runtime_error("Unknown expression");
}
