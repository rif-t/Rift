#include "semantic_analysis.h"

#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace {
using Types = std::vector<std::unordered_map<std::string, std::string>>;
using Functions = std::unordered_map<std::string, const Function*>;

bool numeric(const std::string& t) { return t == "any" || t == "int" || t == "float" || t == "double" || t == "decimal" || t.starts_with("int") || t.starts_with("uint"); }
bool assignable(const std::string& target, const std::string& source)
{
    if (target == "any" || target == source || source == "any") return true;
    if (numeric(target) && numeric(source)) return true; // Runtime range checks enforce narrowing conversions.
    return false;
}

const Function* resolve(const std::string& name, const Functions& functions, const Program& program)
{
    if (auto it = functions.find(name); it != functions.end()) return it->second;
    if (name.find('.') == std::string::npos) {
        const Function* result = nullptr;
        for (const auto& ns : program.usingNamespaces) {
            if (ns == "std") continue;
            if (auto it = functions.find(ns + "." + name); it != functions.end()) {
                if (result) throw std::runtime_error("Ambiguous function '" + name + "'; qualify it with its namespace");
                result = it->second;
            }
        }
        return result;
    }
    return nullptr;
}

std::string expressionType(const Expression& e, Types& scopes, const Functions& functions, const Program& program)
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
            if (auto it = s->find(e.name); it != s->end()) return it->second;
        throw std::runtime_error("Undefined variable: " + e.name);
    case Expression::Type::Unary: {
        const auto type = expressionType(*e.left, scopes, functions, program);
        if (e.operation == "!") { if (type != "bool") throw std::runtime_error("Logical negation requires bool"); return "bool"; }
        if (type != "int" && type != "float" && type != "double" && type != "decimal" && !type.starts_with("uint") && !type.starts_with("int"))
            throw std::runtime_error("Unary numeric operator requires a number");
        return type;
    }
    case Expression::Type::Binary: {
        auto l = expressionType(*e.left, scopes, functions, program);
        auto r = expressionType(*e.right, scopes, functions, program);
        if (e.operation == "&&" || e.operation == "||") {
            if (l != "bool" || r != "bool") throw std::runtime_error("Logical operators require bool operands");
            return "bool";
        }
        if (e.operation == "==" || e.operation == "!=" || e.operation == "<" || e.operation == "<=" || e.operation == ">" || e.operation == ">=") return "bool";
        if (e.operation == "+" && (l == "string" || l == "char") && (r == "string" || r == "char")) return "string";
        const bool ln = numeric(l);
        const bool rn = numeric(r);
        if (!ln || !rn) throw std::runtime_error("Arithmetic operators require numeric operands");
        if (l == "any" || r == "any") return "any";
        if (l == "decimal" || r == "decimal") return "decimal";
        if (l == "double" || r == "double") return "double";
        if (l == "float" || r == "float") return "float";
        return l;
    }
    case Expression::Type::Conditional: {
        if (expressionType(*e.left, scopes, functions, program) != "bool") throw std::runtime_error("Conditional expression condition must be bool");
        auto a = expressionType(*e.right, scopes, functions, program);
        auto b = expressionType(*e.third, scopes, functions, program);
        if (a == b) return a;
        if ((a == "int" || a.starts_with("int") || a.starts_with("uint")) && (b == "float" || b == "double" || b == "decimal")) return b;
        if ((b == "int" || b.starts_with("int") || b.starts_with("uint")) && (a == "float" || a == "double" || a == "decimal")) return a;
        throw std::runtime_error("Conditional branches must have compatible types");
    }
    case Expression::Type::Call: {
        std::vector<std::string> argumentTypes;
        for (const auto& arg : e.arguments) argumentTypes.push_back(expressionType(*arg, scopes, functions, program));
        if (e.name.starts_with("std.types.") || e.name.starts_with("types.")) return "any";
        static const std::unordered_set<std::string> typeBuiltins = {"size_of", "sizeof", "max_value", "min_value", "to_string", "type_of", "typeof", "is_integer", "is_numeric", "is_bool", "to_int", "to_uint", "to_float", "to_double", "to_decimal", "to_int8", "to_int16", "to_int32", "to_int64", "to_int128", "to_uint8", "to_uint16", "to_uint32", "to_uint64", "to_uint128"};
        if (typeBuiltins.contains(e.name)) return e.name == "to_string" ? "string" : "any";
        const Function* f = resolve(e.name, functions, program);
        if (!f) throw std::runtime_error("Undefined function: " + e.name);
        if (e.arguments.size() != f->parameters.size()) throw std::runtime_error("Function '" + e.name + "' expects " + std::to_string(f->parameters.size()) + " argument(s)");
        for (std::size_t i = 0; i < argumentTypes.size(); ++i)
            if (!assignable(f->parameters[i].second, argumentTypes[i]))
                throw std::runtime_error("Argument " + std::to_string(i + 1) + " of function '" + e.name + "' must be " + f->parameters[i].second + ", got " + argumentTypes[i]);
        return f->returnType.empty() ? "void" : f->returnType;
    }
    }
    return "any";
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

void checkStatements(const std::vector<Statement>& statements, Types& scopes, const Functions& functions, const Program& program, const std::string& returnType)
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
                scopes.back()[name] = type;
            }
        } else if (s.type == Statement::Type::Assignment) {
            std::string target;
            bool found = false;
            for (auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope) if (auto it = scope->find(s.name); it != scope->end()) { target = it->second; found = true; break; }
            if (!found) throw std::runtime_error("Cannot assign to undefined variable: " + s.name);
            if (s.expression) {
                const auto source = expressionType(*s.expression, scopes, functions, program);
                if (s.operation == "=" && !assignable(target, source)) throw std::runtime_error("Cannot assign " + source + " to " + target);
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
                if (expressionType(*branch.condition, scopes, functions, program) != "bool") throw std::runtime_error("if condition must be bool");
                scopes.emplace_back(); checkStatements(branch.statements, scopes, functions, program, returnType); scopes.pop_back();
            }
            if (!s.elseStatements.empty()) { scopes.emplace_back(); checkStatements(s.elseStatements, scopes, functions, program, returnType); scopes.pop_back(); }
        }
    }
}
}

void validateProgram(const Program& program)
{
    Functions functions;
    for (const auto& function : program.functions)
        if (!functions.emplace(function.name, &function).second) throw std::runtime_error("Duplicate function: " + function.name);
    const Function* mainFunction = nullptr;
    for (const auto& f : program.functions) if (f.name == "main") mainFunction = &f;
    if (!mainFunction) throw std::runtime_error("No main function");
    if (!mainFunction->parameters.empty()) throw std::runtime_error("Function 'main' must not have parameters");
    for (const auto& f : program.functions) {
        Types scopes(1);
        std::unordered_set<std::string> params;
        for (const auto& [name, type] : f.parameters) {
            if (!params.insert(name).second) throw std::runtime_error("Duplicate parameter '" + name + "' in function '" + f.name + "'");
            scopes.back()[name] = type;
        }
        checkStatements(f.statements, scopes, functions, program, f.returnType);
        if (!f.returnType.empty() && f.returnType != "void" && !returnsOnAllPaths(f.statements))
            throw std::runtime_error("Function '" + f.name + "' may complete without returning a value");
    }
}
