#include "fmt.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace rift::fmt {
namespace {
char alignment(char c) { return c == '<' || c == '>' || c == '^' ? c : '\0'; }

std::string render(const Argument& argument, const std::string& spec)
{
    char fill = ' ';
    char align = '\0';
    std::size_t cursor = 0;
    if (spec.size() >= 2 && alignment(spec[1])) {
        fill = spec[0];
        align = spec[1];
        cursor = 2;
    } else if (!spec.empty() && alignment(spec[0])) {
        align = spec[0];
        cursor = 1;
    }

    char type = '\0';
    if (cursor < spec.size() && (spec.back() == 'd' || spec.back() == 'x' || spec.back() == 'X' || spec.back() == 'b' || spec.back() == 'o' || spec.back() == 's')) {
        type = spec.back();
        if (type == 's') {
            if (argument.isInteger) throw std::runtime_error("String format specifier used with integer");
        } else if (!argument.isInteger) {
            throw std::runtime_error("Numeric format specifier used with non-integer");
        }
    }

    std::string value = argument.text;
    if (argument.isInteger && type && type != 'd') {
        const unsigned base = type == 'x' || type == 'X' ? 16 : type == 'b' ? 2 : 8;
        value = argument.integer.toBase(base, type == 'X');
    }

    std::size_t widthEnd = type ? spec.size() - 1 : spec.size();
    std::size_t width = 0;
    bool zeroPad = false;
    if (cursor < widthEnd && spec[cursor] == '0' && argument.isInteger) { zeroPad = true; fill = '0'; ++cursor; }
    for (; cursor < widthEnd; ++cursor) {
        if (spec[cursor] < '0' || spec[cursor] > '9') throw std::runtime_error("Invalid format specifier: " + spec);
        width = width * 10 + static_cast<std::size_t>(spec[cursor] - '0');
        if (width > 100000) throw std::runtime_error("Format width is too large");
    }
    if (width <= value.size()) return value;
    const std::size_t padding = width - value.size();
    if (align == '\0') align = argument.isInteger ? '>' : '<';
    if (align == '<') return value + std::string(padding, fill);
    if (align == '^') {
        const std::size_t left = padding / 2;
        return std::string(left, fill) + value + std::string(padding - left, fill);
    }
    if (zeroPad && !value.empty() && value[0] == '-') return "-" + std::string(padding, fill) + value.substr(1);
    return std::string(padding, fill) + value;
}
}

std::string format(const std::string& pattern, const std::vector<Argument>& arguments)
{
    std::string result;
    result.reserve(pattern.size());
    std::size_t nextArgument = 0;
    bool automaticIndexing = false;
    bool manualIndexing = false;
    std::vector<bool> usedArguments(arguments.size(), false);
    for (std::size_t i = 0; i < pattern.size(); ++i) {
        if (pattern[i] == '{') {
            if (i + 1 < pattern.size() && pattern[i + 1] == '{') { result += '{'; ++i; continue; }
            const auto close = pattern.find('}', i + 1);
            if (close == std::string::npos) throw std::runtime_error("Unclosed '{' in format string");
            const std::string field = pattern.substr(i + 1, close - i - 1);
            const auto colon = field.find(':');
            const std::string key = field.substr(0, colon);
            const std::string spec = colon == std::string::npos ? "" : field.substr(colon + 1);
            std::size_t argumentIndex = nextArgument;
            if (key.empty()) {
                if (manualIndexing) throw std::runtime_error("Cannot mix automatic and manual argument indexing");
                automaticIndexing = true;
                ++nextArgument;
            } else {
                if (automaticIndexing) throw std::runtime_error("Cannot mix automatic and manual argument indexing");
                manualIndexing = true;
                argumentIndex = 0;
                for (char digit : key) {
                    if (digit < '0' || digit > '9' || argumentIndex > (std::numeric_limits<std::size_t>::max() - static_cast<std::size_t>(digit - '0')) / 10)
                        throw std::runtime_error("Invalid field in format string: {" + field + "}");
                    argumentIndex = argumentIndex * 10 + static_cast<std::size_t>(digit - '0');
                }
            }
            if (argumentIndex >= arguments.size()) throw std::runtime_error("Not enough arguments for format string");
            usedArguments[argumentIndex] = true;
            result += render(arguments[argumentIndex], spec);
            i = close;
        } else if (pattern[i] == '}') {
            if (i + 1 < pattern.size() && pattern[i + 1] == '}') { result += '}'; ++i; }
            else throw std::runtime_error("Unmatched '}' in format string");
        } else result += pattern[i];
    }
    if (std::find(usedArguments.begin(), usedArguments.end(), false) != usedArguments.end())
        throw std::runtime_error("Too many arguments for format string");
    return result;
}

void print(const std::string& pattern, const std::vector<Argument>& arguments) { std::cout << format(pattern, arguments); }
void println(const std::string& pattern, const std::vector<Argument>& arguments) { std::cout << format(pattern, arguments) << '\n'; }
void write(const std::string& text, bool newline) { std::cout << text; if (newline) std::cout << '\n'; }
}
