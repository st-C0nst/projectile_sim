# Simulator correctness tests

Requires CMake 3.25+, Ninja, GLM headers, and a C++23 compiler/library with
`std::format` and `std::print` support (also required by the existing project).
Google Test is loaded from an installed CMake package if available; otherwise
CMake downloads the pinned, SHA-256 verified v1.17.0 release on first configure.
The integration uses [Google Test's CMake workflow](https://google.github.io/googletest/quickstart-cmake.html).

From the project root:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

The `release` presets work the same way. Configure with `-DBUILD_TESTING=OFF`
to build only the timing executable, without fetching Google Test.
`projectile_system` now runs the timing workload from `benchmarks/main.cpp`;
correctness failures are reported by `projectile_tests` and CTest with failing
exit codes.

The suite runs deterministic cases for float and double vectors and randomized
accuracy checks with seed 5000 and 64 projectiles per run. It checks 30, 60, and
120 Hz at 60 seconds, and 1, 10, 60, and 600 seconds at 60 Hz. Expected motion
is calculated component by component in double precision, independently of the
simulation helpers, using the actual stored inputs, gravity, and tick duration.
Counts are checked before comparing states; retained input order identifies entries.

For nonnegative tick durations, entries with lifetime <= 0 stay unchanged.
Entries alive at tick start advance for the **entire tick**, including a tick
that crosses expiry; lifetime can become negative. They remain in the vector
and freeze on subsequent ticks. Exact-value tests cover both expiry boundaries.

Single-step checks use exact values or tight tolerances. Long-run motion checks
use an absolute plus relative error budget: float permits 0.01 + 0.0002 * abs(expected)
through 60 seconds and 0.02 + 0.001 * abs(expected) at 600 seconds; double permits
1e-8 + 1e-10 * abs(expected). Float lifetime has a separate absolute budget of
0.0002 * initial lifetime, plus the motion relative budget against expected
remaining lifetime. Repeated float updates accumulate rounding; these budgets
describe regression limits, not a guarantee of centimeter accuracy over ten minutes.
The ten-minute workload currently shows maximum float errors of about 199 position
units (with vertical positions around -1.76 million), 1.51 velocity units, and
0.105 seconds of lifetime drift. Double errors in the same workload are about
7.3e-7 position units, 1.7e-9 velocity units, and 2.3e-10 seconds of lifetime.

To save maximum absolute component errors for position, velocity, and lifetime
in a Google Test XML report:

```sh
build/debug/projectile_tests --gtest_output=xml:build/debug/test-results.xml
```
