# Indexed local access

Semantic analysis assigns each function parameter and local declaration a stable slot. Runtime reads, assignments, array append, and variable type inspection use those slots instead of searching string-keyed scope maps. The AST remains unchanged; slot indices live in `AnalysisResult`. Block slots are cleared on exit, and for-loop initializer/post slots live for the complete loop scope.

The Release comparison uses the same seven-process benchmark setup as [`optimization_baseline.md`](optimization_baseline.md). Before values are from baseline revision `67a69182c393234011e2dd6ba7063c8eb8068fed`; after values are the median of seven process medians. Raw after output: [`optimization_indexed_locals_raw.txt`](optimization_indexed_locals_raw.txt). Baseline output: [`optimization_baseline_raw.txt`](optimization_baseline_raw.txt).

| Workload | Before | After | Time change | Allocations before → after |
| --- | ---: | ---: | ---: | ---: |
| Local reads/writes, 10,000 iterations | 6.681 ms | 5.499 ms | 17.7% faster | 30,066 → 10,069 |
| Recursive calls, 300 frames | 1.380 ms | 1.314 ms | 4.8% faster | 1,569 → 669 |
| Recursive arithmetic, 250 frames | 1.275 ms | 1.197 ms | 6.1% faster | 1,582 → 580 |
| 250 recursive calls with locals | 1.293 ms | 1.206 ms | 6.8% faster | 1,832 → 583 |
| Array element access, 1,000 accesses | 1.568 ms | 1.254 ms | 20.0% faster | 5,170 → 3,176 |
| Array reads/copies | 561 μs | 537 μs | 4.3% faster | 2,560 → 2,269 |
| Array arguments, 100 calls | 585 μs | 585 μs | effectively unchanged | 3,062 → 2,468 |
| 200 local declarations and assignments | 134 μs | 149 μs | 11.4% slower | 828 → 1,054 |
| No-op function | 1.11 μs | 1.46 μs | 31.4% slower | 22 → 30 |

The lookup-heavy workloads use fewer allocations and run faster. Programs with many declarations pay for the analysis side tables, and the no-op case shows the fixed metadata cost clearly. These regressions are included rather than hidden. This remains a trade-off for workloads with little or no local access; the next performance pass should reduce side-table setup cost before adding more runtime metadata.
