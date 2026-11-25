# Regression investigation: array arguments

## Finding

The measured slowdown was not caused by a costly copy of array contents: Rift's copy-on-write arrays already copy only the shared handle on argument evaluation. Parameter initialization copies that handle once more through `validateValue`. A trial changed the private call path to move the evaluated argument vector and each parameter value, which removed a shared-pointer reference-count update in theory.

The trial did not show a reliable performance benefit. Grouped process medians were 487.0 μs before and 610.0 μs during the trial, with identical allocations/bytes (2,367 / 574,172). An interleaved before/final comparison measured 497.4 μs before and 579.6 μs after the surrounding fixes, also with unchanged allocation counts/bytes apart from a small unrelated reduction. Array mutation was 1.348 ms before and 1.376 ms after. Because process conditions still produce noticeable drift and the argument trial does not reduce allocations, the move change was reverted.

The historical report measured 531 μs on the original COW baseline and 627 μs before this investigation (+18.1%). The new matched three-way benchmark measured 610 μs on that baseline and 580 μs after these fixes. It therefore did not reproduce the earlier regression against the original baseline, but this stage has not demonstrated an improvement over the pre-fix implementation.

## Correctness and future work

The `array_argument_keeps_value_semantics` test passes an array through two function calls, mutates a nested element in the callee, and confirms the original remains unchanged. Existing tests cover array returns, assignments, append, indexed writes, and deep COW detachment. No COW representation or semantics were changed.

The benchmark measures unused, reading, mutating, nested, and returned array arguments. Raw paired results are in [before](regression_paired_before_raw.txt) and [after](regression_paired_after_raw.txt). The move trial was rejected; current evidence does not justify another ownership/API change.

WPA/VS Profiler was unavailable, so call-stack samples were not analyzed. Allocation counters include ordinary `new`/`new[]`, not allocator metadata or peak memory.
