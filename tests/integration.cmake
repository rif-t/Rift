file(MAKE_DIRECTORY "${RIFT_TEST_DIR}")

function(normalize_newlines output_variable input_value)
    string(REPLACE "\r\n" "\n" normalized "${input_value}")
    string(REPLACE "\r" "\n" normalized "${normalized}")
    set(${output_variable} "${normalized}" PARENT_SCOPE)
endfunction()

function(run_successful_example example expected_output)
    execute_process(
        COMMAND "${RIFT_EXECUTABLE}" "${RIFT_SOURCE_ROOT}/examples/${example}.rift"
        WORKING_DIRECTORY "${RIFT_TEST_DIR}"
        RESULT_VARIABLE actual_status
        OUTPUT_VARIABLE actual_stdout
        ERROR_VARIABLE actual_stderr
    )
    normalize_newlines(actual_output "${actual_stdout}")
    normalize_newlines(expected_normalized "${expected_output}")
    if(NOT "${actual_status}" STREQUAL "0" OR NOT "${actual_output}" STREQUAL "${expected_normalized}" OR NOT "${actual_stderr}" STREQUAL "")
        message(FATAL_ERROR
            "${example}: expected exit 0 and stdout [${expected_normalized}], got exit ${actual_status} and stdout [${actual_output}]; stderr: [${actual_stderr}]")
    endif()
endfunction()

run_successful_example(example-0 [=[Hello World!]=])
run_successful_example(example-1 [=[2
7
-8
4
1
0
]=])
run_successful_example(example-2 [=[1 is lesser then 2
Odd
Zero
]=])
run_successful_example(example-3 [=[The awesome program with RIFT
2+2*2=6]=])
run_successful_example(example-4 [=[4
false true
200
]=])
run_successful_example(example-5 [=[Type: int8, size in bits: 8, min value: -128, max value: 127
Type: uint8, size in bits: 8, min value: 0, max value: 255
Type: int16, size in bits: 16, min value: -32768, max value: 32767
Type: uint16, size in bits: 16, min value: 0, max value: 65535
Type: int32, size in bits: 32, min value: -2147483648, max value: 2147483647
Type: uint32, size in bits: 32, min value: 0, max value: 4294967295
Type: int64, size in bits: 64, min value: -9223372036854775808, max value: 9223372036854775807
Type: uint64, size in bits: 64, min value: 0, max value: 18446744073709551615
Type: int128, size in bits: 128, min value: -170141183460469231731687303715884105728, max value: 170141183460469231731687303715884105727
Type: uint128, size in bits: 128, min value: 0, max value: 340282366920938463463374607431768211455
Type: float, size in bits: 32, min value: -3.40282347e+38, max value: 3.40282347e+38
Type: double, size in bits: 64, min value: -1.7976931348623157e+308, max value: 1.7976931348623157e+308
Type: decimal, size in bits: 128, min value: -170141183460469231731687303715884105728, max value: 170141183460469231731687303715884105727
Type: bool, size in bits: 8, min value: false, max value: true
Type: char, size in bits: 16, min value: 0, max value: 65535
Type: string, size in bits: 64, min value: 0, max value: 4294967295
]=])

set(failing_example "${RIFT_TEST_DIR}/intentional-failure.rift")
file(WRITE "${failing_example}" [=[using std
using std.types
func main() {
    println("must not print")
    std.types.does_not_exist(1)
}
]=])
execute_process(
    COMMAND "${RIFT_EXECUTABLE}" "${failing_example}"
    WORKING_DIRECTORY "${RIFT_TEST_DIR}"
    RESULT_VARIABLE failure_status
    OUTPUT_VARIABLE failure_stdout
    ERROR_VARIABLE failure_stderr
)
normalize_newlines(failure_stdout_normalized "${failure_stdout}")
normalize_newlines(failure_stderr_normalized "${failure_stderr}")
set(expected_failure_stderr "Error: Unknown std.types function: does_not_exist\n")
if(NOT "${failure_status}" STREQUAL "1" OR NOT "${failure_stdout_normalized}" STREQUAL "" OR
   NOT "${failure_stderr_normalized}" STREQUAL "${expected_failure_stderr}")
    message(FATAL_ERROR
        "intentional-failure: expected exit 1, stdout [], stderr [${expected_failure_stderr}]; got exit ${failure_status}, stdout [${failure_stdout_normalized}], stderr [${failure_stderr_normalized}]")
endif()
