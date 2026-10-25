# Rift

Rift is an experimental C-like programming language.

## Your first program with RIFT

```rift
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
