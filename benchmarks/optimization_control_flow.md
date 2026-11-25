# Exception-free interpreter control flow

Function returns and loop break/continue/restart now use a tagged execution-flow state. Function calls consume return state locally; loops consume controls addressed to themselves and pass outer labels and returns upward. Runtime errors still use exceptions. The existing loop suite covers labels, skip, iteration limits, loop else behavior, and nested loops.

The table compares the seven-process medians immediately before and after this change using the same Release executable, benchmark source, and settings. This stage builds on indexed locals. Raw after output: [`optimization_flow_raw.txt`](optimization_flow_raw.txt); the before output is [`optimization_indexed_locals_raw.txt`](optimization_indexed_locals_raw.txt).

| Workload | Before | After | Speedup | Time reduction | Allocations before → after |
| --- | ---: | ---: | ---: | ---: | ---: |
| Recursive calls, 300 frames | 1.314 ms | 204 μs | 6.43× | 84.4% | 669 → 669 |
| Loop control and returns, 1,000 iterations | 4.424 ms | 755 μs | 5.86× | 82.9% | 1,112 → 1,112 |
| Recursive arithmetic, 250 frames | 1.197 ms | 281 μs | 4.26× | 76.5% | 580 → 580 |
| Recursive local lookup, 250 frames | 1.206 ms | 282 μs | 4.28× | 76.6% | 583 → 583 |
| Local reads/writes, 10,000 iterations | 5.499 ms | 5.480 ms | 1.00× | 0.4% | unchanged |

Allocation counts are unchanged for these interpreter workloads. The large gains are in paths that previously threw and caught C++ exceptions for normal returns and loop actions. The local read/write workload has no such control transfers and remains effectively unchanged.
