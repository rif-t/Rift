include("${CMAKE_CURRENT_LIST_DIR}/test_helpers.cmake")

foreach(example IN ITEMS example-0 example-1 example-2 example-3 example-4 example-5)
    execute_process(
        COMMAND "${RIFT_EXECUTABLE}" "${RIFT_SOURCE_ROOT}/examples/${example}.rift"
        RESULT_VARIABLE actual_status
        OUTPUT_VARIABLE actual_stdout
        ERROR_VARIABLE actual_stderr
    )
    if(NOT "${actual_status}" STREQUAL "0")
        message(FATAL_ERROR "${example} failed with exit ${actual_status}; stdout: ${actual_stdout}; stderr: ${actual_stderr}")
    endif()
endforeach()
