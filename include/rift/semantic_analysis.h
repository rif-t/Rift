#pragma once

#include "ast.h"

#include <cstddef>
#include <unordered_map>
#include <vector>

namespace rift {

struct ExpressionAnalysis {
    std::string resolvedType;
    const Function* resolvedFunction = nullptr;
    std::size_t localSlot = static_cast<std::size_t>(-1);
};
struct AnalysisResult {
    std::unordered_map<const Expression*, ExpressionAnalysis> expressions;
    std::unordered_map<const VariableDeclaration*, std::size_t> declarationSlots;
    std::unordered_map<const Statement*, std::size_t> assignmentSlots;
    std::unordered_map<const std::vector<Statement>*, std::vector<std::size_t>> scopeSlots;
    std::unordered_map<const Statement*, std::vector<std::size_t>> loopScopeSlots;
    std::unordered_map<const Function*, std::size_t> functionSlotCounts;
};

AnalysisResult validateProgram(const Program& program);

}
