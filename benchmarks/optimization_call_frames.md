# Reusable function call frames

`Interpreter` now keeps a pool of call frames. Each frame owns its local-slot vector; on return the values are cleared while vector capacity is kept for the next call at that depth. The pool uses stable frame objects so nested calls cannot invalidate caller locals. This preserves separate recursive state and releases arrays/strings held in locals at frame exit.

Before/after Release measurements use seven process runs, each reporting seven-sample medians. The before executable is revision `63acd8f45a58748784c47bb4cd82d4253e6945a7`; the same benchmark source was built with that revision and the optimized working tree. Raw output: [`optimization_call_frames_before_raw.txt`](optimization_call_frames_before_raw.txt) and [`optimization_call_frames_after_raw.txt`](optimization_call_frames_after_raw.txt).

| Workload | Before | After | Time change | Allocations before → after | Bytes before → after |
| --- | ---: | ---: | ---: | ---: | ---: |
| 300 recursive calls | 202 μs | 202 μs | unchanged | 669 → 368 (−45.0%) | 42,216 → 18,136 (−57.0%) |
| 1,000 shallow calls | 1.127 ms | 1.144 ms | 1.4% slower | 3,092 → 2,091 (−32.4%) | 160,104 → 79,944 (−50.1%) |
| 500 calls with 8 parameters | 1.498 ms | 1.513 ms | 1.0% slower | 1,633 → 1,132 (−30.7%) | 507,376 → 187,216 (−63.1%) |
| Recursive arithmetic | 275 μs | 275 μs | unchanged | 580 → 329 (−43.3%) | 67,136 → 26,976 (−59.8%) |
| Recursive locals and nested scopes | 279 μs | 279 μs | unchanged | 583 → 332 (−43.1%) | 87,328 → 27,088 (−69.0%) |
| Array arguments, 100 calls | 619 μs | 593 μs | 4.3% faster | 2,468 → 2,367 (−4.1%) | 590,332 → 574,172 (−2.7%) |

The main result is fewer allocations and bytes; elapsed time was effectively unchanged in the call-heavy workloads. The frame pool retains vector capacity until the `Interpreter` is destroyed, so a deep call or large frame can keep its peak local-slot capacity for that interpreter's lifetime. Peak resident memory was not measured.
