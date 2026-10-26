include("${CMAKE_CURRENT_LIST_DIR}/test_helpers.cmake")

run_rift_case(missing_brace [=[func main() {
]=] 1 "Expected '}', found end of file at line 2, column 1")

run_rift_case(missing_expression [=[func main() {
    value: int =
}
]=] 1 "Expected expression at line 3, column 1")

run_rift_case(missing_parenthesis [=[func main() {
    if (true {
    }
}
]=] 1 "Expected ')', found '{'")

run_rift_case(truncated_declaration [=[func main() {
    value: int =
]=] 1 "Expected expression at line 3")
