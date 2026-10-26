#include "ast.h"
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <new>
#include <string>

namespace {
std::atomic<bool> countAllocations{false};
std::atomic<std::size_t> allocationCount{0};
std::atomic<std::size_t> allocationBytes{0};
volatile std::uint64_t resultSink = 0;

struct AllocationWindow {
    AllocationWindow() { allocationCount = 0; allocationBytes = 0; countAllocations = true; }
    ~AllocationWindow() { countAllocations = false; }
};

template <class F>
void measure(const std::string& name, std::size_t operations, F&& fn)
{
    const auto start = std::chrono::steady_clock::now();
    std::size_t count = 0, bytes = 0;
    {
        AllocationWindow window;
        fn();
        count = allocationCount.load();
        bytes = allocationBytes.load();
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
    std::cout << name << ": " << elapsed / static_cast<double>(operations) << " ns/op, "
              << count << " allocations, " << bytes << " allocated bytes\n";
}

Program parseSource(const std::string& source)
{
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    return parser.parse();
}

void countExpression(const Expression* expression, std::size_t& count)
{
    if (!expression) return;
    ++count;
    countExpression(expression->left.get(), count);
    countExpression(expression->right.get(), count);
    countExpression(expression->third.get(), count);
    for (const auto& argument : expression->arguments) countExpression(argument.get(), count);
}

void countStatements(const std::vector<Statement>& statements, std::size_t& statementCount, std::size_t& expressionCount)
{
    for (const auto& statement : statements) {
        ++statementCount;
        if (statement.variable) for (const auto& value : statement.variable->values) countExpression(value.get(), expressionCount);
        countExpression(statement.expression.get(), expressionCount);
        for (const auto& argument : statement.arguments) countExpression(argument.get(), expressionCount);
        for (const auto& branch : statement.branches) {
            countExpression(branch.condition.get(), expressionCount);
            countStatements(branch.statements, statementCount, expressionCount);
        }
        countStatements(statement.elseStatements, statementCount, expressionCount);
        countStatements(statement.blockStatements, statementCount, expressionCount);
    }
}

std::string arithmeticProgram()
{
    return R"(func work(n: int, acc: int128) -> int128 {
    if n == 0 { return acc }
    return work(n - 1, acc + n * n)
}
func main() { work(250, 0) }
)";
}

std::string emptyProgram()
{
    return "func main() {}\n";
}

std::string callProgram()
{
    return R"(func work(n: int) -> int {
    if n == 0 { return 0 }
    return work(n - 1)
}
func main() { work(300) }
)";
}

std::string lookupProgram()
{
    return R"(func work(n: int, seed: int) -> int {
    if n == 0 { return seed }
    local: int = n
    return work(local - 1, seed + local)
}
func main() { work(250, 1) }
)";
}

std::string stringProgram()
{
    return R"(func work(n: int, value: string) -> int {
    if n == 0 { return 0 }
    return work(n - 1, value + "x")
}
func main() { work(150, "") }
)";
}
}

void* operator new(std::size_t size)
{
    if (void* p = std::malloc(size ? size : 1)) {
        if (countAllocations.load(std::memory_order_relaxed)) { ++allocationCount; allocationBytes += size; }
        return p;
    }
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

int runBenchmarks()
{
    std::cout << "Rift benchmark (Release; steady_clock; allocation counts include ordinary new/new[] only)\n";
    std::cout << "sizeof(RuntimeValue)=" << sizeof(RuntimeValue) << " sizeof(Expression)=" << sizeof(Expression)
              << " sizeof(Statement)=" << sizeof(Statement) << " sizeof(WideInteger)=" << sizeof(WideInteger)
              << " sizeof(Decimal)=" << sizeof(Decimal) << "\n";

    std::string largeSource = "func main() {\n";
    for (int i = 0; i < 3000; ++i) largeSource += "v" + std::to_string(i) + ": int = " + std::to_string(i) + "\n";
    largeSource += "}\n";
    Program largeProgram;
    measure("large AST parse (3000 declarations)", 1, [&] { largeProgram = parseSource(largeSource); });
    std::size_t statementCount = 0, expressionCount = 0;
    for (const auto& function : largeProgram.functions) countStatements(function.statements, statementCount, expressionCount);
    std::cout << "AST nodes: " << statementCount << " statements, " << expressionCount << " expressions; "
              << "object-size estimate=" << statementCount * sizeof(Statement) + expressionCount * sizeof(Expression) << " bytes\n";

    auto runWorkload = [](const std::string& label, const std::string& source, std::size_t runs) {
        Program program = parseSource(source);
        Interpreter interpreter;
        measure(label, runs, [&] { for (std::size_t i = 0; i < runs; ++i) interpreter.run(program); });
    };
    runWorkload("no-op interpreter run", emptyProgram(), 150);
    runWorkload("arithmetic plus recursion (250 frames)", arithmeticProgram(), 150);
    runWorkload("function calls (300 frames)", callProgram(), 150);
    runWorkload("scope and variable lookup (250 frames)", lookupProgram(), 150);
    runWorkload("string concatenation (150 steps)", stringProgram(), 100);

    const UInt128 dividend{UINT64_C(0xfedcba9876543210), UINT64_C(0x123456789abcdef0)};
    const UInt128 divisor{0, UINT64_C(123456789)};
    measure("UInt128 multiply", 200000, [&] {
        for (std::uint64_t i = 0; i < 200000; ++i) {
            bool overflow = false;
            const auto product = UInt128::multiply(dividend, divisor, overflow);
            resultSink = resultSink ^ product.low ^ static_cast<std::uint64_t>(overflow);
        }
    });
    measure("UInt128 divide", 200000, [&] {
        for (std::uint64_t i = 0; i < 200000; ++i) {
            UInt128 remainder{};
            const auto quotient = UInt128::divide(dividend, divisor, remainder);
            resultSink = resultSink ^ quotient.low ^ remainder.low;
        }
    });
    const WideInteger formatted{dividend, false, true, 128};
    measure("UInt128 decimal formatting", 100000, [&] {
        for (std::uint64_t i = 0; i < 100000; ++i) resultSink = resultSink + formatted.toString().size();
    });
    std::cout << "sink=" << resultSink << "\n";
    return 0;
}

int main()
{
    try { return runBenchmarks(); }
    catch (const std::exception& error) { std::cerr << "Benchmark failed: " << error.what() << "\n"; return 1; }
}
