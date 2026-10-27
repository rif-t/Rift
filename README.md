# Rift

## Build and test

Rift uses CMake and C++23:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

CTest groups regressions under lexer, parser, integer, interpreter, type-system,
and integration labels. GitHub Actions runs the build and all groups on Linux
and Windows for pushes and pull requests.

Rift is an experimental C-like programming language.

## Standard library

Rift ships its `std` standard library. Use `using std;` to make its names
available directly, or qualify calls with `std.` when you prefer not to add a
using directive. No package installation is needed.

## Your first program with RIFT

```rift
using std

func main() {
    a : int = 2
    b : string = "program"
    println("The awesome {} with Rift", b)
    println(a+a*a)
}
```

## Booleans and conditions

Comparisons (`<`, `>`, `<=`, `>=`, `==`, `!=`) produce `bool` values. Boolean logic uses `&&`, `||`, `!` (or `and`, `or`, `not`) and short-circuits. Conditions can be written with or without parentheses; `elif` and `else` blocks are supported. The conditional expression `condition ? value : otherValue` selects a value.

```rift
using std

score: int = 72
passed: bool = score >= 50
result: string = passed ? "win" : "lose"

if (passed) {
    println("{}", result)
} elif score == 0 {
    println("no score")
} else {
    println("try again")
}
```

## Functions

Functions can take typed parameters and return a typed value. Calls can be used
inside expressions and formatting arguments.
Functions may be overloaded by parameter types. Rift selects an exact match
before considering numeric conversions or an `any` parameter. If multiple
overloads are equally good matches, semantic analysis reports an ambiguity;
the return type and parameter names do not distinguish overloads.

```rift
using std

func is_passing(score: int) -> bool {
    return score >= 50
}

func main() {
    println(is_passing(72) ? "pass" : "retry")
}
```

```rift
func label(value: int) -> string { return "integer" }
func label(value: string) -> string { return "text" }
```

## For loops

`for` supports an initializer, condition, and post statement. The initializer
can declare a scoped variable. A label before `for` lets `break` exit a
specific enclosing loop; an unlabelled `break` exits the innermost loop.

```rift
for i := 0; i < 10; i++ {
    println("{}", i)
}

outer: for value:double = 10.0d, value >= 0.0d; value -= 1.0d {
    if value == 4.0d {
        break outer
    }
}
```

## Namespaces

Use namespaces to group functions. Call a function with its namespace name,
or import that namespace with `using` to call its functions directly.

```rift
namespace some_math {
    func double(value: int) -> int {
        return value * 2
    }
}

using std

func main() {
    println(some_math.double(21))
}
```

With `using math`, the same function can be called as `double(21)`. Namespace
blocks can contain functions or nested namespaces. Nested function
names use dot qualification, such as `graphics.color.rgb(...)`; `using
graphics.color;` makes those functions available unqualified.

## Local variables and numeric types

Variables declared inside an `if`, `elif`, or `else` block are local to that
block. Inner blocks can shadow an outer variable; after the block ends, the
outer value is visible again. Prefix a declaration with `const` to prevent
later assignment.

```rift
using std

func main() {
    total: double = 1.5
    if true {
        const tax: decimal = 0.2
        total += tax
    }
    println(total)
}
```

`float` and `double` use their corresponding IEEE-754 binary precisions; each
operation rounds to the declared/promoted precision. Fractional literals infer
as `double`, and assigning or converting to `float` rounds to 32-bit precision.
Use `f` or `d` suffixes for typed literals, such as `1.25f` and `1.25d`.
`decimal` uses a signed 128-bit base-10 coefficient with up to 18 fractional
digits. Decimal addition, subtraction, multiplication, remainder, and integer
power are exact when the result fits that representation. Decimal division
truncates toward zero to at most 18 fractional digits. An operation that
exceeds the coefficient range or cannot retain its required fractional digits
reports an error.
Use the `m` suffix to preserve decimal literal digits directly, for example
`0.123456789012345678m`.

Mixed numeric operations promote in this order: decimal, double, float, then
integer. Integer operations use the widest operand width; plain integer
literals promote as `int` (`int32`). Results outside the promoted integer
range report overflow. Mixing negative values with unsigned values is an error.
Decimal operations convert binary floating-point operands through
their shortest round-trip decimal spelling. Floating-point `+=`, `-=`, `*=`,
`/=`, `%=`, `^=`, `++`, and `--` use the same arithmetic rules as expressions.
Division and remainder by zero are errors, and a non-finite floating-point
result is reported as overflow. Numeric-to-integer conversions truncate toward
zero and reject values outside the target integer range.

Integer runtime values retain their signedness and declared width. This also
applies when an integer is passed through an `any` parameter, so
`std.types.type_of(value)` and `std.types.size_of(value)` report its original
integer type. Untyped integer literals use the language's `int` default (32
bits); typed arithmetic keeps the widest operand width, and assigning a result
to a narrower type checks that value against the destination range.

Before `main` runs, Rift checks duplicate function and parameter names, call
targets and argument counts, argument and return type compatibility, invalid
returns, and whether non-void functions can fall through without returning. It
also validates `std.types`/`types` built-in names, argument counts, and numeric
conversion argument types during semantic analysis.
Calls and arithmetic involving `any` are checked at runtime because their
concrete types are not known statically. Runtime checks remain in place for
integer range limits and values that cannot be proven from their types.

## Type inference and `any`

Use `:=` to infer a local variable's type from its initial value. Integer
literals infer as `int`, fractional literals as `double`, and booleans, strings,
and characters infer their matching types. The inferred type remains fixed for
later assignments.

```rift
count := 1
ratio := 1.01

a, b: int = 1, 2
first, second: string
```

Function parameters and return types can use `any` when a function should accept
or return any Rift value:

```rift
func identity(value: any) -> any {
    return value
}
```

## `std.types`

The standard `std.types` module provides type sizes, numeric limits, string
conversion, checked integer conversions, floating-point conversions, and type
queries. Integer sizes are language-defined; `float`, `double`, and `decimal`
are reported as 4, 8, and 16 bytes. Those are language storage sizes; decimal
uses a 128-bit coefficient plus scale metadata in the interpreter. `sizeof` accepts either a type name or a
variable and reports sizes in bytes. Rift `string` values use UTF-16 storage;
`char` is one UTF-16 code unit. `min_value`/`max_value` also cover bool and char;
for string they report the minimum and maximum supported UTF-16 code-unit length.

```rift
using std;

func main() {
    value: int16 = 120
    println("{} bytes, max {}", std.types.size_of(value), std.types.max_value(int16))
    println(std.types.to_string(value))
    println(std.types.to_int8(12))
    println(std.types.type_of(value), " ", std.types.is_numeric(value))
}
```

Available conversions include `to_int8` through `to_int128`, `to_uint8` through
`to_uint128`, `to_int` (the `int32` alias), `to_uint` (the `uint32` alias),
`to_float`, `to_double`, and `to_decimal`. Integer conversions check the target
range; fractional inputs are truncated toward zero. Type queries include
`type_of` (also spelled `typeof`), `size_of` (also spelled `sizeof`),
`is_integer`, `is_numeric`, and `is_bool`.
