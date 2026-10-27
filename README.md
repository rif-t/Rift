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
