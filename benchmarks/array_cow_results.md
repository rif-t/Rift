# Array copy-on-write benchmark

Release measurements on Windows with MSVC 19.51. Each value is the median of
seven runs. A run is one interpreter execution of the workload; allocation
counts cover ordinary `new` and `new[]` during that execution. The baseline
was built from the pre-change interpreter with the same benchmark source.

| Workload | Time before → after (ns/run) | Allocations before → after | Bytes before → after | Time change |
| --- | ---: | ---: | ---: | ---: |
| Read a 2,048-element array 100 times | 20,389,500 → 490,267 | 2,760 → 2,560 | 8,796,330 → 593,228 | −97.6% |
| Access array elements 1,000 times | 13,221,300 → 1,551,370 | 7,170 → 5,170 | 82,871,500 → 840,508 | −88.3% |
| Copy a 2,048-element array 20 times | 4,621,600 → 526,333 | 2,280 → 2,240 | 2,200,570 → 559,948 | −88.6% |
| Modify 20 independent copies | 4,629,070 → 660,833 | 2,282 → 2,282 | 2,200,700 → 2,200,700 | −85.7% |
| Pass a 2,048-element array to a function 100 times | 23,194,400 → 573,767 | 3,462 → 3,062 | 17,048,200 → 642,044 | −97.5% |
| Read/copy a nested array 500 times | 1,366,800 → 390,967 | 14,606 → 2,594 | 1,101,900 → 281,512 | −71.4% |
| Copy and mutate an array nested 8 levels (40 times) | 104,933 → 82,300 | 1,357 → 1,042 | 83,080 → 66,744 | −21.6% |
| Append 1,000 values | 608,367 → 626,433 | 3,088 → 3,088 | 423,242 → 423,242 | +3.0% |

The read, copy, and function workloads stop recursively allocating and copying
array elements. Indexed reads also avoid cloning nested arrays. A write to a
shared array still copies that array's element vector once; nested writes detach
each shared level along the path. The append workload does not benefit because
it grows one uniquely owned array, and its small timing difference is within
run-to-run variation.

Array values still use `std::shared_ptr`, so passing or reading an array value
does a reference-count update. This keeps the existing runtime representation
and lifetime guarantees; the measurements show that avoiding recursive element
copies outweighs that bookkeeping for the tested workloads.

The allocation hook does not count aligned allocations or allocations made
outside ordinary `new`/`new[]`. These figures compare the interpreter's normal
allocation paths, not total process memory or peak resident memory.
