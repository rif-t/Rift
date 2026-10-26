# Rift benchmarks

Build and run the opt-in Release benchmark on Windows:

```powershell
cmake -S . -B build-bench -DRIFT_BUILD_BENCHMARKS=ON
cmake --build build-bench --config Release --target rift_bench
.\build-bench\Release\rift_bench.exe
```

On Linux or macOS:

```sh
cmake -S . -B build-bench -DCMAKE_BUILD_TYPE=Release -DRIFT_BUILD_BENCHMARKS=ON
cmake --build build-bench --config Release --target rift_bench
./build-bench/rift_bench
```

Run the benchmark at least three times and compare medians. It reports timings
per operation, total ordinary `new`/`new[]` calls and bytes for each sample,
and the `sizeof` values of key runtime and AST types. Allocation counting does
not include aligned allocations or allocator metadata. AST object-size estimates
exclude vectors, strings, and allocator overhead.

Workloads cover parsing a 3,000-declaration program, interpreter setup, recursive
arithmetic/function calls, local-variable lookup, string concatenation, and
100-level nested expressions. Rift does not currently implement loop syntax;
recursion is the closest available repeated-control-flow workload. The final
microbenchmarks measure `UInt128` multiplication, division, and decimal
formatting. Runtime timings include semantic validation and interpreter setup
once per run.

## Baseline

Three Release runs on 2026-10-26 on Windows 11, an Intel Core i5-12400
(6 cores / 12 logical processors), MSVC 19.51, Visual Studio 18 generator.
Times below are medians; allocations and bytes are per program run, derived
from the benchmark's fixed sample counts.

| Workload | Median time | Allocations / run | Bytes / run |
| --- | ---: | ---: | ---: |
| Parse 3,000 declarations | 3.54 ms | 12,048 | 5,886,132 |
| No-op interpreter run | 601 ns | 12 | 1,072 |
| Arithmetic + 250 recursive frames | 1.03 ms | 1,547 | 168,408 |
| 300 recursive calls | 1.11 ms | 1,540 | 150,144 |
| 250 calls with local lookup | 1.09 ms | 1,798 | 200,480 |
| 150 string concatenations | 624 μs | 1,393 | 177,176 |
| 100-level nested expressions | 29.3 μs | 14 | 1,288 |
| `UInt128` multiply | 7.59 ns | 0 | 0 |
| `UInt128` divide | 178 ns | 0 | 0 |
| `UInt128` decimal formatting | 553 ns | 1 | 48 |

The current AST sizes are `Expression` 192 bytes and `Statement` 168 bytes.
The 3,000-declaration sample contains 3,000 of each, or about 1.08 MB in node
objects alone. Parsing this sample allocates about 5.89 MB including tokens,
containers, strings, and allocation overhead. The recursive interpreter
workloads take about 1 ms and allocate roughly 1,500–1,800 times per run. These
numbers identify scope/function-call execution and parsing as useful areas to
profile further, but they do not demonstrate that a different AST layout would
improve performance. Treat this as a machine-specific baseline; compare before
and after measurements on the same machine and compiler.

Floating-point NaN and infinity cannot be written as Rift numeric literals.
Operations that produce a non-finite float or double already return a language
runtime error; division by zero is also rejected. The regression suite checks
those overflow paths and numeric boundaries that the current syntax can
express.
