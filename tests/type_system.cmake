include("${CMAKE_CURRENT_LIST_DIR}/test_helpers.cmake")

run_rift_case(preserve_any_width [=[using std
using std.types
func inspect(value: any) {
    println(type_of(value))
    println(size_of(value))
}
func main() {
    value: int16 = 12
    inspect(value)
}
]=] 0 "int16\n2")

run_rift_case(validate_call_before_execution [=[using std
func takes_int(value: int) {
}
func main() {
    println("must not print")
    takes_int("wrong")
}
]=] 1 "must be int, got string")

run_rift_case(validate_return_type_before_execution [=[using std
func wrong() -> int {
    return "wrong"
}
func main() {
    println("must not print")
    wrong()
}
]=] 1 "Return value must be int, got string")

run_rift_case(validate_missing_return_before_execution [=[func answer() -> int {
    if true {
        return 1
    }
}
func main() {
}
]=] 1 "may complete without returning")

run_rift_case(numeric_mixed_and_type [=[using std
using std.types
func main() {
    value: double = 2 + 0.5
    value += 1
    println(value)
    println(type_of(to_int16(4)))
}
]=] 0 "3.5\nint16")

run_rift_case(floating_types [=[using std
using std.types
func main() {
    f: float = 16777216
    f += 1
    d: double = 9007199254740992
    d += 1
    println(type_of(f), " ", size_of(f), " ", f)
    println(type_of(d), " ", size_of(d), " ", d)
    inferred_float := 1.1f
    println(type_of(inferred_float), " ", size_of(inferred_float))
    a: decimal = to_decimal(0.1)
    b: decimal = to_decimal(0.2)
    println(type_of(a), " ", size_of(a), " ", a + b)
    precise: decimal = 0.123456789012345678m
    println(precise)
    decimal_limit: decimal = 170141183460469231731687303715884105727.0m
    println(decimal_limit == max_value(decimal))
    a += b
    println(a)
    println(to_decimal(0.02) < to_decimal(0.1))
    c: decimal = 1
    c /= 3
    println(c)
    println(to_int8(to_decimal(-12.9)))
    println(min_value(decimal) < max_value(decimal))
}
]=] 0 "float 4 16777216\ndouble 8 9007199254740992\nfloat 4\ndecimal 16 0.3\n0.123456789012345678\ntrue\n0.3\ntrue\n0.333333333333333333\n-12\ntrue")

run_rift_case(decimal_division [=[using std
using std.types
func main() {
    value: decimal = to_decimal(1) / to_decimal(7)
    println(value)
}
]=] 0 "0.142857142857142857")

run_rift_case(implicit_numeric_to_integer [=[using std
func main() {
    whole: int = 4.9
    negative: int64 = -4.9m
    byte: uint8 = 255.9
    println(whole, " ", negative, " ", byte)
}
]=] 0 "4 -4 255")

run_rift_case(implicit_integer_range_check [=[using std
func main() {
    value: int8 = 128.0
    println(value)
}
]=] 1 "outside the range of int8")

run_rift_case(const_assignment_rejected_before_execution [=[using std
func main() {
    const value: int = 1
    println("must not print")
    value = 2
}
]=] 1 "Cannot assign to const variable: value")

run_rift_case(incompatible_comparison_rejected [=[using std
func main() {
    println("must not print")
    value: bool = "one" == 1
}
]=] 1 "Comparison operands must have compatible types")

run_rift_case(invalid_compound_assignment_rejected [=[using std
func main() {
    text: string = "hello"
    println("must not print")
    text *= "!"
}
]=] 1 "Compound arithmetic assignment requires numeric operands")

run_rift_case(inferred_integer_width [=[using std
using std.types
func main() {
    original: uint64 = 5000000000
    copied := original
    println(type_of(copied), " ", copied)
}
]=] 0 "uint64 5000000000")

run_rift_case(conditional_numeric_promotion [=[using std
using std.types
func main() {
    integer_branch := true ? 1 : 2.5
    float_branch := false ? 1 : 2.5
    println(type_of(integer_branch), " ", integer_branch, " ", type_of(float_branch), " ", float_branch)
}
]=] 0 "double 1 double 2.5")

run_rift_case(negative_fraction_truncates_to_unsigned_zero [=[using std
using std.types
func main() {
    println(to_uint8(-0.5))
}
]=] 0 "0")

run_rift_case(any_boolean_operation [=[using std
func invert(value: any) -> bool {
    return !value
}
func main() {
    println(invert(true))
}
]=] 0 "false")

run_rift_case(any_boolean_runtime_check [=[using std
func invert(value: any) -> bool {
    return !value
}
func main() {
    println(invert(1))
}
]=] 1 "Logical negation operand must be bool")
