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
]=] 0 "int16\n2\n")

run_rift_case_contains(validate_call_before_execution [=[using std
func takes_int(value: int) {
}
func main() {
    println("must not print")
    takes_int("wrong")
}
]=] 1 "must be int, got string")

run_rift_case_contains(validate_return_type_before_execution [=[using std
func wrong() -> int {
    return "wrong"
}
func main() {
    println("must not print")
    wrong()
}
]=] 1 "Return value must be int, got string")

run_rift_case_contains(validate_missing_return_before_execution [=[func answer() -> int {
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
]=] 0 "3.5\nint16\n")

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
]=] 0 "float 4 16777216\ndouble 8 9007199254740992\nfloat 4\ndecimal 16 0.3\n0.123456789012345678\ntrue\n0.3\ntrue\n0.333333333333333333\n-12\ntrue\n")

run_rift_case(decimal_division [=[using std
using std.types
func main() {
    value: decimal = to_decimal(1) / to_decimal(7)
    println(value)
}
]=] 0 "0.142857142857142857\n")

run_rift_case(implicit_numeric_to_integer [=[using std
func main() {
    whole: int = 4.9
    negative: int64 = -4.9m
    byte: uint8 = 255.9
    println(whole, " ", negative, " ", byte)
}
]=] 0 "4 -4 255\n")

run_rift_case_contains(implicit_integer_range_check [=[using std
func main() {
    value: int8 = 128.0
    println(value)
}
]=] 1 "outside the range of int8")

run_rift_case_contains(const_assignment_rejected_before_execution [=[using std
func main() {
    const value: int = 1
    println("must not print")
    value = 2
}
]=] 1 "Cannot assign to const variable: value")

run_rift_case_contains(incompatible_comparison_rejected [=[using std
func main() {
    println("must not print")
    value: bool = "one" == 1
}
]=] 1 "Comparison operands must have compatible types")

run_rift_case_contains(invalid_compound_assignment_rejected [=[using std
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
]=] 0 "uint64 5000000000\n")

run_rift_case(conditional_numeric_promotion [=[using std
using std.types
func main() {
    integer_branch := true ? 1 : 2.5
    float_branch := false ? 1 : 2.5
    println(type_of(integer_branch), " ", integer_branch, " ", type_of(float_branch), " ", float_branch)
}
]=] 0 "double 1 double 2.5\n")

run_rift_case(negative_fraction_truncates_to_unsigned_zero [=[using std
using std.types
func main() {
    println(to_uint8(-0.5))
}
]=] 0 "0\n")

run_rift_case(any_boolean_operation [=[using std
func invert(value: any) -> bool {
    return !value
}
func main() {
    println(invert(true))
}
]=] 0 "false\n")

run_rift_case_contains(any_boolean_runtime_check [=[using std
func invert(value: any) -> bool {
    return !value
}
func main() {
    println(invert(1))
}
]=] 1 "Logical negation operand must be bool")

run_rift_case_contains(unknown_qualified_type_builtin [=[using std
func main() {
    println("must not print")
    std.types.not_a_function(1)
}
]=] 1 "Unknown std.types function: not_a_function")

run_rift_case_contains(unknown_types_builtin_via_std [=[using std
func main() {
    println("must not print")
    types.not_a_function(1)
}
]=] 1 "Unknown std.types function: not_a_function")

run_rift_case_contains(unknown_unqualified_builtin [=[using std
using std.types
func main() {
    println("must not print")
    not_a_function(1)
}
]=] 1 "Undefined function: not_a_function")

run_rift_case_contains(type_builtin_wrong_argument_count [=[using std
using std.types
func main() {
    println("must not print")
    size_of(int, int)
}
]=] 1 "std.types.size_of expects 1 argument(s)")

run_rift_case_contains(type_builtin_missing_argument [=[using std
using std.types
func main() {
    println("must not print")
    type_of()
}
]=] 1 "std.types.type_of expects 1 argument(s)")

run_rift_case_contains(type_builtin_incompatible_argument [=[using std
using std.types
func main() {
    println("must not print")
    to_int8("not a number")
}
]=] 1 "std.types.to_int8 expects a numeric argument, got string")

run_rift_case_contains(type_builtin_unsupported_type [=[using std
using std.types
func main() {
    println("must not print")
    size_of(any)
}
]=] 1 "std.types.size_of does not support type 'any'")

run_rift_case(explicit_builtin_routes_and_types [=[using std
func main() {
    value: int16 = 7
    type_name: string = std.types.type_of(value)
    println("{} {} {}", type_name, types.size_of(value), std.types.max_value(int16))
    from_type_name: string = std.types.type_of(uint64)
    println("{}", from_type_name)
}
]=] 0 "int16 2 32767\nuint64\n")

run_rift_case(decimal_minimum_boundary [=[using std
using std.types
func main() {
    minimum: decimal = min_value(decimal)
    maximum: decimal = max_value(decimal)
    println(minimum)
    println(maximum)
    println(minimum < maximum)
}
]=] 0 "-170141183460469231731687303715884105728\n170141183460469231731687303715884105727\ntrue\n")

run_rift_case(integer_edges_and_mixed_comparisons [=[using std
using std.types
func main() {
    signed_min: int8 = min_value(int8)
    signed_max: int8 = max_value(int8)
    unsigned_max: uint8 = max_value(uint8)
    as_float: float = signed_min
    as_double: double = unsigned_max
    println(signed_min, " ", signed_max, " ", unsigned_max)
    println(as_float, " ", as_double)
    println(signed_min < as_double)
}
]=] 0 "-128 127 255\n-128 255\ntrue\n")

run_rift_case(decimal_precision_and_truncation [=[using std
using std.types
func main() {
    exact: decimal = 0.123456789012345678m
    quotient: decimal = to_decimal(2) / to_decimal(3)
    negative_quotient: decimal = to_decimal(-2) / to_decimal(3)
    truncated: int = to_int(-0.9)
    println(exact)
    println(quotient)
    println(negative_quotient)
    println(truncated)
}
]=] 0 "0.123456789012345678\n0.666666666666666666\n-0.666666666666666666\n0\n")

run_rift_case(explicit_float_conversions [=[using std
using std.types
func main() {
    single: float = to_float(1)
    precise: double = to_double(2.5f)
    decimal_value: decimal = to_decimal(0.1)
    println(type_of(single), " ", single)
    println(type_of(precise), " ", precise)
    println(decimal_value)
}
]=] 0 "float 1\ndouble 2.5\n0.1\n")

run_rift_case_contains(void_builtin_argument_rejected [=[using std
using std.types
func nothing() {
}
func main() {
    println("must not print")
    size_of(nothing())
}
]=] 1 "std.types.size_of cannot inspect a void value")

run_rift_case_contains(decimal_overflow_is_language_error [=[using std
using std.types
func main() {
    value: decimal = max_value(decimal)
    value += 1
}
]=] 1 "Decimal")

run_rift_case_contains(float_overflow_is_language_error [=[using std
using std.types
func main() {
    value: float = max_value(float)
    value *= 2
}
]=] 1 "Floating-point overflow")

run_rift_case_contains(double_overflow_is_language_error [=[using std
using std.types
func main() {
    value: double = max_value(double)
    value *= 2
}
]=] 1 "Floating-point overflow")

run_rift_case_contains(float_conversion_overflow [=[using std
using std.types
func main() {
    value: float = to_float(340282366920938463463374607431768211455)
}
]=] 1 "outside the range of float")

run_rift_case(decimal_division_truncates_at_precision_limit [=[using std
using std.types
func main() {
    value: decimal = 0.000000000000000001m / 2
    println(value)
}
]=] 0 "500000000000000000\n")
