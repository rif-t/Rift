#pragma once

#include "ast.h"
#include "fmt.h"

#include <optional>
#include <string>

namespace rift::runtime {
WideInteger applyArithmetic(const std::string& operation, WideInteger left, WideInteger right);
RuntimeValue applyNumericArithmetic(const std::string& operation, const RuntimeValue& left, const RuntimeValue& right);
WideInteger negate(WideInteger value);
bool compareValues(const RuntimeValue& left, const RuntimeValue& right, const std::string& operation);
bool requireBoolean(const RuntimeValue& value, const std::string& context);
RuntimeValue validateValue(RuntimeValue value, const std::string& type, const std::string& name);
RuntimeValue cloneValue(const RuntimeValue& value);
void ensureUniqueArray(std::shared_ptr<RuntimeArray>& array);
std::string stringify(const RuntimeValue& value);
fmt::Argument formatArgument(const RuntimeValue& value);
}
