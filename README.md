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
