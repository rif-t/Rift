# Rift

Rift is an experimental C-like programming language.

## Standard library

Rift ships its `std` standard library. Use `using std;` to make its names
available directly, or qualify calls with `std.` when you prefer not to add a
using directive. No package installation is needed.

## Your first program with RIFT

```rift
using std;

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
using std;

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
using std;

func is_passing(score: int) -> bool {
    return score >= 50
}

func main() {
    println(is_passing(72) ? "pass" : "retry")
}
```
