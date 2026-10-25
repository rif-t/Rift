#pragma once

#include "integer.h"
#include <string>
#include <vector>

namespace rift::fmt {
struct Argument {
    std::string text;
    bool isInteger = false;
    WideInteger integer;
};

std::string format(const std::string& pattern, const std::vector<Argument>& arguments);
void print(const std::string& pattern, const std::vector<Argument>& arguments);
void println(const std::string& pattern, const std::vector<Argument>& arguments);
void write(const std::string& text, bool newline);
}
