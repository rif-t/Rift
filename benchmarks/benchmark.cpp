#include "ast.h"
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"

#include <atomic>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <new>
#include <string>
#include <vector>

using namespace rift;

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
    std::vector<double> times;
    std::vector<double> allocations, bytes;
    fn(); // Warm caches and retained interpreter storage before sampling.
    for (int sample = 0; sample < 7; ++sample) {
        std::size_t count = 0, allocated = 0;
        const auto start = std::chrono::steady_clock::now();
        {
            AllocationWindow window;
            fn();
            count = allocationCount.load();
            allocated = allocationBytes.load();
        }
        times.push_back(std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - start).count() / operations);
        allocations.push_back(static_cast<double>(count) / operations);
        bytes.push_back(static_cast<double>(allocated) / operations);
    }
    const auto median = [](std::vector<double>& values) {
        std::sort(values.begin(), values.end());
        return values[values.size() / 2];
    };
    std::cout << name << ": median " << median(times) << " ns/op, "
              << median(allocations) << " allocations/op, " << median(bytes) << " allocated bytes/op (7 runs)\n";
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

std::string nestedExpressionProgram()
{
    std::string expression = "1";
    for (int i = 0; i < 100; ++i) expression = "1 + (" + expression + ")";
    return "func main() { value: int = " + expression + " }\n";
}

std::string shallowCallProgram()
{
    return R"(func increment(value: int) -> int { return value + 1 }
func main() {
total := 0
for i := 0; i < 1000; i++ { total += increment(i) }
})";
}

std::string manyParameterProgram()
{
    return R"(func sum(a: int, b: int, c: int, d: int, e: int, f: int, g: int, h: int) -> int {
return a + b + c + d + e + f + g + h
}
func main() {
total := 0
for i := 0; i < 500; i++ { total += sum(i, i, i, i, i, i, i, i) }
})";
}

std::string localAccessProgram()
{
    return R"(func main() {
value: int = 0
for i := 0; i < 10000; i++ { value += i }
})";
}

std::string shadowingProgram()
{
    return R"(func main() {
value: int = 1
if true { value: int = 2
if true { value: int = 3
if true { value: int = 4 } } }
})";
}

std::string controlFlowProgram()
{
    return R"(func work(n: int) -> int {
total: int = 0
for i := 0; i < n; i++ {
if i % 2 == 0 { loop.continue }
total += i
}
return total
}
func main() { work(1000) })";
}

std::string integerProgram(const std::string& type)
{
    return "func main() { value: " + type + " = 1\nfor i := 0; i < 10000; i++ { value += i * 3 }\n}";
}

std::string manyVariablesProgram(int count = 200)
{
    std::string source = "func main() {\n";
    for (int i = 0; i < count; ++i) source += "value" + std::to_string(i) + ": int = " + std::to_string(i) + "\n";
    for (int i = 0; i < count; ++i) source += "value" + std::to_string(i) + " += 1\n";
    return source + "}\n";
}

std::string arrayWorkload(const std::string& body)
{
    std::string source = "func consume(values: int[]) { item := values[0] }\nfunc main() {\nvalues := [";
    for (int i = 0; i < 2048; ++i) source += (i ? "," : "") + std::to_string(i);
    source += "]\n" + body + "\n}\n";
    return source;
}

std::string deepArrayWorkload()
{
    std::string type = "int", literal = "1", path = "copy";
    for (int i = 0; i < 8; ++i) {
        type += "[]";
        literal = "[" + literal + "]";
        path += "[0]";
    }
    return "func main() {\nvalues: " + type + " = " + literal + "\nfor i := 0; i < 40; i++ {\ncopy := values\n" + path + " = i\n}\n}\n";
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
    runWorkload("shallow function calls (1000 calls)", shallowCallProgram(), 5);
    runWorkload("function calls with 8 parameters (500 calls)", manyParameterProgram(), 5);
    runWorkload("scope and variable lookup (250 frames)", lookupProgram(), 150);
    runWorkload("string concatenation (150 steps)", stringProgram(), 100);
    runWorkload("nested expressions (100 levels)", nestedExpressionProgram(), 300);
    runWorkload("local reads and writes (10000 iterations)", localAccessProgram(), 3);
    runWorkload("nested shadowed scopes", shadowingProgram(), 100);
    runWorkload("loop control and returns (1000 iterations)", controlFlowProgram(), 5);
    runWorkload("int32 arithmetic (10000 iterations)", integerProgram("int32"), 3);
    runWorkload("int64 arithmetic (10000 iterations)", integerProgram("int64"), 3);
    runWorkload("int128 arithmetic (10000 iterations)", integerProgram("int128"), 3);
    runWorkload("many variables and expressions (200 locals)", manyVariablesProgram(), 10);
    for (const int count : {10, 50, 100, 200, 500, 1000}) {
        const Program analysisProgram = parseSource(manyVariablesProgram(count));
        measure("semantic analysis only (" + std::to_string(count) + " locals)", 100,
            [&] { for (int i = 0; i < 100; ++i) (void)validateProgram(analysisProgram); });
    }

    runWorkload("array reads (2048 items x 100 reads)", arrayWorkload(R"(for i := 0; i < 100; i++ { copy := values })"), 3);
    runWorkload("array element access (2048 items x 1000 accesses)", arrayWorkload(R"(total := 0
for i := 0; i < 1000; i++ { total += values[i % 2048] })"), 3);
    runWorkload("large array copies (2048 items x 20 copies)", arrayWorkload(R"(for i := 0; i < 20; i++ { copy := values })"), 3);
    runWorkload("shared array modifications (2048 items x 20 detaches)", arrayWorkload(R"(for i := 0; i < 20; i++ {
copy := values
copy[0] = i
})"), 3);
    runWorkload("array function arguments (2048 items x 100 calls)", arrayWorkload(R"(for i := 0; i < 100; i++ { consume(values) })"), 3);
    runWorkload("array append (1000 elements)", R"(func main() { values: int[] = []
for i := 0; i < 1000; i++ { values.append(i) } })", 3);
    runWorkload("nested array reads and copies", R"(func main() { values: int[][] = [[1, 2], [3, 4], [5, 6]]
for i := 0; i < 500; i++ {
copy := values
item := values[1]
} })", 3);
    runWorkload("deep nested array copies and writes (8 levels x 40)", deepArrayWorkload(), 3);

    const UInt128 dividend{UINT64_C(0xfedcba9876543210), UINT64_C(0x123456789abcdef0)};
    const UInt128 divisor{0, UINT64_C(123456789)};
    const UInt128 narrowDividend{0, UINT64_C(9876543210123)};
    const UInt128 narrowDivisor{0, UINT64_C(1234567)};
    measure("UInt128 narrow multiply", 200000, [&] {
        for (std::uint64_t i = 0; i < 200000; ++i) {
            bool overflow = false;
            const auto product = UInt128::multiply({0, i & 0xffffffffu}, {0, 3}, overflow);
            resultSink = resultSink ^ product.low ^ static_cast<std::uint64_t>(overflow);
        }
    });
    measure("UInt128 narrow divide", 200000, [&] {
        for (std::uint64_t i = 0; i < 200000; ++i) {
            UInt128 remainder{};
            const auto quotient = UInt128::divide(narrowDividend, narrowDivisor, remainder);
            resultSink = resultSink ^ quotient.low ^ remainder.low;
        }
    });
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
