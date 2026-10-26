file(MAKE_DIRECTORY "${RIFT_TEST_DIR}")

function(run_rift_case name source expected_status expected_output)
    set(source_path "${RIFT_TEST_DIR}/${name}.rift")
    file(WRITE "${source_path}" "${source}")
    execute_process(
        COMMAND "${RIFT_EXECUTABLE}" "${source_path}"
        RESULT_VARIABLE actual_status
        OUTPUT_VARIABLE actual_stdout
        ERROR_VARIABLE actual_stderr
    )
    if(NOT "${actual_status}" STREQUAL "${expected_status}")
        message(FATAL_ERROR "${name}: expected exit ${expected_status}, got ${actual_status}; stderr: ${actual_stderr}")
    endif()
    if(expected_status STREQUAL "0")
        set(actual_output "${actual_stdout}")
    else()
        set(actual_output "${actual_stderr}")
    endif()
    string(FIND "${actual_output}" "${expected_output}" match_at)
    if(match_at EQUAL -1)
        message(FATAL_ERROR "${name}: expected output to contain [${expected_output}], got [${actual_output}]")
    endif()
endfunction()

run_rift_case(unknown_character [=[func main() {
  @
}
]=] 1 "Unexpected character '@' at line 2, column 3")

run_rift_case(floating_compound [=[using std
func main() {
    value: double = 5.5
    value += 1.25
    println(value)
    value -= 0.25
    value *= 2
    value /= 2
    value %= 2
    value ^= 2
    value++
    value--
    println(value)
}
]=] 0 "6.75\n0.25")

run_rift_case(missing_brace [=[func main() {
]=] 1 "Expected '}', found end of file at line 2, column 1")

run_rift_case(missing_expression [=[func main() {
    value: int =
}
]=] 1 "Expected expression at line 3, column 1")

run_rift_case(branch_return [=[using std
func answer() -> int {
    if true {
        if true {
            return 42
        }
    }
    return 0
}
func main() { println(answer()) }
]=] 0 "42")

run_rift_case(branch_runtime_error [=[func main() {
    if true {
        if true {
            value: int = 1 / 0
        }
    }
}
]=] 1 "Division by zero")

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
]=] 0 "-128\n255\n-170141183460469231731687303715884105728")

run_rift_case(conversion_overflow [=[using std
using std.types
func main() { println(to_uint128(340282366920938463463374607431768211456.0)) }
]=] 1 "outside the 128-bit integer range")
