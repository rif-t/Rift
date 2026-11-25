# Native-width integer fast paths

The `UInt128` add, multiply, and divide helpers now use native 64-bit operations when both operands fit in the low limb and the product fits in 64 bits. Overflowing 64-bit products and all wider values retain the existing 128-bit implementation. This keeps the general path and overflow behavior intact.

The comparison used the same Release benchmark source with seven process runs on the same Windows host, MSVC 19.51.36260.0, Visual Studio 18 2026 generator, and same 7-sample median methodology. The baseline executable was built from revision `67a69182c393234011e2dd6ba7063c8eb8068fed`; the optimized executable was built from the working tree. Raw output is preserved in [`optimization_integer_exact_baseline_raw.txt`](optimization_integer_exact_baseline_raw.txt) and [`optimization_integer_after_raw.txt`](optimization_integer_after_raw.txt).

| Microbenchmark | Before median | After median | Speedup | Time reduction |
| --- | ---: | ---: | ---: | ---: |
| UInt128 narrow multiply | 8.877 ns/op | 3.0785 ns/op | 2.88× | 65.3% |
| UInt128 narrow divide | 169.785 ns/op | 2.7505 ns/op | 61.73× | 98.4% |

The workloads use operands that fit in 64 bits and consume results through a volatile sink. They measure the integer helper directly, so they isolate the optimized operation. The full interpreter arithmetic workloads did not show a meaningful speedup: AST evaluation and validation dominate those end-to-end runs. Allocation counts are zero in these microbenchmarks before and after. Peak memory is not relevant to these stack-only operations and was not measured.
