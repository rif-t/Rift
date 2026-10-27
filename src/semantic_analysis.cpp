#include "semantic_analysis.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace {
struct TypeInfo { std::string type; bool isConst = false; };
using Types = std::vector<std::unordered_map<std::string, TypeInfo>>;
using Functions = std::unordered_map<std::string, std::vector<const Function*>>;

std::string canonicalType(std::string type)
{
    if (type == "int32") return "int";
    if (type == "uint") return "uint32";
    return type;
}

bool numeric(const std::string& t) { return t == "any" || t == "int" || t == "float" || t == "double" || t == "decimal" || t.starts_with("int") || t.starts_with("uint"); }
bool assignable(const std::string& target, const std::string& source)
{
    if (target == "any" || target == source || source == "any") return true;
    if (numeric(target) && numeric(source)) return true; // Runtime range checks enforce narrowing conversions.
    return false;
}

std::string promotedNumericType(const std::string& left, const std::string& right, const std::string& operation = "")
{
    if (left == "any" || right == "any") return "any";
    if (left == "decimal" || right == "decimal") return "decimal";
    if (left == "double" || right == "double") return "double";
    if (left == "float" || right == "float") return "float";
    if (operation == "^") return left;
    const bool unsignedResult = left.starts_with("uint") || right.starts_with("uint");
    const unsigned leftBits = left == "int" ? 32 : static_cast<unsigned>(std::stoi(left.substr(left.starts_with("uint") ? 4 : 3)));
    const unsigned rightBits = right == "int" ? 32 : static_cast<unsigned>(std::stoi(right.substr(right.starts_with("uint") ? 4 : 3)));
    return std::string(unsignedResult ? "uint" : "int") + std::to_string(std::max(leftBits, rightBits));
}

std::vector<const Function*> resolve(const std::string& name, const Functions& functions, const Program& program)
{
    if (auto it = functions.find(name); it != functions.end()) return it->second;
    if (name.find('.') == std::string::npos) {
        std::vector<const Function*> result;
        for (const auto& ns : program.usingNamespaces) {
            if (ns == "std") continue;
            if (auto it = functions.find(ns + "." + name); it != functions.end()) {
                if (!result.empty()) throw std::runtime_error("Ambiguous function '" + name + "'; qualify it with its namespace");
                result = it->second;
            }
        }
        return result;
    }
    return {};
}

const Function* selectOverload(const std::string& name, const std::vector<const Function*>& candidates,
    const std::vector<std::string>& argumentTypes)
{
    int bestScore = std::numeric_limits<int>::max();
    const Function* best = nullptr;
    bool tied = false;
    bool matchingArity = false;
    for (const Function* candidate : candidates) {
        if (candidate->parameters.size() != argumentTypes.size()) continue;
        matchingArity = true;
        int score = 0;
        bool matches = true;
        for (std::size_t i = 0; i < argumentTypes.size(); ++i) {
            const auto target = canonicalType(candidate->parameters[i].second);
            const auto source = canonicalType(argumentTypes[i]);
            if (target == source) continue;
            if (target == "any") { score += 2; continue; }
            if (source == "any") { score += 1; continue; }
            if (numeric(target) && numeric(source)) { score += 1; continue; }
            matches = false;
            break;
        }
        if (!matches) continue;
        if (score < bestScore) { bestScore = score; best = candidate; tied = false; }
        else if (score == bestScore) tied = true;
    }
    if (tied) throw std::runtime_error("Ambiguous overload for function '" + name + "'");
    if (!best) {
        if (!matchingArity) throw std::runtime_error("No overload of function '" + name + "' accepts " + std::to_string(argumentTypes.size()) + " argument(s)");
        if (candidates.size() == 1) {
            const Function& candidate = *candidates.front();
            for (std::size_t i = 0; i < argumentTypes.size(); ++i) {
                if (!assignable(candidate.parameters[i].second, argumentTypes[i]))
                    throw std::runtime_error("Argument " + std::to_string(i + 1) + " of function '" + name + "' must be " + candidate.parameters[i].second + ", got " + argumentTypes[i]);
            }
        }
        throw std::runtime_error("No matching overload for function '" + name + "'");
    }
    return best;
}

std::string expressionType(const Expression& e, Types& scopes, const Functions& functions, const Program& program);

std::string resolveTypeBuiltin(const std::string& name, const Program& program)
{
    static const std::unordered_set<std::string> builtins = {
        "size_of", "sizeof", "max_value", "min_value", "to_string", "type_of", "typeof",
        "is_integer", "is_numeric", "is_bool", "to_int", "to_uint", "to_float", "to_double", "to_decimal",
        "to_int8", "to_int16", "to_int32", "to_int64", "to_int128",
        "to_uint8", "to_uint16", "to_uint32", "to_uint64", "to_uint128"
    };
    constexpr const char* explicitPrefix = "std.types.";
    constexpr const char* stdPrefix = "types.";
    if (name.starts_with(explicitPrefix)) return name.substr(std::char_traits<char>::length(explicitPrefix));
    if (name.starts_with(stdPrefix) &&
        std::find(program.usingNamespaces.begin(), program.usingNamespaces.end(), "std") != program.usingNamespaces.end())
        return name.substr(std::char_traits<char>::length(stdPrefix));
    if (name.find('.') == std::string::npos &&
        std::find(program.usingNamespaces.begin(), program.usingNamespaces.end(), "std.types") != program.usingNamespaces.end() && builtins.contains(name))
        return name;
    return {};
}

bool isConcreteStdType(const std::string& type)
{
    static const std::unordered_set<std::string> types = {
        "bool", "char", "string", "int", "int8", "int16", "int32", "int64", "int128",
        "uint8", "uint16", "uint32", "uint64", "uint128", "float", "double", "decimal"
    };
    return types.contains(type);
}

std::string validateTypeBuiltin(const std::string& name, const std::vector<std::string>& args)
{
    static const std::unordered_set<std::string> unaryValueFunctions = {
        "to_string", "type_of", "typeof", "is_integer", "is_numeric", "is_bool"
    };
    static const std::unordered_set<std::string> conversions = {
        "to_int", "to_uint", "to_float", "to_double", "to_decimal",
        "to_int8", "to_int16", "to_int32", "to_int64", "to_int128",
        "to_uint8", "to_uint16", "to_uint32", "to_uint64", "to_uint128"
    };
    const auto requireOne = [&]() {
        if (args.size() != 1) throw std::runtime_error("std.types." + name + " expects 1 argument(s)");
    };
    if (unaryValueFunctions.contains(name)) {
        requireOne();
        if (name == "to_string" && args[0] == "void")
            throw std::runtime_error("std.types.to_string cannot convert a void value");
        if (name == "to_string") return "string";
        if (name == "type_of" || name == "typeof") return "any";
        return "bool";
    }
    if (name == "size_of" || name == "sizeof" || name == "max_value" || name == "min_value") {
        requireOne();
        const auto& argument = args[0];
        if (argument == "void") throw std::runtime_error("std.types." + name + " cannot inspect a void value");
        if (argument == "type") return "int";
        if (argument == "any") return name == "size_of" || name == "sizeof" ? "int" : "any";
        if (name == "size_of" || name == "sizeof") return "int";
        return argument == "char" || argument == "string" ? "int" : argument;
    }
    if (conversions.contains(name)) {
        requireOne();
        if (!numeric(args[0])) throw std::runtime_error("std.types." + name + " expects a numeric argument, got " + args[0]);
        if (name == "to_int") return "int";
        if (name == "to_uint") return "uint32";
        return name.substr(3);
    }
    throw std::runtime_error("Unknown std.types function: " + name);
}

std::string inferExpressionType(const Expression& e, Types& scopes, const Functions& functions, const Program& program)
{
    switch (e.type) {
    case Expression::Type::Number:
        if (std::holds_alternative<Decimal>(e.value)) return "decimal";
        if (std::holds_alternative<float>(e.value)) return "float";
        return std::holds_alternative<double>(e.value) ? "double" : "int";
    case Expression::Type::String: return "string";
    case Expression::Type::Character: return "char";
    case Expression::Type::Boolean: return "bool";
    case Expression::Type::TypeName: return "type";
    case Expression::Type::Identifier:
        for (auto s = scopes.rbegin(); s != scopes.rend(); ++s)
            if (auto it = s->find(e.name); it != s->end()) return it->second.type;
        throw std::runtime_error("Undefined variable: " + e.name);
    case Expression::Type::Unary: {
        const auto type = expressionType(*e.left, scopes, functions, program);
        if (e.operation == "!") { if (type != "bool" && type != "any") throw std::runtime_error("Logical negation requires bool"); return "bool"; }
        if (e.operation == "-" && type.starts_with("uint")) throw std::runtime_error("Unsigned integer values cannot be negated");
        if (type != "int" && type != "float" && type != "double" && type != "decimal" && !type.starts_with("uint") && !type.starts_with("int"))
            throw std::runtime_error("Unary numeric operator requires a number");
        return type;
    }
    case Expression::Type::Binary: {
        auto l = expressionType(*e.left, scopes, functions, program);
        auto r = expressionType(*e.right, scopes, functions, program);
        if (e.operation == "&&" || e.operation == "||") {
            if ((l != "bool" && l != "any") || (r != "bool" && r != "any")) throw std::runtime_error("Logical operators require bool operands");
            return "bool";
        }
        if (e.operation == "==" || e.operation == "!=" || e.operation == "<" || e.operation == "<=" || e.operation == ">" || e.operation == ">=") {
            const bool bothNumeric = numeric(l) && numeric(r);
            const bool bothAny = l == "any" || r == "any";
            const bool sameComparable = l == r && (l == "string" || l == "char" || l == "bool");
            if (!bothNumeric && !bothAny && !sameComparable)
                throw std::runtime_error("Comparison operands must have compatible types, got " + l + " and " + r);
            if (l == "bool" && (e.operation != "==" && e.operation != "!="))
                throw std::runtime_error("Boolean values support equality comparisons only");
            return "bool";
        }
        if (e.operation == "+" && (l == "string" || l == "char") && (r == "string" || r == "char")) return "string";
        const bool ln = numeric(l);
        const bool rn = numeric(r);
        if (!ln || !rn) throw std::runtime_error("Arithmetic operators require numeric operands");
        return promotedNumericType(l, r, e.operation);
    }
    case Expression::Type::Conditional: {
        const auto conditionType = expressionType(*e.left, scopes, functions, program);
        if (conditionType != "bool" && conditionType != "any") throw std::runtime_error("Conditional expression condition must be bool");
        auto a = expressionType(*e.right, scopes, functions, program);
        auto b = expressionType(*e.third, scopes, functions, program);
        if (a == b) return a;
        if (numeric(a) && numeric(b)) return promotedNumericType(a, b);
        throw std::runtime_error("Conditional branches must have compatible types");
    }
    case Expression::Type::Call: {
        std::vector<std::string> argumentTypes;
        for (const auto& arg : e.arguments) argumentTypes.push_back(expressionType(*arg, scopes, functions, program));
        const std::string builtin = resolveTypeBuiltin(e.name, program);
        if (!builtin.empty()) {
            if (builtin == "size_of" || builtin == "sizeof" || builtin == "max_value" || builtin == "min_value") {
                for (std::size_t i = 0; i < e.arguments.size(); ++i) {
                    if (e.arguments[i]->type == Expression::Type::TypeName && !isConcreteStdType(e.arguments[i]->name))
                        throw std::runtime_error("std.types." + builtin + " does not support type '" + e.arguments[i]->name + "'");
                }
            }
            const std::string result = validateTypeBuiltin(builtin, argumentTypes);
            if ((builtin == "max_value" || builtin == "min_value") && !e.arguments.empty() &&
                e.arguments[0]->type == Expression::Type::TypeName)
                return e.arguments[0]->name == "char" || e.arguments[0]->name == "string" ? "int" : e.arguments[0]->name;
            return result;
        }
        const auto candidates = resolve(e.name, functions, program);
        if (candidates.empty()) throw std::runtime_error("Undefined function: " + e.name);
        const Function* f = selectOverload(e.name, candidates, argumentTypes);
        e.resolvedFunction = f;
        return f->returnType.empty() ? "void" : f->returnType;
    }
    }
    return "any";
}

std::string expressionType(const Expression& e, Types& scopes, const Functions& functions, const Program& program)
{
    e.resolvedType = inferExpressionType(e, scopes, functions, program);
    return e.resolvedType;
}

bool returnsOnAllPaths(const std::vector<Statement>& statements)
{
    for (const auto& s : statements) {
        if (s.type == Statement::Type::Return) return true;
        if (s.type == Statement::Type::If && !s.elseStatements.empty()) {
            bool all = returnsOnAllPaths(s.elseStatements);
            for (const auto& branch : s.branches) all = all && returnsOnAllPaths(branch.statements);
            if (all) return true;
        }
    }
    return false;
}

void checkStatements(const std::vector<Statement>& statements, Types& scopes, const Functions& functions, const Program& program, const std::string& returnType, std::vector<std::string>& loopStack)
{
    for (const auto& s : statements) {
        if (s.type == Statement::Type::VariableDeclaration) {
            const auto& d = *s.variable;
            for (std::size_t i = 0; i < d.names.size(); ++i) {
                const auto& name = d.names[i];
                if (scopes.back().contains(name)) throw std::runtime_error("Variable already declared in this scope: " + name);
                std::string type = d.type;
                if (!d.values.empty()) {
                    const auto valueType = expressionType(*d.values[i], scopes, functions, program);
                    if (type == "auto") type = valueType;
                    else if (type != "const" && !assignable(type, valueType))
                        throw std::runtime_error("Cannot initialize '" + name + "' of type " + type + " with " + valueType);
                }
                scopes.back()[name] = {type, d.isConst};
            }
        } else if (s.type == Statement::Type::Assignment) {
            std::string target;
            bool found = false;
            bool isConst = false;
            for (auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope) if (auto it = scope->find(s.name); it != scope->end()) { target = it->second.type; isConst = it->second.isConst; found = true; break; }
            if (!found) throw std::runtime_error("Cannot assign to undefined variable: " + s.name);
            if (isConst) throw std::runtime_error("Cannot assign to const variable: " + s.name);
            if (s.expression) {
                const auto source = expressionType(*s.expression, scopes, functions, program);
                if (s.operation == "=" && !assignable(target, source)) throw std::runtime_error("Cannot assign " + source + " to " + target);
                const bool stringConcatenation = s.operation == "+" && target == "string" && (source == "string" || source == "char");
                if (s.operation != "=" && !stringConcatenation && (!numeric(target) || !numeric(source)))
                    throw std::runtime_error("Compound arithmetic assignment requires numeric operands");
            } else if (s.operation != "=" && !numeric(target)) {
                throw std::runtime_error("Increment and decrement require a numeric variable");
            }
        } else if (s.type == Statement::Type::Print) {
            for (const auto& a : s.arguments) expressionType(*a, scopes, functions, program);
        } else if (s.type == Statement::Type::ExpressionStatement) expressionType(*s.expression, scopes, functions, program);
        else if (s.type == Statement::Type::Return) {
            if (!s.expression && !returnType.empty() && returnType != "void") throw std::runtime_error("Function must return a value of type " + returnType);
            if (s.expression) {
                const auto actual = expressionType(*s.expression, scopes, functions, program);
                if (returnType.empty() || returnType == "void") throw std::runtime_error("Void function cannot return a value");
                if (actual == "void") throw std::runtime_error("Cannot return a void expression");
                if (!assignable(returnType, actual)) throw std::runtime_error("Return value must be " + returnType + ", got " + actual);
            }
        } else if (s.type == Statement::Type::If) {
            for (const auto& branch : s.branches) {
                const auto conditionType = expressionType(*branch.condition, scopes, functions, program);
                if (conditionType != "bool" && conditionType != "any") throw std::runtime_error("if condition must be bool");
                scopes.emplace_back(); checkStatements(branch.statements, scopes, functions, program, returnType, loopStack); scopes.pop_back();
            }
            if (!s.elseStatements.empty()) { scopes.emplace_back(); checkStatements(s.elseStatements, scopes, functions, program, returnType, loopStack); scopes.pop_back(); }
        } else if (s.type == Statement::Type::For) {
            scopes.emplace_back();
            checkStatements(s.initializer, scopes, functions, program, returnType, loopStack);
            if (s.expression) {
                const auto conditionType = expressionType(*s.expression, scopes, functions, program);
                if (conditionType != "bool" && conditionType != "any") throw std::runtime_error("for-loop condition must be bool");
            }
            loopStack.push_back(s.loopName);
            scopes.emplace_back();
            checkStatements(s.loopStatements, scopes, functions, program, returnType, loopStack);
            scopes.pop_back();
            loopStack.pop_back();
            checkStatements(s.post, scopes, functions, program, returnType, loopStack);
            scopes.pop_back();
        } else if (s.type == Statement::Type::Break) {
            if (loopStack.empty()) throw std::runtime_error("break can only be used inside a loop");
            if (!s.name.empty() && std::find(loopStack.begin(), loopStack.end(), s.name) == loopStack.end())
                throw std::runtime_error("Unknown loop name in break: " + s.name);
        }
    }
}
}

void validateProgram(const Program& program)
{
    Functions functions;
    std::unordered_map<std::string, std::unordered_set<std::string>> signatures;
    for (const auto& function : program.functions) {
        std::string signature;
        for (const auto& parameter : function.parameters) signature += canonicalType(parameter.second) + ";";
        if (!signatures[function.name].insert(signature).second)
            throw std::runtime_error("Duplicate function overload: " + function.name);
        functions[function.name].push_back(&function);
    }
    const Function* mainFunction = nullptr;
    if (auto it = functions.find("main"); it != functions.end()) {
        if (it->second.size() != 1) throw std::runtime_error("Function 'main' cannot be overloaded");
        mainFunction = it->second.front();
    }
    if (!mainFunction) throw std::runtime_error("No main function");
    if (!mainFunction->parameters.empty()) throw std::runtime_error("Function 'main' must not have parameters");
    for (const auto& f : program.functions) {
        Types scopes(1);
        std::unordered_set<std::string> params;
        for (const auto& [name, type] : f.parameters) {
            if (!params.insert(name).second) throw std::runtime_error("Duplicate parameter '" + name + "' in function '" + f.name + "'");
            scopes.back()[name] = {type, false};
        }
        std::vector<std::string> loopStack;
        checkStatements(f.statements, scopes, functions, program, f.returnType, loopStack);
        if (!f.returnType.empty() && f.returnType != "void" && !returnsOnAllPaths(f.statements))
            throw std::runtime_error("Function '" + f.name + "' may complete without returning a value");
    }
}
