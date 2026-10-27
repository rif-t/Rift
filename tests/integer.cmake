include("${CMAKE_CURRENT_LIST_DIR}/test_helpers.cmake")

run_rift_case(conversion_boundaries [=[using std
using std.types
func main() {
    low: int128 = -170141183460469231731687303715884105728
    high: uint128 = 340282366920938463463374607431768211455
    println(to_int8(-128.9))
    println(to_uint8(255.9))
    println(to_int128(-170141183460469231731687303715884105728.0))
    println(low)
    println(high)
}
]=] 0 "-128\n255\n-170141183460469231731687303715884105728\n-170141183460469231731687303715884105728\n340282366920938463463374607431768211455\n")

run_rift_case_contains(conversion_overflow [=[using std
using std.types
func main() { println(to_uint128(340282366920938463463374607431768211456.0)) }
]=] 1 "outside the 128-bit integer range")

run_rift_case(unsigned_subtraction [=[using std
func main() {
    left: uint8 = 5
    right: uint8 = 3
    result: uint8 = left - right
    println(result)
}
]=] 0 "2\n")

run_rift_case_contains(unsigned_subtraction_underflow [=[using std
func main() {
    left: uint8 = 3
    right: uint8 = 5
    result: uint8 = left - right
    println(result)
}
]=] 1 "Unsigned integer underflow")

run_rift_case(unsigned_compound_decrement [=[using std
func main() {
    value: uint8 = 5
    value -= 1
    value--
    println(value)
}
]=] 0 "3\n")

run_rift_case_contains(unsigned_negation [=[using std
func main() {
    value: uint8 = 3
    println(-value)
}
]=] 1 "Unsigned integer values cannot be negated")

run_rift_case(uint64_maximum [=[using std
func main() {
    maximum: uint64 = 18446744073709551615
    println(maximum)
}
]=] 0 "18446744073709551615\n")

run_rift_case_contains(signed_width_overflow [=[using std
func main() {
    value: int8 = 127
    println(value + value)
}
]=] 1 "Integer overflow for int8")

run_rift_case(signed_subtract_minimum [=[using std
func main() {
    left: int8 = -5
    right: int8 = -128
    println(left - right)
}
]=] 0 "123\n")

run_rift_case(integer_literal_promotes_to_int [=[using std
func main() {
    value: int8 = 127
    println(value + 1)
}
]=] 0 "128\n")

run_rift_case_contains(signed_division_overflow [=[using std
func main() {
    minimum: int128 = -170141183460469231731687303715884105728
    println(minimum / -1)
}
]=] 1 "Integer overflow for int128")

run_rift_case_contains(unsigned_128_overflow [=[using std
func main() {
    maximum: uint128 = 340282366920938463463374607431768211455
    println(maximum + 1)
}
]=] 1 "128-bit integer overflow")
