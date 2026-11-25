# Regression investigation: no-op execution

## Finding and change

Every `Interpreter::run()` rebuilt an `unordered_map<string, vector<Function*>>` for functions even though `validateProgram()` had already put a resolved `Function*` in every ordinary user call's analysis record. In an empty `main`, the table is only used to find `main`. The map and copied name strings added fixed work and allocations on every run.

After validation, `run()` now finds the unique `main` function directly in `Program::functions`. User calls dispatch through the function pointer selected by semantic analysis. `run()` still validates the program on every invocation, preserving its behavior.

## Measurement

The no-op benchmark (empty main; includes validation and interpreter setup) measured 1,422.67 ns, 30 allocations, and 2,448 bytes before this change, versus 1,194.67 ns, 28 allocations, and 2,368 bytes after. This is 16.0% lower time and two fewer allocations. The historical original baseline report measured 1,110 ns / 22 allocations / 1,904 bytes, so a 7.6% time regression remains against that older result. These figures are one host, Release, MSVC 19.51; seven steady-clock samples after warm-up. They include semantic validation and are not pure execution time.

Other workloads in [`regression_stage_runtime_raw.txt`](regression_stage_runtime_raw.txt) were rerun and retained. Their short differences from stage 1 are noisy and this change targets only the per-run lookup map.

## Verification and limits

Function overload, namespace, and recursion coverage remains in the CTest suite. WPR is available, but WPA/VS Profiler is not installed, so call-stack sample analysis was unavailable. The benchmark allocation counter covers ordinary `new`/`new[]`, not aligned allocations, allocator metadata, or peak memory.
