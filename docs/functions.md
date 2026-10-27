# Functions

Functions have typed parameters and may declare a return type with `->`.
Calls can appear in expressions.

```rift
func add(left: int, right: int) -> int {
    return left + right
}

func main() {
    println("{}", add(2, 3))
}
```

Parameters and return types may use `any` when the function should accept or
return any Rift value:

```rift
func identity(value: any) -> any {
    return value
}
```

## Overloads

Functions may share a name when their parameter type signatures differ. Names
of parameters and return types do not distinguish overloads. Aliases such as
`int` and `int32` count as the same type in a signature.

Rift prefers exact argument type matches, then considers numeric conversions
and `any` parameters. If multiple overloads are equally good matches, semantic
analysis reports an ambiguity. Numeric conversions are allowed by the type
system, so a call that could convert to multiple overloads may be ambiguous.

```rift
func label(value: int) -> string { return "integer" }
func label(value: string) -> string { return "text" }

func main() {
    println("{} {}", label(42), label("rift"))
}
```

`main` must have no parameters and cannot be overloaded. Rift checks function
declarations, call targets, argument counts and types, return statements, and
non-void return paths during semantic analysis before executing `main`.
