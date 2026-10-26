# Rift benchmarks

Build and run the opt-in Release benchmark on Windows:

```powershell
cmake -S . -B build-bench -DRIFT_BUILD_BENCHMARKS=ON
cmake --build build-bench --config Release --target rift_bench
.\build-bench\Release\rift_bench.exe
```

The executable times parsing a 3,000-declaration program, repeated interpreter
runs for arithmetic, recursive calls, variable lookup and string concatenation,
and isolated `UInt128` multiply, divide and decimal formatting. Runtime timings
include semantic validation and interpreter setup once per run. Allocation
counts cover ordinary `new`/`new[]` calls seen by the process; they do not count
aligned allocations or allocator metadata. AST byte estimates count only
`sizeof` each AST node and exclude vectors, strings, and allocator overhead.

## Initial local measurements

One Release run on the project workstation gave these approximate numbers:

| Workload | Time | Allocations |
| --- | ---: | ---: |
| Parse 3,000 declarations | 2.85 ms | 12,048 / 6.05 MB |
| No-op interpreter run | 0.58 μs | 12 / 1.06 KB |
| 300 recursive calls | 1.04 ms / run | 1,540 / run |
| 250 call steps with arithmetic | 0.98 ms / run | 1,547 / run |
| 250 call steps with local lookup | 1.05 ms / run | 1,798 / run |
| 150 string concatenations | 0.62 ms / run | 1,393 / run |
| `UInt128` multiply | 9 ns / operation | 0 |
| `UInt128` divide | 184 ns / operation | 0 |
| `UInt128` decimal formatting, after chunking | 604 ns / value | 1 / value |

For comparison, the same formatting microbenchmark before chunking took about
10.9 μs per value and made two allocations per value. Formatting now extracts
base-10 chunks of 19 digits at a time, reducing the measured time by about 18x
in the latest run and allocations by half. Timing varies between runs, so use
multiple runs before evaluating future changes.

The current AST object sizes are `Expression` 160 bytes and `Statement` 192
bytes. The 3,000-declaration AST uses about 1.06 MB for node objects alone; the
parse allocation sample was about 6.05 MB including token, vector, string, and
allocation overhead. These measurements establish a baseline for considering
an AST representation change; no typed-AST rewrite is justified by this sample
alone.
