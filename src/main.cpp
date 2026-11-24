#include <fstream>
#include <iostream>
#include <sstream>

#include "interpreter.h"
#include "lexer.h"
#include "parser.h"

using namespace rift;

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: rift <file>\n";
        return 1;
    }

    std::ifstream file(argv[1]);

    if (!file) {
        std::cerr << "Could not open file: "
            << argv[1] << '\n';
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    std::string source = buffer.str();

    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        Parser parser(tokens);
        Program program = parser.parse();

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
