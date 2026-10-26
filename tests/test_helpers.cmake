function(run_rift_case name source expected_status expected_output)
    set(source_path "${RIFT_TEST_DIR}/${name}.rift")
    file(MAKE_DIRECTORY "${RIFT_TEST_DIR}")
    file(WRITE "${source_path}" "${source}")
    execute_process(
        COMMAND "${RIFT_EXECUTABLE}" "${source_path}"
        RESULT_VARIABLE actual_status
        OUTPUT_VARIABLE actual_stdout
        ERROR_VARIABLE actual_stderr
    )
    if(NOT "${actual_status}" STREQUAL "${expected_status}")
        message(FATAL_ERROR "${name}: expected exit ${expected_status}, got ${actual_status}; stdout: ${actual_stdout}; stderr: ${actual_stderr}")
    endif()
    if(expected_status STREQUAL "0")
        set(actual_output "${actual_stdout}")
    else()
        set(actual_output "${actual_stderr}")
        if(NOT actual_stdout STREQUAL "")
            message(FATAL_ERROR "${name}: expected an error before program output, got [${actual_stdout}]")
        endif()
    endif()
    string(FIND "${actual_output}" "${expected_output}" match_at)
    if(match_at EQUAL -1)
        message(FATAL_ERROR "${name}: expected output to contain [${expected_output}], got [${actual_output}]")
    endif()
endfunction()
