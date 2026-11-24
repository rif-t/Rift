#pragma once

#include "ast.h"

#include <unordered_map>

namespace rift {

struct ExpressionAnalysis {
    std::string resolvedType;
    const Function* resolvedFunction = nullptr;
};
struct AnalysisResult {
    std::unordered_map<const Expression*, ExpressionAnalysis> expressions;
};

AnalysisResult validateProgram(const Program& program);

}
