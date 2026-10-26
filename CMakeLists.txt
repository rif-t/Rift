cmake_minimum_required(VERSION 3.20)

project(Rift
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE RIFT_FILES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/include/rift/*.h"
)

add_executable(rift ${RIFT_FILES})

target_include_directories(rift
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include/rift
)

enable_testing()
add_test(
    NAME rift_priority0_regressions
    COMMAND ${CMAKE_COMMAND}
        "-DRIFT_EXECUTABLE=$<TARGET_FILE:rift>"
        "-DRIFT_TEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/tests"
        -P "${PROJECT_SOURCE_DIR}/tests/priority0.cmake"
)
add_test(
    NAME rift_priority1_regressions
    COMMAND ${CMAKE_COMMAND}
        "-DRIFT_EXECUTABLE=$<TARGET_FILE:rift>"
        "-DRIFT_TEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/tests"
        -P "${PROJECT_SOURCE_DIR}/tests/priority1.cmake"
)
add_test(
    NAME rift_floating_type_regressions
    COMMAND ${CMAKE_COMMAND}
        "-DRIFT_EXECUTABLE=$<TARGET_FILE:rift>"
        "-DRIFT_TEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/tests"
        -P "${PROJECT_SOURCE_DIR}/tests/floating_types.cmake"
)
