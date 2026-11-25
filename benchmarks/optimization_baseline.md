# Runtime optimization baseline

- Baseline revision: `67a69182c393234011e2dd6ba7063c8eb8068fed`
- Configuration: Release, CMake Visual Studio 18 2026 generator, C++23
- Compiler: MSVC 19.51 (recorded by the existing benchmark report)
- Hardware/OS: the existing report records Windows 11 and Intel Core i5-12400 (6 cores / 12 threads); host inventory APIs were unavailable in this run, so these details could not be independently rechecked.
- Method: seven separate benchmark process runs. Each printed result is itself the median of seven `steady_clock` samples. This table reports medians across the seven process medians. Times are per interpreter invocation or micro-operation as labeled. Interpreter workloads include `validateProgram()` on every `run()`; the separate analysis row measures validation only.
- Allocation counts cover ordinary `new`/`new[]`, not aligned allocations or allocator metadata. They are per interpreter invocation or micro-operation as labeled and are not peak memory measurements.

Raw output: [`optimization_baseline_raw.txt`](optimization_baseline_raw.txt).

| Workload | Median time (ns) | Allocations | Allocated bytes |
| --- | ---: | ---: | ---: |
| Parse 3,000 declarations | 4,321,500 | 12,048 | 8,473,460 |
| No-op interpreter run | 1,110 | 22 | 1,904 |
| Arithmetic and 250 recursive frames | 1,275,100 | 1,582 | 171,672 |
| 300 recursive calls | 1,379,880 | 1,569 | 153,040 |
| 250 recursive calls with locals | 1,293,330 | 1,832 | 203,696 |
| 150 string concatenations | 750,863 | 1,426 | 180,328 |
| 100-level nested expressions | 44,304.3 | 227 | 24,239 |
| Local reads and writes, 10,000 iterations | 6,681,430 | 30,066 | 2,886,660 |
| Nested shadowed scopes | 4,787 | 91 | 9,912 |
| Loop control and returns, 1,000 iterations | 4,835,000 | 4,124 | 429,096 |
| int32 arithmetic, 10,000 iterations | 7,995,530 | 30,068 | 2,886,780 |
| int64 arithmetic, 10,000 iterations | 7,984,230 | 30,068 | 2,886,780 |
| int128 arithmetic, 10,000 iterations | 8,055,700 | 30,068 | 2,886,780 |
| 200 locals and assignments | 133,690 | 828 | 98,469 |
| Semantic analysis only, 200 locals | 48,471 | 621 | 63,214 |
| Array reads, 2,048 items × 100 | 560,933 | 2,560 | 593,228 |
| Array element access, 2,048 items × 1,000 | 1,567,630 | 5,170 | 840,508 |
| Array copies, 2,048 items × 20 | 506,433 | 2,240 | 559,948 |
| Shared array modifications, 20 detaches | 697,667 | 2,282 | 2,200,700 |
| Array arguments, 100 calls | 584,567 | 3,062 | 642,044 |
| Array append, 1,000 elements | 614,367 | 3,088 | 423,242 |
| Nested array reads/copies | 391,467 | 2,594 | 281,512 |
| Deep nested array copies/writes | 81,666.7 | 1,042 | 66,744 |
| UInt128 multiply | 6.5835 | 0 | 0 |
| UInt128 divide | 169.824 | 0 | 0 |
| UInt128 decimal formatting | 547.4 | 1 | 48 |

The interpreter benchmark intentionally retains the existing behavior where every `Interpreter::run()` performs semantic validation. Consequently these execution results include validation cost and must not be presented as runtime-only execution. The allocation hook counts ordinary C++ allocations only; peak process memory is not measured.
