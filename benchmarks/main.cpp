#include "projectile_sim/simulation.hpp"
#include <benchmark/benchmark.h>
#include <cmath>
#include <cstdlib>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace {
constexpr benchmark::IterationCount batch_ticks = 60;
constexpr unsigned seed = 5000;

template <pdef::ProjectileVector Vec3>
void tick_benchmark(benchmark::State &state) {
  using Scalar = typename Vec3::value_type;
  const auto count = static_cast<std::size_t>(state.range(0));
  const Scalar dt = Scalar{1} / Scalar{60};
  std::mt19937 generator(seed);
  std::uniform_real_distribution<double> position(-30, 30);
  std::uniform_real_distribution<double> velocity(-10, 10);
  pdef::Projectiles<Vec3> initial;
  initial.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    initial.push_back({{static_cast<Scalar>(position(generator)),
                        static_cast<Scalar>(position(generator)),
                        static_cast<Scalar>(position(generator))},
                       {static_cast<Scalar>(velocity(generator)),
                        static_cast<Scalar>(velocity(generator)),
                        static_cast<Scalar>(velocity(generator))},
                       Scalar{70},
                       0});
  }
  psim::ProjectileEngine<Vec3> engine(initial);
  // One benchmark iteration is one engine tick, despite timing in batches.
  while (state.KeepRunningBatch(batch_ticks)) {
    state.PauseTiming();
    engine = psim::ProjectileEngine<Vec3>(initial);
    const auto *data = engine.projectiles().data();
    benchmark::DoNotOptimize(data);
    state.ResumeTiming();
    for (benchmark::IterationCount tick = 0; tick < batch_ticks; ++tick) {
      engine.tick(dt);
      benchmark::ClobberMemory();
    }
  }
  // KeepRunningBatch has stopped the timer before validation and reporting.
  if (engine.projectiles().size() != count) {
    state.SkipWithError("Projectile count changed");
  }
  for (const auto &projectile : engine.projectiles()) {
    bool valid =
        projectile.lifetime > Scalar{0} && std::isfinite(projectile.lifetime);
    for (int axis = 0; axis < 3; ++axis) {
      valid = valid && std::isfinite(projectile.position[axis]) &&
              std::isfinite(projectile.velocity[axis]);
    }
    if (!valid) {
      state.SkipWithError(
          "Expired or nonfinite projectile in all-active workload");
      break;
    }
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
  state.counters["projectiles"] = static_cast<double>(count);
  state.counters["projectile_bytes"] = sizeof(pdef::BaseProjectile<Vec3>);
}

void register_benchmarks() {
  for (const auto count : {1000, 10000, 100000}) {
    for (const bool use_double : {false, true}) {
      auto *entry = benchmark::RegisterBenchmark(
          use_double ? "Tick/double" : "Tick/float",
          use_double ? &tick_benchmark<glm::dvec3>
                     : &tick_benchmark<glm::vec3>);
      entry->Arg(count)->Unit(benchmark::kMicrosecond);
    }
  }
}
} // namespace

int main(int argc, char **argv) {
  // Supply defaults only when neither a CLI flag nor its environment override
  // exists. Per-benchmark registration settings would override CLI flags.
  std::vector<std::string> arguments(argv, argv + argc);
  const auto add_default = [&](std::string_view flag, const char *environment,
                               std::string_view value) {
    for (const auto &argument : arguments) {
      if (argument == flag || argument.starts_with(std::string(flag) + "=")) {
        return;
      }
    }
    if (std::getenv(environment) == nullptr) {
      arguments.emplace_back(std::string(flag) + "=" + std::string(value));
    }
  };
  add_default("--benchmark_repetitions", "BENCHMARK_REPETITIONS", "5");
  add_default("--benchmark_min_warmup_time", "BENCHMARK_MIN_WARMUP_TIME",
              "0.2");
  add_default("--benchmark_min_time", "BENCHMARK_MIN_TIME", "0.5s");
  std::vector<char *> argument_pointers;
  for (auto &argument : arguments) {
    argument_pointers.push_back(argument.data());
  }
  argc = static_cast<int>(argument_pointers.size());
  argument_pointers.push_back(nullptr);
  argv = argument_pointers.data();
  benchmark::Initialize(&argc, argv);
  if (benchmark::ReportUnrecognizedArguments(argc, argv)) {
    return 1;
  }
  benchmark::AddCustomContext("compiler", PROJECTILE_COMPILER);
  benchmark::AddCustomContext("build_configuration", PROJECTILE_BUILD_CONFIG);
  benchmark::AddCustomContext("seed", std::to_string(seed));
  benchmark::AddCustomContext("timestep", "Scalar(1) / Scalar(60)");
  benchmark::AddCustomContext("batch_ticks", std::to_string(batch_ticks));
  benchmark::AddCustomContext("initial_lifetime_seconds", "70");
  benchmark::AddCustomContext("gravity", "9.81 (cast to workload precision)");
  benchmark::AddCustomContext("float_projectile_bytes",
                              std::to_string(sizeof(pdef::Projectile)));
  benchmark::AddCustomContext("double_projectile_bytes",
                              std::to_string(sizeof(pdef::DoubleProjectile)));
  register_benchmarks();
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
}
