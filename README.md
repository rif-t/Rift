# Rift

Rift is an experimental C-like programming language developed as a school
science project. The project is written in C++23 and currently includes a
lexer, parser, semantic analysis, interpreter, and a built-in standard library.

## Build

Rift uses CMake:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

To build and package a release ZIP on Windows, run `./release.ps1` from PowerShell.
It writes `release/Rift.zip` with `rift.exe`, the docs, examples, README, and license.

The `rift_core` library can be installed with `cmake --install build --prefix <prefix>`.
Consumers can use `find_package(Rift CONFIG REQUIRED)` and link `rift::core`.
The C++ API is in the `rift` namespace; include headers as `<rift/parser.h>`,
`<rift/lexer.h>`, and `<rift/interpreter.h>`.

## Run a program

Save this as `hello.rift`:

```rift
using std

func main() {
    println("Hello, Rift!")
}
```

Build Rift, then run the file from PowerShell:

```powershell
.\build\Release\rift.exe hello.rift
```

## Language documentation

Feature guides are in [`docs/`](docs/index.md):

- [Data types and numeric behavior](docs/data_types.md)
- [Functions and overloads](docs/functions.md)
- [Conditions and boolean expressions](docs/conditions.md)
- [Loops](docs/loops.md)
- [Namespaces](docs/namespaces.md)
- [Standard library overview](docs/std.md)
- [`std.types`](docs/std-types.md)
