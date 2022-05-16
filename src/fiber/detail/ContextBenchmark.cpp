#include "../../../include/benchmark/benchmark.h"

// 2022-05-16T09:26:06+08:00
// Run on (8 X 4100 MHz CPU s)
// CPU Caches:
//   L1 Data 32 KiB (x4)
//   L1 Instruction 32 KiB (x4)
//   L2 Unified 256 KiB (x4)
//   L3 Unified 8192 KiB (x1)
// Load Average: 0.82, 1.28, 1.04
// ----------------------------------------------------------------
// Benchmark                      Time             CPU   Iterations
// ----------------------------------------------------------------
// Benchmark_MakeContext       1.50 ns         1.50 ns    464366360
// Benchmark_JumpContext       6.52 ns         6.52 ns    106898757


extern "C" {
void jump_context(void** self, void* to, void* context);
void* make_context(void* sp, std::size_t size, void (*start_proc)(void*));
}

namespace tinyRCPC::fiber::detail {

void Benchmark_MakeContext(benchmark::State& state) {
  char stack_buffer[4096];
  while (state.KeepRunning()) {
    make_context(stack_buffer + 4096, 4096, nullptr);
  }
}

BENCHMARK(Benchmark_MakeContext);

void* master;
void* child;

void Benchmark_JumpContext(benchmark::State& state) {
  char ctx[4096];
  child = make_context(ctx + 4096, 4096, [](void*) {
    while (true) {
      jump_context(&child, master, nullptr);
    }
  });
  while (state.KeepRunning()) {
    // Note that we do two context switch each round (one to `child`, and one
    // back to `master`.).
    jump_context(&master, child, nullptr);
  }
}

BENCHMARK(Benchmark_JumpContext);

}  // namespace tinyRPC::fiber::detail