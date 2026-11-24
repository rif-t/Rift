#include "rift/module_loader.h"
#include "rift/semantic_analysis.h"

#include <algorithm>
#include <cassert>
#include <string>

int main(int argc, char* argv[])
{
    assert(argc == 2);
    const rift::Program program = rift::loadProgram(argv[1]);
    assert(std::any_of(program.functions.begin(), program.functions.end(), [](const rift::Function& function) {
        return function.name == "math.answer";
    }));
    assert(std::any_of(program.functions.begin(), program.functions.end(), [](const rift::Function& function) {
        return function.name == "main";
    }));
    rift::validateProgram(program);
}
