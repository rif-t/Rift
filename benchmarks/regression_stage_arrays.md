# Regression investigation: array arguments

## Finding and change

An array identifier evaluation creates a shallow `RuntimeValue` copy. That increment in the array's `shared_ptr` count is required: the callee must receive a separate logical value so a later write detaches under copy-on-write. Parameter initialization then passed the already-independent argument as an lvalue to `validateValue`, making another shallow `RuntimeValue` copy and reference-count update.

`callFunction` now takes ownership of the evaluated argument vector, and parameter setup moves each value into `validateValue`. The first copy remains, preserving COW ownership and lifetime rules; the redundant parameter copy is removed. The signature is private to `Interpreter`.

## Measurements

The 2,048-element array / 100 calls benchmark measured 629.90 μs, 2,363 allocations, and 573,772 bytes immediately before this change, versus 579.23 μs, 2,363 allocations, and 573,772 bytes after. Time fell 8.0%; allocation counts and bytes did not change, as expected for removing shared-pointer copies rather than heap allocations. The before/after values are medians of seven `steady_clock` samples following warm-up, from Release MSVC 19.51 builds on the same host.

The original comparison report measured 531 μs before the earlier optimizations and 627 μs before this investigation, so the new result remains about 9.1% slower than the old baseline. That comparison also varies by benchmark revision and warm-up methodology; the paired measurements above are stronger evidence for this change.

The expanded benchmark now also measures array arguments unused by the callee, mutation, nested arrays, and returning arrays. See [`regression_stage_arrays_raw.txt`](regression_stage_arrays_raw.txt). The COW regression test passes an array through two calls, mutates a nested value in the callee, and confirms the caller's value is unchanged.

## Verification and limits

All 9 CTest targets passed. The complete benchmark includes seven measured samples per workload and one warm-up. Counts include ordinary `new`/`new[]`, not allocator metadata or peak memory. WPA/VS Profiler is unavailable here, so no CPU call-stack profile is claimed.
