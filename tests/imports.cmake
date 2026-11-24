include("${CMAKE_CURRENT_LIST_DIR}/test_helpers.cmake")

file(MAKE_DIRECTORY "${RIFT_TEST_DIR}/nested")
file(WRITE "${RIFT_TEST_DIR}/math.rift" [=[namespace math {
    func add(a: int, b: int) -> int { return a + b }
    func twice(value: int) -> int { return math.add(value, value) }
}
]=])
run_rift_case(default_namespace [=[using std
import "math"
func main() { println(math.twice(21)) }
]=] 0 "42\n")

run_rift_case(alias_namespace [=[using std
import smth "math"
func main() { println(smth.twice(21)) }
]=] 0 "42\n")

run_rift_case(using_imported_namespace [=[using std
import smth "math"
using smth
func main() { println(twice(21)) }
]=] 0 "42\n")

file(WRITE "${RIFT_TEST_DIR}/nested/shared.rift" [=[namespace shared {
    func value() -> int { return 42 }
}
]=])
file(WRITE "${RIFT_TEST_DIR}/nested/math.rift" [=[import "shared"
namespace math {
    func value() -> int { return shared.value() }
}
]=])
run_rift_case(nested_relative_import [=[using std
import trig "nested/math"
func main() { println(trig.value()) }
]=] 0 "42\n")

run_rift_case(duplicate_import [=[using std
import "math"
import "math"
func main() { println(math.add(19, 23)) }
]=] 0 "42\n")

run_rift_case_contains(missing_import [=[import "missing"
func main() {}
]=] 1 "Could not resolve import 'missing'")

file(WRITE "${RIFT_TEST_DIR}/multiple_namespaces.rift" [=[namespace first {
    func one() -> int { return 1 }
    namespace nested { func two() -> int { return 2 } }
}
namespace second { func three() -> int { return 3 } }
]=])
run_rift_case(multiple_and_nested_namespaces [=[using std
import "multiple_namespaces"
func main() { println(first.one() + first.nested.two() + second.three()) }
]=] 0 "6\n")

file(WRITE "${RIFT_TEST_DIR}/wrong_name.rift" [=[namespace actual { func value() -> int { return 42 } }
]=])
run_rift_case(namespace_name_independent_of_file_name [=[using std
import "wrong_name"
func main() { println(actual.value()) }
]=] 0 "42\n")

run_rift_case(alias_multiple_namespaces [=[using std
import bundle "multiple_namespaces"
func main() { println(bundle.first.one() + bundle.first.nested.two() + bundle.second.three()) }
]=] 0 "6\n")

file(WRITE "${RIFT_TEST_DIR}/cycle_a.rift" [=[import "cycle_b"
namespace cycle_a { func a() {} }
]=])
file(WRITE "${RIFT_TEST_DIR}/cycle_b.rift" [=[import "cycle_a"
namespace cycle_b { func b() {} }
]=])
run_rift_case_contains(import_cycle [=[import "cycle_a"
func main() {}
]=] 1 "Import cycle")

file(WRITE "${RIFT_TEST_DIR}/module_main.rift" [=[namespace module_main { func main() {} }
]=])
run_rift_case_contains(imported_main [=[import "module_main"
func main() {}
]=] 1 "cannot declare main")

run_rift_case(single_file_still_works [=[using std
func main() { println("ok") }
]=] 0 "ok\n")
