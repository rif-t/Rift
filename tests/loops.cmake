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
]=] 0 "3")

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
]=] 0 "6")

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
]=] 0 "normal 3")

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
]=] 0 "finished 3")

run_rift_case(for_skip_runs_post_without_counting_body [=[using std
using std
func main() {
    index := 0
    for index < 5; index++ {
        if index == 0 { loop.skip(2) }
        println("{}:{}", index, loop.iteration)
    }
}
]=] 0 "0:1\n3:2\n4:3")

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
]=] 0 "1\n1\n2\n3")

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
]=] 0 "1")

run_rift_case(for_rejects_unknown_loop_target [=[func main() {
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
]=] 0 "4")

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
]=] 0 "normal 3")

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
]=] 0 "finished 3")

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
]=] 0 "0")

run_rift_case(while_rejects_restart [=[func main() {
    while true { loop.restart }
}
]=] 1 "restart is only available for for loops")

run_rift_case(while_rejects_skip [=[func main() {
    while true { loop.skip(1) }
}
]=] 1 "skip is only available for for loops")

run_rift_case(while_rejects_non_boolean_condition [=[func main() {
    while 1 { }
}
]=] 1 "Loop condition must be bool")
