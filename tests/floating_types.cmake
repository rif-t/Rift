file(MAKE_DIRECTORY "${RIFT_TEST_DIR}")
set(source_path "${RIFT_TEST_DIR}/floating_types.rift")
file(WRITE "${source_path}" [=[using std
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
]=])
execute_process(COMMAND "${RIFT_EXECUTABLE}" "${source_path}"
    RESULT_VARIABLE actual_status OUTPUT_VARIABLE actual_output ERROR_VARIABLE actual_error)
if(NOT "${actual_status}" STREQUAL "0")
    message(FATAL_ERROR "floating types failed with exit ${actual_status}; stdout: ${actual_output}; stderr: ${actual_error}")
endif()
set(expected "float 4 16777216\ndouble 8 9007199254740992\nfloat 4\ndecimal 16 0.3\n0.123456789012345678\ntrue\n0.3\ntrue\n0.333333333333333333\n-12\ntrue")
if(NOT "${actual_output}" STREQUAL "${expected}\n")
    message(FATAL_ERROR "floating types output mismatch; expected [${expected}], got [${actual_output}]")
endif()

file(WRITE "${RIFT_TEST_DIR}/decimal_precision.rift" [=[using std
using std.types
func main() {
    value: decimal = to_decimal(1) / to_decimal(7)
    println(value)
}
]=])
execute_process(COMMAND "${RIFT_EXECUTABLE}" "${RIFT_TEST_DIR}/decimal_precision.rift"
    RESULT_VARIABLE division_status OUTPUT_VARIABLE division_output ERROR_VARIABLE division_error)
if(NOT "${division_status}" STREQUAL "0" OR NOT "${division_output}" STREQUAL "0.142857142857142857\n")
    message(FATAL_ERROR "decimal division failed: status ${division_status}; stdout: ${division_output}; stderr: ${division_error}")
endif()
