# Regression investigation: large-scope semantic analysis

## Finding

The analyzer keeps each active scope in an `unordered_map<string, TypeInfo>`. Every declaration copied its already-owned AST name into a second string key. The keys only live for one `validateProgram()` call, while the AST names live for the entire analysis. This duplicated key storage contributed allocation and byte overhead in large scopes. The analyzer also grew each scope map incrementally.

The change uses `string_view` keys into the immutable program AST and reserves the current scope's known declarations before inserting them. Lexical lookup still searches scopes from innermost to outermost, and duplicate checks still happen before initializer analysis.

## Measurements

Release, MSVC 19.51, same host and benchmark executable, warm-up followed by seven steady-clock samples (median per operation). Allocation counts include ordinary `new`/`new[]`, not peak memory.

| Locals | Before: time | After: time | Time change | Before: bytes | After: bytes |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 10 | 5.29 μs | 5.11 μs | −3.4% | 9,104 | 8,928 |
| 50 | 19.30 μs | 18.27 μs | −5.3% | 31,479 | 30,663 |
| 100 | 39.38 μs | 35.81 μs | −9.1% | 66,045 | 64,429 |
| 200 | 73.23 μs | 69.23 μs | −5.5% | 100,220 | 97,004 |
| 500 | 185.63 μs | 174.33 μs | −6.1% | 223,986 | 215,970 |
| 1,000 | 435.47 μs | 379.49 μs | −12.9% | 458,398 | 442,382 |

At the target size, analysis still takes 42.8% longer than the original baseline report's 48.47 μs. It is approximately linear through 500 declarations, with a higher per-local cost at 1,000; the current change does not eliminate the historical regression. The before/after values above were taken in adjacent Release runs using the same updated benchmark, so small timing differences are subject to run-to-run noise.

The old comprehensive report recorded the 200-local analysis increasing from 47.9 μs / 63,214 bytes to 72.6 μs / 105,340 bytes. The present result lowers the latter to 69.2 μs / 97,004 bytes. Other dominant per-expression analysis side-table allocations remain and were not redesigned because doing so would change how validation metadata is represented and queried at runtime.

## Verification and limits

The complete CTest suite passed (9/9). The benchmark harness now includes 10, 50, 100, 200, 500, and 1,000 locals, and warms up before measuring. WPR is installed, but WPA/VS Profiler is not available in this environment; CPU call-stack samples could not be analyzed. This investigation therefore uses the benchmark's time and allocation counters rather than attributing exclusive CPU samples to individual functions.

Raw output: [`regression_stage_analysis_raw.txt`](regression_stage_analysis_raw.txt).
