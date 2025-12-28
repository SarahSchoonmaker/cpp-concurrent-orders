# cpp-concurrent-order-engine

A **C++17** concurrent order simulation + processing engine that demonstrates **thread-safe design**, **correct shared-state
management**, and **clean shutdown semantics**. This is a **C++ development project** (not ops/support tooling).

It models a small “order pipeline”:

- Multiple producer threads submit orders concurrently
- A thread pool consumes tasks from a bounded queue
- An `OrderEngine` maintains a shared in-memory ledger safely
- Deterministic processing is achieved via a global sequence number + ordered commit

## Project Features

- Modern C++17: RAII, STL, move semantics
- Concurrency: `std::thread`, `std::mutex`, `std::condition_variable`, atomic seq
- Correctness: bounded queue, backpressure, graceful shutdown
- Deterministic results: events are *committed* in sequence order even if processed in parallel

## Build

### MSYS2 (UCRT64/MINGW64) + Ninja

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Run:

```bash
./build/order_engine_demo.exe --orders 50000 --producers 4 --workers 4 --symbols 50
```

### Linux/macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/order_engine_demo --orders 50000 --producers 4 --workers 4 --symbols 50
```

### Windows (Visual Studio generator)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\build\Release\order_engine_demo.exe --orders 50000 --producers 4 --workers 4 --symbols 50
```

## CLI options

- `--orders N` total orders to submit (default 20000)
- `--producers N` producer threads (default 4)
- `--workers N` worker threads (default 4)
- `--symbols N` number of synthetic symbols (default 25)
- `--queue N` bounded queue capacity (default 8192)
- `--seed N` RNG seed (default 42)
- `--print-sample` print a small sample of committed events

## Output

The demo prints throughput-ish counters and a deterministic summary:

- Total submitted / processed / committed
- Per-symbol net position (buys - sells)
- Total notional traded

Because commits are ordered by sequence number, running with the same `--seed` yields the same summary.

## Layout

```
cpp-concurrent-order-engine/
  CMakeLists.txt
  include/
    bounded_queue.hpp
    order.hpp
    order_engine.hpp
    thread_pool.hpp
  src/
    order_engine.cpp
    main.cpp
  tests/
    test_determinism.cpp
```


