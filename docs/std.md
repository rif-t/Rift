# Standard library

Rift includes its `std` standard library. Standard library features are
available without installing a package. Add `using std` to call names directly,
or qualify them with `std.`.

```rift
using std

func main() {
    print("Hello ")
    println("{}", "Rift")
}
```

`print` writes without a trailing newline; `println` adds one. When the first
argument is a string, it is treated as a format string. Formatting fields use
`{}` for automatic argument indexing and support explicit indexes such as
`{0}`. Automatic and explicit indexing cannot be mixed.

```rift
using std

func main() {
    name := "Rift"
    println("Hello, {}!", name)
    println("{1} before {0}", "second", "first")
}
```

The standard library's type utilities are documented in [`std.types`](std-types.md).
