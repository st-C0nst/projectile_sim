# Projectile tick benchmarks

Requires CMake 3.25+, Ninja, GLM, and the project's C++23 compiler/library.
CMake uses an installed Google Benchmark package (1.9.4 or newer), or downloads
v1.9.4 with a pinned SHA-256 checksum. The fallback disables upstream tests and
installation. See the [Google Benchmark guide](https://google.github.io/benchmark/user_guide.html)
for command-line options and measurement controls.

## Build and run

From the project root:

```sh
cmake --preset release
cmake --build --preset release
build/release/projectile_system --benchmark_list_tests
build/release/projectile_system --benchmark_display_aggregates_only=true \
  --benchmark_out=build/release/benchmark-baseline.json \
  --benchmark_out_format=json
```

Results stay in ignored `build/`. Defaults are five repetitions, 0.2 seconds of
warmup, and a minimum of 0.5 seconds measured CPU time per repetition.
CLI flags or corresponding `BENCHMARK_*` environment variables override these
defaults. Console aggregates show mean, median, standard deviation, and
coefficient of variation; JSON retains individual repetitions as well.

For a quick smoke run or one workload:

```sh
build/release/projectile_system --benchmark_repetitions=1 \
  --benchmark_min_warmup_time=0 --benchmark_min_time=0.01s
build/release/projectile_system --benchmark_filter='^Tick/double/100000$'
```

Use Release results for performance comparisons. Debug is useful for checking
the harness. Configure with `-DBUILD_BENCHMARKS=OFF` to build only correctness
tests, or `-DBUILD_TESTING=OFF` to build only benchmarks. Both options default to
ON and control their dependencies independently.

## What is measured

The six cases are `Tick/{float,double}/{1000,10000,100000}`. Each iteration is
one whole engine tick; the harness times batches of 60 ticks and reports
microseconds **per tick**, not per batch. `items_per_second` counts projectile
updates, not batches. It reports both elapsed and CPU time using Google
Benchmark's default single-thread CPU timing mode.

Inputs use seed 5000, positions in [-30, 30], velocities in [-10, 10], lifetime
70 seconds, and type 0. Random components are generated in double then cast to
the workload precision. Inputs match across precisions on the same standard
library; distribution implementations can differ between libraries. The
engine uses default gravity and `Scalar(1) / Scalar(60)` as the timestep.

Generation and initial construction happen before timing. Each batch resets
by reconstructing the engine from the initial vector with timing paused, then
advances one second of simulation. All entries remain active. A pointer escape
and memory barriers keep state updates observable to the optimizer. Barriers
and small batch-loop overhead are included in the measurement. The last batch
is checked for count, positive lifetime, and finite components after timing;
full motion correctness remains covered by Google Test.

Resetting allocates and copies memory outside timing and warms the state.
This baseline represents repeated updates to recently touched data. It does
not measure cold-cache startup, spawning, expiry-heavy populations, Godot state
transfer, or rendering.

## Save and compare a baseline

JSON includes compiler/version, build configuration, workload constants,
projectile structure sizes, and Google Benchmark's host/CPU/cache metadata.
Also record the source commit and whether local changes are present. Obtain
those with `git rev-parse HEAD` and `git status --short`, then supply context,
for example `--benchmark_context=commit=<hash>,working_tree=modified`.
Preserve `build/release/compile_commands.json` alongside shared results to
record exact compiler flags; the Release preset normally uses `-O3 -DNDEBUG`.

Compare the same workload, machine, compiler, flags, and library versions while
other heavy processes are idle. Use median CPU microseconds per tick and the
variation across repetitions; a small difference within the noise is weak
evidence of an improvement. Useful derived metrics are:

- Nanoseconds per projectile per tick: `cpu_microseconds * 1000 / count`.
- Fraction of one core at 60 Hz: `cpu_microseconds * 60 / 1,000,000`.
- Scaling: compare tick cost when count increases by 10x.
- Precision cost: compare double and float at the same count.

Use elapsed time when evaluating main-thread frame budget. Increased cost per
projectile at larger counts is a reason to profile cache/memory behavior, not
proof of a particular bottleneck. Run `perf` only after identifying a measured
question. No pass/fail timing threshold is defined by this first baseline.
