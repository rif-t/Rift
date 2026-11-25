# Runtime analysis metadata lookup finding

`interpreter_expressions.cpp` reads semantic metadata in five runtime paths: array element type, identifier slot, conditional result type, array append receiver slot, type inspection, and resolved function calls. These accesses are already constant-time pointer-keyed hash lookups. A dense array would still need a pointer-to-index mapping unless Rift first builds a prepared execution representation.

I measured a compact unified pointer-keyed metadata table against the indexed-local implementation. Both used Release builds, the same benchmark source/settings, and seven process runs with seven samples per reported median. The unified table was not retained because its overall results were mixed:

| Workload | Separate side tables | Unified table | Change |
| --- | ---: | ---: | ---: |
| Semantic analysis only, 200 locals | 73.0 μs; 1,051 allocations; 105,340 bytes | 78.2 μs; 1,038 allocations; 124,909 bytes | 7.0% slower; 18.6% more bytes |
| 200 locals and assignments | 147 μs | 158 μs | 7.4% slower |
| 300 recursive calls | 204 μs | 209 μs | 2.3% slower |
| Local reads/writes, 10,000 iterations | 5.480 ms | 5.823 ms | 6.3% slower |
| No-op function | 1.321 μs; 30 allocations | 1.050 μs; 20 allocations | 20.5% faster |

The no-op case improved, but function-heavy and metadata-heavy cases regressed. The extra structure is not justified by this comparison, so the implementation remains unchanged. Raw experiment output is in [`optimization_metadata_raw.txt`](optimization_metadata_raw.txt); comparison output before the experiment is [`optimization_flow_raw.txt`](optimization_flow_raw.txt).
