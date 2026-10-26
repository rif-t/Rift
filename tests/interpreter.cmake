include("${CMAKE_CURRENT_LIST_DIR}/test_helpers.cmake")

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

run_rift_case(format_explicit_index [=[using std
func main() {
    println("{1}, {0}, {1}", "first", "second")
}
]=] 0 "second, first, second")

run_rift_case(format_mixed_indexing [=[using std
func main() {
    println("{0} {}", "value", "other")
}
]=] 1 "Cannot mix automatic and manual argument indexing")

run_rift_case(format_invalid_index [=[using std
func main() {
    println("{1x}", "value", "other")
}
]=] 1 "Invalid field in format string")

run_rift_case(format_unused_argument [=[using std
func main() {
    println("{}", "used", "unused")
}
]=] 1 "Too many arguments for format string")

run_rift_case(string_compound_concatenation [=[using std
func main() {
    text: string = "Rift"
    text += '!'
    text += " language"
    println(text)
}
]=] 0 "Rift! language")
