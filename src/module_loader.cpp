#include "module_loader.h"

#include "lexer.h"
#include "parser.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace rift {
namespace {

void renamePrefix(std::string& name, const std::string& from, const std::string& to)
{
    if (name.starts_with(from + ".")) name.replace(0, from.size(), to);
}

void renameExpression(Expression& expression, const std::string& from, const std::string& to)
{
    renamePrefix(expression.name, from, to);
    if (expression.left) renameExpression(*expression.left, from, to);
    if (expression.right) renameExpression(*expression.right, from, to);
    if (expression.third) renameExpression(*expression.third, from, to);
    for (auto& argument : expression.arguments) renameExpression(*argument, from, to);
}

void renameStatements(std::vector<Statement>& statements, const std::string& from, const std::string& to)
{
    for (auto& statement : statements) {
        renamePrefix(statement.name, from, to);
        if (statement.expression) renameExpression(*statement.expression, from, to);
        for (auto& argument : statement.arguments) renameExpression(*argument, from, to);
        for (auto& branch : statement.branches) {
            if (branch.condition) renameExpression(*branch.condition, from, to);
            renameStatements(branch.statements, from, to);
        }
        renameStatements(statement.elseStatements, from, to);
        renameStatements(statement.initializer, from, to);
        renameStatements(statement.post, from, to);
        renameStatements(statement.loopStatements, from, to);
        renameStatements(statement.loopElseStatements, from, to);
    }
}

void renameProgram(Program& program, const std::string& from, const std::string& to)
{
    for (auto& function : program.functions) {
        renamePrefix(function.name, from, to);
        renameStatements(function.statements, from, to);
    }
    for (auto& name : program.usingNamespaces) renamePrefix(name, from, to);
    for (auto& name : program.topLevelNamespaces) {
        if (name == from) name = to;
    }
}

class Loader
{
public:
    Program load(const std::filesystem::path& entryPath)
    {
        return loadFile(entryPath, true, {});
    }

private:
    Program parseFile(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file) throw std::runtime_error("Could not open file: " + path.string());
        std::stringstream buffer;
        buffer << file.rdbuf();
        Lexer lexer(buffer.str());
        const auto tokens = lexer.tokenize();
        Parser parser(tokens);
        return parser.parse();
    }

    std::filesystem::path resolveImport(const std::filesystem::path& importer, const std::string& rawPath)
    {
        std::filesystem::path path(rawPath);
        if (path.extension().empty()) path += ".rift";
        if (path.is_relative()) path = importer.parent_path() / path;
        std::error_code error;
        auto canonical = std::filesystem::canonical(path, error);
        if (error) throw std::runtime_error("Could not resolve import '" + rawPath + "' from " + importer.string());
        return canonical;
    }

    Program loadFile(const std::filesystem::path& inputPath, bool isEntry, const std::string& binding)
    {
        std::error_code error;
        const auto path = std::filesystem::canonical(inputPath, error);
        if (error) throw std::runtime_error("Could not open file: " + inputPath.string());
        if (active_.contains(path)) throw std::runtime_error("Import cycle involving: " + path.string());
        if (!isEntry) {
            if (auto existing = loaded_.find(path); existing != loaded_.end()) {
                if (existing->second != binding)
                    throw std::runtime_error("Imported file has conflicting names: " + path.string());
                return {};
            }
        }

        Program source = parseFile(path);
        if (!isEntry) {
            if (std::any_of(source.functions.begin(), source.functions.end(), [](const Function& function) {
                return function.name == "main" || function.name.ends_with(".main");
            }))
                throw std::runtime_error("Imported file cannot declare main: " + path.string());
        }

        active_.insert(path);
        Program combined;
        try {
            for (const auto& import : source.imports) {
                auto importedPath = resolveImport(path, import.path);
                Program imported = loadFile(importedPath, false, import.alias);
                combined.functions.insert(combined.functions.end(),
                    std::make_move_iterator(imported.functions.begin()), std::make_move_iterator(imported.functions.end()));
                combined.usingNamespaces.insert(combined.usingNamespaces.end(),
                    std::make_move_iterator(imported.usingNamespaces.begin()), std::make_move_iterator(imported.usingNamespaces.end()));
            }
            if (!isEntry && !binding.empty()) {
                const auto namespaces = source.topLevelNamespaces;
                for (const auto& name : namespaces)
                    renameProgram(source, name, namespaces.size() == 1 ? binding : binding + "." + name);
            }
            combined.functions.insert(combined.functions.end(),
                std::make_move_iterator(source.functions.begin()), std::make_move_iterator(source.functions.end()));
            combined.usingNamespaces.insert(combined.usingNamespaces.end(),
                std::make_move_iterator(source.usingNamespaces.begin()), std::make_move_iterator(source.usingNamespaces.end()));
            if (isEntry) combined.topLevelNamespaces = std::move(source.topLevelNamespaces);
            if (!isEntry) loaded_[path] = binding;
        } catch (...) {
            active_.erase(path);
            throw;
        }
        active_.erase(path);
        return combined;
    }

    std::unordered_map<std::filesystem::path, std::string> loaded_;
    std::unordered_set<std::filesystem::path> active_;
};

}

Program loadProgram(const std::filesystem::path& entryPath)
{
    return Loader{}.load(entryPath);
}

}
