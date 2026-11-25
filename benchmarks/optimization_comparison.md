# Runtime optimization comparison

## Configuration and method

- Baseline source revision: `67a69182c393234011e2dd6ba7063c8eb8068fed`
- Optimized source revision measured: `9fd956af51ed14816e19f6afba3473e4ad102d8b`
- Host: Windows 11; Intel Core i5-12400, 6 cores / 12 threads, as recorded in the pre-existing benchmark report. Host inventory APIs were blocked in this run, so the hardware details could not be independently rechecked.
- Compiler: MSVC 19.51.36260.0; C++23; CMake Visual Studio 18 2026 generator; Release configuration.
- Method: seven separate process runs per revision. Each benchmark result is the median of seven `steady_clock` samples within a process; this report uses the median of the seven process medians. Both revisions used the identical final benchmark source and settings. The baseline was rebuilt from the original source revision with that benchmark source.
- Runtime workloads call `Interpreter::run()`, which performs semantic validation on every invocation. Their times include validation and interpreter setup; they are end-to-end run costs, not pure execution-only times. The semantic-analysis-only row measures validation without execution.
- Allocation counts/bytes measure ordinary `new` and `new[]` only. Aligned allocations and allocator metadata are excluded. Allocated bytes are not peak memory; peak resident memory was not measured.
- Raw output: [`optimization_comparison_baseline_raw.txt`](optimization_comparison_baseline_raw.txt) and [`optimization_final_raw.txt`](optimization_final_raw.txt).

## Before and after

Speedup is `before_time / after_time`; positive time change is calculated as `(before - after) / before`. Allocation and byte changes are reductions; negative values indicate an increase.

| Workload | Time before → after | Speedup | Allocations before → after | Allocated bytes before → after |
| --- | ---: | ---: | ---: | ---: |
| Parse 3,000 declarations | 4.512 → 4.406 ms | 1.02× | 12,048 → 12,048 | 8,473,460 → 8,473,460 |
| No-op interpreter run | 1.126 → 1.454 μs | 0.77× | 22 → 30 (−36.4%) | 1,904 → 2,464 (−29.4%) |
| Arithmetic + 250 recursive frames | 1.287 → 0.277 ms | 4.64× | 1,582 → 329 (79.2%) | 171,672 → 26,976 (84.3%) |
| 300 recursive calls | 1.406 → 0.200 ms | 7.02× | 1,569 → 368 (76.5%) | 153,040 → 18,136 (88.1%) |
| 1,000 shallow function calls | 5.418 → 1.146 ms | 4.73× | 8,088 → 2,091 (74.1%) | 785,064 → 79,944 (89.8%) |
| 500 calls with 8 parameters | 3.727 → 1.517 ms | 2.46× | 7,629 → 1,132 (85.2%) | 988,128 → 187,216 (81.1%) |
| 250 recursive calls with locals | 1.334 → 0.279 ms | 4.78× | 1,832 → 332 (81.9%) | 203,696 → 27,088 (86.7%) |
| 150 string concatenations | 769 → 155 μs | 4.96× | 1,426 → 672 (52.9%) | 180,328 → 92,912 (48.5%) |
| 100-level nested expressions | 44.6 → 46.8 μs | 0.95× | 227 → 237 (−4.4%) | 24,239 → 26,375 (−8.8%) |
| Local reads/writes, 10,000 iterations | 6.599 → 5.944 ms | 1.11× | 30,066 → 10,068 (66.5%) | 2,886,660 → 325,456 (88.7%) |
| Nested shadowed scopes | 4.910 → 4.759 μs | 1.03× | 91 → 80 (12.1%) | 9,912 → 6,872 (30.7%) |
| Loop control and returns, 1,000 iterations | 4.934 → 0.797 ms | 6.19× | 4,124 → 1,111 (73.1%) | 429,096 → 41,976 (90.2%) |
| int32 arithmetic, 10,000 iterations | 7.912 → 7.419 ms | 1.07× | 30,068 → 10,070 (66.5%) | 2,886,780 → 325,600 (88.7%) |
| int64 arithmetic, 10,000 iterations | 7.935 → 7.443 ms | 1.07× | 30,068 → 10,070 (66.5%) | 2,886,780 → 325,600 (88.7%) |
| int128 arithmetic, 10,000 iterations | 7.964 → 7.401 ms | 1.08× | 30,068 → 10,070 (66.5%) | 2,886,780 → 325,600 (88.7%) |
| 200 locals and assignments | 133 → 149 μs | 0.89× | 828 → 1,053 (−27.2%) | 98,469 → 105,420 (−7.1%) |
| Semantic analysis only, 200 locals | 47.9 → 72.6 μs | 0.66× | 621 → 1,051 (−69.2%) | 63,214 → 105,340 (−66.6%) |
| Array reads/copies, 2,048 items × 100 | 546 → 603 μs | 0.91× | 2,560 → 2,268 (11.4%) | 593,228 → 570,244 (3.9%) |
| Array element access, 2,048 items × 1,000 | 1.535 → 1.335 ms | 1.15× | 5,170 → 3,175 (38.6%) | 840,508 → 599,588 (28.7%) |
| Array copies, 2,048 items × 20 | 508 → 532 μs | 0.95× | 2,240 → 2,188 (2.3%) | 559,948 → 567,684 (−1.4%) |
| Shared array modifications, 20 detaches | 691 → 713 μs | 0.97× | 2,282 → 2,231 (2.2%) | 2,200,700 → 2,208,480 (−0.4%) |
| Array arguments, 100 calls | 531 → 627 μs | 0.85× | 3,062 → 2,367 (22.7%) | 642,044 → 574,172 (10.6%) |
| Array append, 1,000 elements | 605 → 544 μs | 1.11× | 3,088 → 1,089 (64.7%) | 423,242 → 166,026 (60.8%) |
| Nested array reads/copies | 388 → 336 μs | 1.15× | 2,594 → 600 (76.9%) | 281,512 → 24,528 (91.3%) |
| Deep nested array copies/writes | 80.7 → 77.6 μs | 1.04× | 1,042 → 925 (11.2%) | 66,744 → 50,352 (24.6%) |
| UInt128 narrow multiply | 8.939 → 3.037 ns | 2.94× | 0 → 0 | 0 → 0 |
| UInt128 narrow divide | 170.226 → 2.768 ns | 61.51× | 0 → 0 | 0 → 0 |
| UInt128 wide multiply | 6.611 → 6.744 ns | 0.98× | 0 → 0 | 0 → 0 |
| UInt128 wide divide | 172.172 → 170.875 ns | 1.01× | 0 → 0 | 0 → 0 |
| UInt128 decimal formatting | 551 → 392 ns | 1.40× | 1 → 1 | 48 → 48 |

Zero-allocation workloads remain at zero; the reduction percentage is therefore 0% rather than undefined. The narrow multiplication/division microbenchmarks use operands that fit in 64 bits and consume their outputs through a volatile sink.

## Results by optimization stage

1. **Indexed locals — commit `7f872fdb0715fd739ac0940f3380777bab0e1a7e`.** String-keyed runtime scope lookups were replaced by semantic-analysis-assigned slots. Local access, recursive calls, and array element access improved; the 200-local and no-op workloads regressed because of analysis metadata setup. See [`optimization_indexed_locals.md`](optimization_indexed_locals.md).
2. **Explicit flow — commit `b064645bd1c5ee21478dcfdeacc700c6bf151f03`.** Returns and loop actions now use tagged flow state. The focused loop workload improved 5.86× and the 300-frame call workload 6.43× relative to the immediately preceding stage. See [`optimization_control_flow.md`](optimization_control_flow.md).
3. **Analysis metadata lookup — no runtime change.** A unified side-table experiment regressed semantic-analysis and local-heavy workloads despite improving the no-op case, so it was not retained. See [`optimization_analysis_metadata.md`](optimization_analysis_metadata.md).
4. **Native-width integer helpers — commit `152ede5de1a787b6faf83439a72f20125084e508`.** Narrow UInt128 multiplication improved 2.94× and narrow division 61.51× in direct helper measurements. End-to-end arithmetic changed only modestly because interpreter and validation work dominate. See [`optimization_integer_fast_paths.md`](optimization_integer_fast_paths.md).
5. **Reusable function frames — commit `9fd956af51ed14816e19f6afba3473e4ad102d8b`.** A frame pool reuses local-slot capacity. In the matched frame benchmark, the 300-frame workload kept similar time while reducing allocations by 45.0% and allocated bytes by 57.0%. See [`optimization_call_frames.md`](optimization_call_frames.md).

## Regressions and trade-offs

- Semantic analysis for 200 locals took 51.7% longer and allocated 66.6% more bytes. The no-op function also took 29.1% longer. These show the cost of creating slot metadata on every `run()`.
- Array reads/copies, array arguments, large array copies, and shared array modifications regressed 3–18% in time. These workloads include validation and setup each run; their allocation counts still fell, but those reductions did not always translate into faster execution.
- The 200-local workload regressed 12.4%; nested expressions regressed 4.9%. Large parsing was effectively unchanged (+2.4%).
- Narrow helper paths are significantly faster; wide UInt128 multiply/divide are effectively unchanged. Peak resident memory was not sampled. The call-frame pool keeps each frame's high-water vector capacity until interpreter destruction.
- The COW implementation was not changed, and its existing tests pass.

## Assessment and next steps

The largest gains are in normal control-flow transfers and local-heavy recursion. The runtime now avoids repeated string lookups and exception unwinding, and reuses call-frame storage. The remaining main cost is semantic validation and the metadata it builds on every `Interpreter::run()`. Before adding further runtime metadata, Rift should measure a safe validation cache or prepared-program representation and reduce side-table allocation overhead. Array copy workloads also deserve profiling, especially where slot and frame changes added setup cost.

The results describe Rift against its own pre-change revision on one host. They do not compare Rift with C++, Rust, or another language.
