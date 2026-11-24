#include <iostream>

#include "interpreter.h"
#include "module_loader.h"

using namespace rift;

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: rift <file>\n";
        return 1;
    }

    try {
        Program program = loadProgram(argv[1]);

        Interpreter interpreter;
        interpreter.run(program);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: "
            << e.what()
            << '\n';

        return 1;
    }

    return 0;
}
