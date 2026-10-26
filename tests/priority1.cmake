file(MAKE_DIRECTORY "${RIFT_TEST_DIR}")

function(run_rift_case name source expected_status expected_output)
    set(source_path "${RIFT_TEST_DIR}/${name}.rift")
    file(WRITE "${source_path}" "${source}")
    execute_process(COMMAND "${RIFT_EXECUTABLE}" "${source_path}"
        RESULT_VARIABLE actual_status OUTPUT_VARIABLE actual_stdout ERROR_VARIABLE actual_stderr)
    if(NOT "${actual_status}" STREQUAL "${expected_status}")
        message(FATAL_ERROR "${name}: expected exit ${expected_status}, got ${actual_status}; stdout: ${actual_stdout}; stderr: ${actual_stderr}")
    endif()
    if(expected_status STREQUAL "0")
        set(actual_output "${actual_stdout}")
    else()
        set(actual_output "${actual_stderr}")
        if(NOT actual_stdout STREQUAL "")
            message(FATAL_ERROR "${name}: validation happened after program output: [${actual_stdout}]")
        endif()
    endif()
    string(FIND "${actual_output}" "${expected_output}" match_at)
    if(match_at EQUAL -1)
        message(FATAL_ERROR "${name}: expected output to contain [${expected_output}], got [${actual_output}]")
    endif()
endfunction()

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
