# Conditions and boolean expressions

Comparisons (`<`, `>`, `<=`, `>=`, `==`, `!=`) produce `bool` values. Boolean
logic uses `&&`, `||`, `!` or the word operators `and`, `or`, `not`. `&&` and
`||` short-circuit.

Conditions may be written with or without parentheses. Rift supports `if`,
`elif`, and `else` blocks, plus conditional expressions using
`condition ? value : otherValue`.

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

Boolean variables can be initialized or assigned from comparisons. Conditions
must evaluate to `bool` (or `any`, which is checked at runtime).
