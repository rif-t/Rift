# Namespaces

Namespaces group functions and may contain nested namespaces. Call a function
using its dot-qualified name, or use a namespace to call its functions
unqualified.

```rift
namespace math {
    func double(value: int) -> int {
        return value * 2
    }
}

func main() {
    println("{}", math.double(21))
}
```

Nested functions use dot qualification, such as `graphics.color.rgb(...)`.
With `using graphics.color`, its functions may be called by their short names.
If multiple imported namespaces provide the same short function name, qualify
the call to resolve the ambiguity.
