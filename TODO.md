# Next steps

## Correctness and reusable types

- [ ] Separate correctness tests from timing into `tests/` and `benchmarks/`.
- [ ] Return a failing exit code when validation fails.
- [ ] Check initial/final state sizes before comparing zipped elements.
- [ ] Generalize projectile conversion, random generation, time steps, and state validation for both `glm::vec3` and `glm::dvec3`.
- [ ] Define expiry semantics, including when lifetime ends partway through a tick, and test them.
- [ ] Decide how expired projectiles are removed and how identity is preserved when comparing states.
- [ ] Add independently calculated cases for zero time, zero gravity, known ballistic motion, and both precisions.
- [ ] Keep long-duration accuracy checks; vary tick rate at fixed duration and duration at fixed tick rate.

## Benchmark baseline

- [ ] Add a shared CMake `INTERFACE` target for simulation includes and C++ requirements.
- [ ] Add a Google Benchmark executable measuring whole engine ticks for 1,000, 10,000, and 100,000 projectiles in both precisions.
- [ ] Keep generation, resetting, validation, and logging outside tick timing; measure those separately if needed.
- [ ] Use fixed tick batches and reset between batches so projectiles do not expire during an all-active benchmark.
- [ ] Use repetitions and appropriate optimization barriers, and record build configuration and workload with results.

## Integration and profiling

- [ ] Build a minimal Godot integration and measure simulation, state transfer, and rendering separately.
- [ ] Define ownership/synchronization for render state; a const span does not provide a snapshot.
- [ ] Add other projectile behaviors as concrete gameplay requirements emerge, with corresponding tests and benchmarks.
- [ ] Use `perf` to investigate measured bottlenecks, then compare changes against the baseline.
