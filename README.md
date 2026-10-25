# Rift

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
    print("The awesome {}", b, "with RIFT")
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

```rift
using std

func is_passing(score: int) -> bool {
    return score >= 50
}

func main() {
    println(is_passing(72) ? "pass" : "retry")
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
Namespace blocks can contain functions or nested namespaces. Nested function
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

`float`, `double`, and `decimal` accept fractional literals and arithmetic.
They currently use the same floating-point runtime representation; `decimal`
does not yet provide exact base-10 arithmetic.

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
are reported as 4, 8, and 16 bytes. `sizeof` accepts either a type name or a
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
