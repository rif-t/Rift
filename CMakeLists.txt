cmake_minimum_required(VERSION 3.20)

project(Rift
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(RIFT_CORE_SOURCES
    src/decimal.cpp
    src/fmt.cpp
    src/interpreter.cpp
    src/interpreter_expressions.cpp
    src/interpreter_values.cpp
    src/lexer.cpp
    src/parser.cpp
    src/parser_expressions.cpp
    src/semantic_analysis.cpp
)

set(RIFT_PUBLIC_HEADERS
    include/rift/ast.h
    include/rift/decimal.h
    include/rift/fmt.h
    include/rift/integer.h
    include/rift/interpreter.h
    include/rift/lexer.h
    include/rift/parser.h
    include/rift/semantic_analysis.h
    include/rift/utf.h
)

add_library(rift_core STATIC
    ${RIFT_CORE_SOURCES}
    ${RIFT_PUBLIC_HEADERS}
    src/interpreter_internal.h
)
target_include_directories(rift_core PUBLIC "${PROJECT_SOURCE_DIR}/include/rift")

add_executable(rift src/main.cpp)
target_link_libraries(rift PRIVATE rift_core)

enable_testing()
function(add_rift_test category)
    add_test(
        NAME "rift_${category}"
        COMMAND "${CMAKE_COMMAND}"
            "-DRIFT_EXECUTABLE=$<TARGET_FILE:rift>"
            "-DRIFT_TEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/tests/${category}"
            "-DRIFT_SOURCE_ROOT=${PROJECT_SOURCE_DIR}"
            -P "${PROJECT_SOURCE_DIR}/tests/${category}.cmake"
    )
    set_tests_properties("rift_${category}" PROPERTIES LABELS "${category}")
endfunction()

add_rift_test(lexer)
add_rift_test(parser)
add_rift_test(integer)
add_rift_test(interpreter)
add_rift_test(type_system)
add_rift_test(integration)

option(RIFT_BUILD_BENCHMARKS "Build Rift's profiling benchmarks" OFF)
if(RIFT_BUILD_BENCHMARKS)
    add_executable(rift_bench benchmarks/benchmark.cpp)
    target_link_libraries(rift_bench PRIVATE rift_core)
endif()
