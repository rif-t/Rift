# `std.types`

The `std.types` module provides type sizes, numeric limits, conversions, and
type queries. It follows the normal namespace rules:

- Without a using directive, qualify names as `std.types.size_of(...)`.
- With `using std`, use `types.size_of(...)` or `std.types.size_of(...)`.
- With `using std.types`, use `size_of(...)` directly.

## Size and limits

`size_of` (also `sizeof`) accepts a type name or a value and reports bytes.
`min_value` and `max_value` accept a type name or value. Supported numeric
types include the signed and unsigned integer widths, `float`, `double`, and
`decimal`. Boolean and character limits are also available. For `string`, the
limits describe supported UTF-16 code-unit length.

```rift
using std

func main() {
    value: int16 = 120
    println("{} bytes", types.size_of(value))
    println("{}..{}", types.min_value(int16), types.max_value(int16))
}
```

Floating-point sizes are reported as 4 bytes for `float` and 8 bytes for
`double`; decimal reports 16 bytes for its coefficient representation.

## Conversion and queries

Available conversions are `to_int8` through `to_int128`, `to_uint8` through
`to_uint128`, `to_int` (the `int32` alias), `to_uint` (the `uint32` alias),
`to_float`, `to_double`, and `to_decimal`. Integer conversions check the
destination range; fractional inputs are truncated toward zero.

Type queries are `type_of` (also `typeof`), `is_integer`, `is_numeric`, and
`is_bool`. Integer values retain their signedness and declared width, including
when passed through an `any` parameter.

```rift
using std

func main() {
    value: int16 = 120
    println("{}", types.to_string(value))
    println("{}", types.to_int8(12))
    println("{} {}", types.type_of(value), types.is_numeric(value))
}
```
