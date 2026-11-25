# Performance regression investigation and optimization results

## Method

- Original COW baseline: `67a69182c393234011e2dd6ba7063c8eb8068fed`; before-fix implementation: `9f76601f4d6dd79074b7c1782f3b1f97431358f1`; final code includes the analysis and no-op changes in this report.
- All three revisions used the same current benchmark source, MSVC 19.51, C++23, Visual Studio 18 2026 generator, and Release configuration on the same Windows host.
- Each process warmed up each workload and then measured seven `steady_clock` samples. We ran seven process repetitions for each revision and report the median across process medians. Before/after repetitions were interleaved by process to reduce run-order drift.
- Runtime workloads include `validateProgram()` on each `Interpreter::run()`; they are whole-run costs, not pure execution. Semantic-analysis rows call `validateProgram()` directly. There is no separate program-preparation API.
- Allocation counts cover ordinary `new`/`new[]`, not aligned allocations or allocator metadata. Allocated bytes are not peak memory; peak RSS was not measured.
- Repeated raw outputs: [baseline](regression_comparison_baseline_repetitions.txt), [before fixes](regression_paired_before_raw.txt), and [after fixes](regression_paired_after_raw.txt).
- WPR CPU profiling was available to start, but WPA/VS Profiler was unavailable for analyzing sampled call stacks. Findings rely on timing and allocation counters; no exclusive-CPU attribution is claimed.

## Three-way comparison

Times are median ns/op. Positive time change means slower after fixes; negative means faster. Speedup is `before_time / after_time`. Allocation and byte columns show baseline / before / after.

| Workload | Original baseline | Before fixes | After fixes | Before→after speedup | Time change | Allocations (baseline / before / after) | Allocated bytes (baseline / before / after) |
|---|---:|---:|---:|---:|---:|---:|---:|
| Semantic analysis: 200 locals | 47,953 ns | 71,333 ns | 68,110 ns | 1.047× | -4.5% | 621 / 1,051 / 1,050 | 63,214 / 105,340 / 97,004 |
| Semantic analysis: 500 locals | 127,017 ns | 186,942 ns | 181,262 ns | 1.031× | -3.0% | 1,522 / 2,555 / 2,554 | 144,437 / 225,010 / 215,970 |
| No-op interpreter run | 1,105.33 ns | 1,436 ns | 1,360 ns | 1.056× | -5.3% | 22 / 30 / 28 | 1,904 / 2,464 / 2,368 |
| Empty function calls (100) | 64,320 ns | 48,380 ns | 46,600 ns | 1.038× | -3.7% | 673 / 175 / 171 | 67,704 / 9,176 / 8,856 |
| Array arguments (2048 items × 100) | 622,333 ns | 492,067 ns | 578,667 ns | 0.850× | 17.6% | 3,062 / 2,367 / 2,363 | 642,044 / 574,172 / 573,772 |
| Array argument mutation (100) | 1,366,430 ns | 1,365,600 ns | 1,381,630 ns | 0.988× | 1.2% | 3,160 / 2,563 / 2,559 | 8,832,190 / 8,777,050 / 8,776,660 |
| Nested array arguments (100) | 111,467 ns | 88,000 ns | 86,100 ns | 1.022× | -2.2% | 1,005 / 310 / 306 | 100,576 / 16,376 / 15,976 |
| Several array arguments (100) | 175,100 ns | 140,267 ns | 138,433 ns | 1.013× | -1.3% | 1,242 / 345 / 341 | 137,224 / 26,936 / 26,408 |
| Array arguments returned (100) | 497,500 ns | 82,033.3 ns | 80,133.3 ns | 1.024× | -2.3% | 994 / 299 / 295 | 99,928 / 15,680 / 15,280 |
| Recursive function calls (300) | 1,396,810 ns | 192,519 ns | 190,542 ns | 1.010× | -1.0% | 1,569 / 368 / 364 | 153,040 / 18,136 / 17,880 |
| Loop control and returns (1000) | 4,907,960 ns | 809,940 ns | 786,180 ns | 1.030× | -2.9% | 4,124 / 1,111 / 1,107 | 429,096 / 41,976 / 41,448 |
| Indexed local reads/writes (10000) | 6,609,070 ns | 6,027,070 ns | 5,874,800 ns | 1.026× | -2.5% | 30,066 / 10,068 / 10,066 | 2,886,660 / 325,456 / 325,184 |
| int32 arithmetic (10000) | 7,983,900 ns | 6,973,400 ns | 6,984,470 ns | 0.998× | 0.2% | 30,068 / 10,070 / 10,068 | 2,886,780 / 325,600 / 325,328 |
| Array element access (2048 × 1000) | 1,564,500 ns | 1,190,500 ns | 1,289,730 ns | 0.923× | 8.3% | 5,170 / 3,175 / 3,171 | 840,508 / 599,588 / 599,140 |

The earlier report measured semantic analysis at 47.9 → 72.6 μs (+51.7%), no-op execution at 1.126 → 1.454 μs (+29.1%), and array arguments at 531 → 627 μs (+18.1%). The new matched benchmark reproduced the semantic-analysis and no-op regressions, though smaller. It did not reproduce the array slowdown against the original COW baseline (622 → 579 μs, 7.0% faster). The pre-fix and final array-argument measurements differ by 17.6% in the opposite direction, despite the small allocation reduction; this inconsistency is not explained by the code changes, so stage 3 is not claimed as fixed.

The 200- and 500-local cases remain about 42% and 43% slower than the original baseline. No-op runs remain about 23% slower. Array mutation is 1.1% slower than the baseline and 1.2% slower than before fixes. Array element access is 17.6% faster than the baseline, but 8.3% slower than before fixes.

## Reproduced causes and changes

1. **Large-scope semantic analysis.** The analyzer kept copied `std::string` keys in per-scope hash maps and grew those maps incrementally. The AST already owns each declaration name for the full analysis lifetime. Scope keys now use `std::string_view` into that stable AST storage, and each scope reserves its direct declarations. Scope lookup order, duplicate detection before initializer analysis, shadowing, and diagnostics are unchanged. At 200 locals, the paired run fell from 71.3 to 68.1 μs and allocated bytes fell from 105,340 to 97,004; the historical regression is reduced but remains.
2. **No-op execution.** `Interpreter::run()` rebuilt a function-name hash table even though semantic analysis resolves user calls to `Function*`. It now finds the validated unique `main` directly and calls resolved function pointers. Validation still runs on every call to `run()`. In the paired run, no-op time fell from 1.436 to 1.360 μs, with allocations from 30 to 28.
3. **Array arguments.** The existing path already passes COW arrays as shallow handles; no array elements are copied for a same-type parameter. A trial to move the evaluated argument vector and parameter values did not demonstrate a reliable time win or fewer allocations. It was reverted. The COW implementation was left unchanged.

The stage 3 trial's code was committed as `f68b7cb`, then reverted in the findings commit; it is not part of the final implementation. COW value semantics are covered by the added nested-array-through-two-functions test, which mutates inside the callee and checks that the caller's original value is unchanged.

## Additional benchmark coverage

The benchmark includes semantic analysis at 10, 50, 100, 200, 500, and 1,000 locals; no-op and empty function calls; minimal arithmetic; array argument pass-through, mutation, multiple arrays, nested arguments, and returns; recursive calls; loop control; indexed locals; integer arithmetic; and array element access. Workloads remain in the raw output regardless of outcome.

## Rejected approaches and remaining work

- Moving array arguments was rejected because repeated measurements did not establish a reliable improvement and allocation counts did not change. A custom reference counter or new prepared-program API is not justified by these measurements.
- The remaining dominant analysis cost is construction and lookup of per-expression analysis metadata. A larger representation change would affect runtime lookups and was not justified by this targeted pass.
- The current benchmark does not provide reliable peak-memory data or analyzed CPU call stacks. Future profiling should use WPA/VS Profiler and controlled CPU-frequency/thermal conditions before changing array ownership.

## Regression tests and final checks

- Added an array argument COW regression test covering nested mutation through multiple calls.
- Release build succeeded.
- CTest: 9/9 targets passed.
- `git diff --check` passed before the findings/report commits.
- Previous optimization commits and existing user changes were preserved; nothing was pushed.
