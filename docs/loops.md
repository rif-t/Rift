# For loops

Rift `for` loops support an initializer, condition, and post statement. The
initializer can declare a variable scoped to the loop. Parentheses around the
header are optional.

```rift
for i := 0; i < 10; i++ {
    println("{}", i)
}

for (j:double = 100.0d, j >= 0.0d; j -= 10.0d) {
    println("{}", j)
}
```

## Labels and control

A label before `for` names that loop. `loop` refers to the innermost active
loop; a label refers to that specific enclosing loop.

```rift
outer: for i := 0; i < 10; i++ {
    for j := 0; j < 10; j++ {
        if i == j {
            outer.break
        }
    }
}
```

The following controls are available on `loop` or a named loop:

- `.iteration` reads the number of body entries. It starts at `1` and is
  read-only. Skipped bodies do not increment it.
- `.limit = n` sets a writable maximum number of body executions. There is no
  limit by default. Reaching it ends the loop normally without another body
  execution. Reading `.limit` before setting it raises a runtime error.
- `.break` exits the target loop. `break` and `break name` remain supported.
- `.break_if(condition)` exits only when its boolean argument is true.
- `.continue` skips the rest of the current body and runs the post statement.
- `.restart` resumes at the next condition check without rerunning the
  initializer. It resets the iteration count. Restarting suppresses the loop's
  `else` block.
- `loop.skip(n)` skips the next `n` body executions. Each skipped iteration
  still checks the loop condition and runs the post statement.

For example:

```rift
outer: for i := 0; i < 100; i++ {
    if i == 5 {
        outer.limit = 20
    }
    if i == 10 {
        loop.skip(2)
    }
    if i == 15 {
        outer.break_if(true)
    }
}
```

## Loop `else`

An optional `else` block runs when the loop ends because its condition becomes
false or its iteration limit is reached. It does not run after a break,
restart, return, or another non-normal exit. A `continue` does not prevent the
`else` block from running later if the loop terminates normally.

```rift
for i := 0; i < 3; i++ {
    println("{}", i)
} else {
    println("finished normally")
}
```
