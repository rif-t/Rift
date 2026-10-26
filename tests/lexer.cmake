include("${CMAKE_CURRENT_LIST_DIR}/test_helpers.cmake")

run_rift_case(unknown_character [=[func main() {
  @
}
]=] 1 "Unexpected character '@' at line 2, column 3")
