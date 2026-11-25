include("${CMAKE_CURRENT_LIST_DIR}/test_helpers.cmake")

run_rift_case(arrays_literals_indexing_copy_and_functions [=[using std
using std.types
func edit(values: int[]) -> int[] {
    values[0] = 99
    return values
}
func main() {
    values := [1, 2, 3]
    values[0] = 4
    values.append(5)
    copy := values
    copy[1] = 9
    result := edit(values)
    println("{} {} {} {} {}", values.length, type_of(values), size_of(values), type_of(int[][]), size_of(int[]))
    println(values)
    println(copy)
    println(result)
}
]=] 0 "4 int[] 8 int[][] 8\n[4, 2, 3, 5]\n[4, 9, 3, 5]\n[99, 2, 3, 5]\n")

run_rift_case(arrays_nested_and_empty [=[using std
func main() {
    values: int[][] = [[1, 2], []]
    values[0][1] = 7
    values[1] = [8]
    empty: int[] = []
    println(values)
    println(empty.length)
}
]=] 0 "[[1, 7], [8]]\n0\n")

run_rift_case(arrays_copy_on_write_values [=[using std
func edit(values: int[][]) -> int[][] {
    values[0][1] = 90
    values.append([7])
    return values
}
func main() {
    original: int[][] = [[1, 2], [3, 4]]
    assigned := original
    assigned[0][0] = 8
    returned := edit(original)
    returned[1][0] = 6
    println(original)
    println(assigned)
    println(returned)
}
]=] 0 "[[1, 2], [3, 4]]\n[[8, 2], [3, 4]]\n[[1, 90], [6, 4], [7]]\n")

run_rift_case(arrays_copy_on_write_deep_detach [=[using std
func main() {
    values: int[][][][] = [[[[1]]]]
    copy := values
    copy[0][0][0][0] = 9
    println(values)
    println(copy)
}
]=] 0 "[[[[1]]]]\n[[[[9]]]]\n")

run_rift_case(arrays_numeric_promotion_and_any [=[using std
func main() {
    values := [1, 2.5]
    mixed: any[] = []
    mixed.append(1)
    mixed.append("two")
    mixed.append(true)
    println("{} {}", types.type_of(values), values)
    println(mixed)
}
]=] 0 "double[] [1, 2.5]\n[1, two, true]\n")

run_rift_case_contains(array_index_out_of_range [=[using std
func main() {
    values := [1]
    println(values[1])
}
]=] 1 "Array index out of range")

run_rift_case_contains(array_negative_index [=[using std
func main() {
    values := [1]
    println(values[-1])
}
]=] 1 "Array index must be a non-negative integer no larger than uint64")

run_rift_case_contains(array_rejects_incompatible_element [=[func main() {
    values := [1, 2]
    values[0] = "wrong"
}
]=] 1 "Cannot assign string to array element int")

run_rift_case_contains(array_rejects_empty_inference [=[func main() {
    values := []
}
]=] 1 "Empty array literals need an explicit array type")

run_rift_case_contains(array_rejects_const_append [=[func main() {
    const values: int[] = [1]
    values.append(2)
}
]=] 1 "Cannot mutate const array")

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
]=] 0 "6.75\n0.25\n")

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
]=] 0 "42\n")

run_rift_case_contains(branch_runtime_error [=[func main() {
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
]=] 0 "second, first, second\n")

run_rift_case_contains(format_mixed_indexing [=[using std
func main() {
    println("{0} {}", "value", "other")
}
]=] 1 "Cannot mix automatic and manual argument indexing")

run_rift_case_contains(format_invalid_index [=[using std
func main() {
    println("{1x}", "value", "other")
}
]=] 1 "Invalid field in format string")

run_rift_case_contains(format_unused_argument [=[using std
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
]=] 0 "Rift! language\n")

run_rift_case(function_overloads_exact_and_fallback [=[using std
func describe(value: int) -> string { return "integer" }
func describe(value: string) -> string { return "text" }
func describe(value: any) -> string { return "other" }
func main() {
    println("{} {} {}", describe(42), describe("rift"), describe(true))
}
]=] 0 "integer text other\n")

run_rift_case_contains(function_overload_ambiguous_conversion [=[using std
func choose(value: int64) -> string { return "wide integer" }
func choose(value: double) -> string { return "floating point" }
func main() {
    println("must not execute")
    println(choose(42))
}
]=] 1 "Ambiguous overload for function 'choose'")

run_rift_case_contains(function_overload_duplicate_signature [=[func same(first: int) -> int { return first }
func same(second: int32) -> int { return second }
func main() {}
]=] 1 "Duplicate function overload: same")

run_rift_case(indexed_local_shadowing_and_recursion [=[using std
func sum(n: int, total: int) -> int {
    if n == 0 { return total }
    local := n
    if true { local := local + 1 }
    return sum(n - 1, total + local - 1)
}
func main() {
    value := 42
    for value := 0; value < 3; value++ {
        item := value
        if true { item := item + 1 }
    }
    println("{} {}", value, sum(3, 0))
}
]=] 0 "42 3\n")

run_rift_case(for_loop_counting [=[using std
func main() {
    total := 0
    for i := 0; i < 10; i++ {
        total += i
    }
    println("{}", total)
}
]=] 0 "45\n")

run_rift_case(for_loop_named_break [=[using std
func main() {
    count := 0
    loop : for j:double=3.0d, j>=0.0d; j-=1.0d {
        count++
        for k := 0; k < 3; k++ {
            break loop
        }
    }
    println("{}", count)
}
]=] 0 "1\n")

run_rift_case(for_loop_unlabeled_break [=[using std
func main() {
    count := 0
    for i := 0; true; i++ {
        if i == 3 { break }
        count++
    }
    println("{}", count)
}
]=] 0 "3\n")

run_rift_case_contains(break_outside_loop [=[func main() {
    break
}
]=] 1 "break can only be used inside a loop")

run_rift_case(while_loop_with_limit_and_else [=[using std
func main() {
    value := 0
    named: while value < 10 {
        if named.iteration == 2 { named.limit = 3 }
        value++
    } else {
        println("normal {}", value)
    }
}
]=] 0 "normal 3\n")

run_rift_case(while_loop_continue_and_break_if [=[using std
func main() {
    value := 0
    while value < 5 {
        value++
        if value == 2 { loop.continue }
        loop.break_if(value == 4)
    } else {
        println("should not run")
    }
    println("value {}", value)
}
]=] 0 "value 4\n")

run_rift_case_contains(while_loop_rejects_restart [=[func main() {
    while true { loop.restart }
}
]=] 1 "restart is only available for for loops")

run_rift_case_contains(while_loop_rejects_skip [=[func main() {
    while true { loop.skip(1) }
}
]=] 1 "skip is only available for for loops")
