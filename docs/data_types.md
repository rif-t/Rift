# Data types

Rift has booleans, UTF-16 characters and strings, fixed-width integers, and
floating-point and decimal numbers.

## Types

| Type | Description |
| --- | --- |
| `bool` | `true` or `false` |
| `char` | One UTF-16 code unit |
| `string` | UTF-16 text |
| `int8`, `int16`, `int32`, `int64`, `int128` | Signed integers |
| `uint8`, `uint16`, `uint32`, `uint64`, `uint128` | Unsigned integers |
| `int` | Alias for `int32` |
| `float` | 32-bit IEEE-754 binary floating point |
| `double` | 64-bit IEEE-754 binary floating point |
| `decimal` | Signed 128-bit base-10 coefficient with scale metadata |
| `any` | A value whose concrete type is checked at runtime |

```rift
enabled: bool = true
letter: char = 'R'
message: string = "Hello, Rift"
small: int16 = 120
ratio: double = 1.25d
price: decimal = 12.50m
```

## Inference and assignment

Use `:=` to infer a local variable's type from its initializer. Integer literals
infer as `int`, fractional literals as `double`, and booleans, strings, and
characters infer as their corresponding types. The inferred type stays fixed
for later assignments.

```rift
count := 1
ratio := 1.01

a, b: int = 1, 2
first, second: string
```

`const` makes a variable immutable after initialization. Local variables are
scoped to their enclosing function or block.

## Numeric behavior

Use `f`, `d`, or `m` suffixes for explicitly typed fractional literals, such as
`1.25f`, `1.25d`, and `0.123456789012345678m`.

`float` and `double` use their IEEE-754 binary precisions; operations round to
the declared or promoted precision. `decimal` uses a signed 128-bit base-10
coefficient with up to 18 fractional digits. Decimal addition, subtraction,
multiplication, remainder, and integer powers are exact when the result fits.
Decimal division truncates toward zero to at most 18 fractional digits.
Overflow or a result that cannot retain its required fractional digits is an
error.

Mixed numeric operations promote in this order: `decimal`, `double`, `float`,
then integer. Integer operations use the widest operand width; plain integer
literals use `int` (`int32`). Results outside the promoted integer range are
errors. Mixing negative values with unsigned values is an error. Decimal
operations convert floating-point operands through their shortest round-trip
decimal spelling.

Numeric compound assignments and increment/decrement use the same arithmetic
rules as expressions. Division and remainder by zero are errors. Numeric to
integer conversions truncate toward zero and reject values outside the target
range. See [`std.types`](std-types.md) for checked conversions and numeric
limits.

Integer runtime values retain their signedness and declared width, including
when passed through an `any` parameter.
