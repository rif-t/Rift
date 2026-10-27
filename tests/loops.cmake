include("${CMAKE_CURRENT_LIST_DIR}/test_helpers.cmake")

run_rift_case(for_no_header_clauses [=[using std
func main() {
    count := 0
    for {
        count++
        if count == 3 { loop.break }
    }
    println("{}", count)
}
]=] 0 "3\n")

run_rift_case(for_condition_without_post [=[using std
func main() {
    index := 0
    total := 0
    for index < 4 {
        total += index
        index++
    }
    println("{}", total)
}
]=] 0 "6\n")

run_rift_case(for_iteration_and_limit_else [=[using std
func main() {
    index := 0
    for index < 10 {
        if loop.iteration == 2 { loop.limit = 3 }
        index++
    } else {
        println("normal {}", index)
    }
}
]=] 0 "normal 3\n")

run_rift_case(for_break_skips_else [=[using std
func main() {
    for true {
        loop.break
    } else {
        println("must not execute")
    }
}
]=] 0 "")

run_rift_case(for_continue_runs_post_and_else [=[using std
func main() {
    index := 0
    for index < 3; index++ {
        loop.continue
    } else {
        println("finished {}", index)
    }
}
]=] 0 "finished 3\n")

run_rift_case(for_skip_runs_post_without_counting_body [=[using std
using std
func main() {
    index := 0
    for index < 5; index++ {
        if index == 0 { loop.skip(2) }
        println("{}:{}", index, loop.iteration)
    }
}
]=] 0 "0:1\n3:2\n4:3\n")

run_rift_case(for_restart_resets_iteration_without_initializer [=[using std
func main() {
    index := 0
    restarted := false
    for ; index < 4; index++ {
        if index == 1 && !restarted {
            restarted = true
            loop.restart
        }
        println("{}", loop.iteration)
    }
}
]=] 0 "1\n1\n2\n3\n")

run_rift_case(for_restart_clears_iteration_limit [=[using std
func main() {
    index := 0
    restarted := false
    for index < 5; index++ {
        if index == 1 && !restarted {
            restarted = true
            loop.limit = 1
            loop.restart
        }
        println("{}", loop.iteration)
    }
}
]=] 0 "1\n1\n2\n3\n4\n")

run_rift_case(for_named_break_exits_outer_loop [=[using std
func main() {
    count := 0
    outer: for i := 0; i < 5; i++ {
        for j := 0; j < 5; j++ {
            count++
            outer.break
        }
    }
    println("{}", count)
}
]=] 0 "1\n")

run_rift_case_contains(for_rejects_unknown_loop_target [=[func main() {
    for true { missing.break }
}
]=] 1 "Unknown active loop: missing")

run_rift_case(while_parenthesized_and_unparenthesized [=[using std
func main() {
    count := 0
    while (count < 2) { count++ }
    named: while count < 4 { count++ }
    println("{}", count)
}
]=] 0 "4\n")

run_rift_case(while_iteration_limit_and_else [=[using std
func main() {
    count := 0
    while count < 10 {
        if loop.iteration == 2 { loop.limit = 3 }
        count++
    } else {
        println("normal {}", count)
    }
}
]=] 0 "normal 3\n")

run_rift_case(while_continue_then_else [=[using std
func main() {
    count := 0
    while count < 3 {
        count++
        loop.continue
    } else {
        println("finished {}", count)
    }
}
]=] 0 "finished 3\n")

run_rift_case(while_named_break_exits_outer_loop [=[using std
func main() {
    count := 0
    outer: while true {
        while true { outer.break }
        count++
    } else {
        println("must not execute")
    }
    println("{}", count)
}
]=] 0 "0\n")

run_rift_case_contains(while_rejects_restart [=[func main() {
    while true { loop.restart }
}
]=] 1 "restart is only available for for loops")

run_rift_case_contains(while_rejects_skip [=[func main() {
    while true { loop.skip(1) }
}
]=] 1 "skip is only available for for loops")

run_rift_case_contains(while_rejects_non_boolean_condition [=[func main() {
    while 1 { }
}
]=] 1 "Loop condition must be bool")

run_rift_case_contains(while_any_condition_reports_while_context [=[func main() {
    condition: any = 1
    while condition { }
}
]=] 1 "while condition must be bool")

run_rift_case(for_post_break_is_handled_by_loop [=[using std
func main() {
    count := 0
    for i := 0; i < 5; loop.break {
        count++
    } else {
        println("must not execute")
    }
    println("{}", count)
}
]=] 0 "1\n")

run_rift_case(for_post_restart_resets_iteration_and_suppresses_else [=[using std
func main() {
    index := 0
    restarted := false
    for index < 3; loop.restart {
        index++
        if index == 1 && !restarted {
            restarted = true
            index = 3
        }
    } else {
        println("must not execute")
    }
    println("{}", index)
}
]=] 0 "3\n")

run_rift_case(for_dynamic_limit_zero_stops_normally [=[using std
func main() {
    count := 0
    for true {
        count++
        loop.limit = 0
    } else {
        println("normal {}", count)
    }
}
]=] 0 "normal 1\n")

run_rift_case(while_dynamic_limit_reached_normally [=[using std
func main() {
    count := 0
    while count < 10 {
        if count == 0 { loop.limit = 2 }
        count++
} else {
        println("normal {}", count)
    }
}
]=] 0 "normal 2\n")

run_rift_case(for_zero_iterations_runs_else [=[using std
func main() {
    for index := 0; index < 0; index++ {
        println("must not execute")
    } else {
        println("empty")
    }
}
]=] 0 "empty\n")

run_rift_case(for_skip_zero_and_break_if_false [=[using std
func main() {
    count := 0
    for count < 2; count++ {
        loop.skip(0)
        loop.break_if(false)
        println("{}", count)
    }
}
]=] 0 "0\n1\n")

run_rift_case_contains(for_negative_limit_is_runtime_error [=[func main() {
    for true {
        loop.limit = -1
    }
}
]=] 1 "Loop limit must be a non-negative integer no larger than uint64")

run_rift_case_contains(for_negative_skip_is_runtime_error [=[func main() {
    for true { loop.skip(-1) }
}
]=] 1 "loop.skip count must be a non-negative integer no larger than uint64")

run_rift_case(for_named_continue_targets_outer_loop [=[using std
func main() {
    count := 0
    outer: for i := 0; i < 3; i++ {
        for j := 0; j < 3; j++ {
            count++
            outer.continue
        }
    }
    println("{}", count)
}
]=] 0 "3\n")
