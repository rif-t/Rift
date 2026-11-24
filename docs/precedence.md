# Expression precedence

From highest to lowest, Rift parses expressions in this order:

1. Parentheses, literals, identifiers, and calls
2. Power (`^` or `**`), right associative
3. Unary minus (`-`) and logical negation (`!`)
4. Multiplication, division, and remainder (`*`, `/`, `%`)
5. Addition and subtraction (`+`, `-`)
6. Comparisons (`<`, `<=`, `>`, `>=`)
7. Equality (`==`, `!=`)
8. Logical AND (`&&`)
9. Logical OR (`||`)
10. Conditional expressions (`? :`)

Power binds more tightly than unary minus, so `-2^2` means `-(2^2)` and
evaluates to `-4`. Parentheses make the other grouping explicit: `(-2)^2`
evaluates to `4`. Power associates right to left, so `2^3^2` means `2^(3^2)`.
Unary operators are also permitted on a power's exponent, so `2^-2` means
`2^(-2)`. The spellings `^` and `**` have the same precedence and behavior.

This changes the old grouping of expressions such as `-2^2`; use parentheses
where you need to preserve the former `(-2)^2` meaning.
