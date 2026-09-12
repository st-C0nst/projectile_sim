#include "projectile_sim/projectile_group.hpp"
#include "projectile_sim/simulation.hpp"
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <numbers>
#include <print>
#include <random>
#include <ranges>
#include <span>
#include <type_traits>

using Clock = std::chrono::steady_clock;

// TODO find way to print out vec in an easy manner, can prob fold expression
// it?

// TODO should prob seperate tests which measure accuracy vs tests which measure
// performance
//
// TODO use doubles for validation. also allow template for projectile type to
// use diff vectors
//
// TOOD mult accuracy runs and vary the tick rate. sees if accuracy depends on
// frame rate. for every run record largest error across projectiles
//
// TODO test where we hold 60hz and try different durations, shows how error
// develops over longer simulations.
//
template <pdef::ProjectileVector Vec3>
typename Vec3::value_type distance(const Vec3 &actual, const Vec3 &expected) {
  return glm::length(actual - expected);
}
template <typename Scalar>
  requires std::floating_point<Scalar>
Scalar distance(const Scalar &actual, const Scalar &expected) {
  return std::abs(static_cast<double>(actual) - static_cast<double>(expected));
}

template <typename Scalar>
  requires std::floating_point<Scalar>
Scalar length(Scalar s) {
  return std::abs(s);
}

template <pdef::ProjectileVector Vec3> Vec3::value_type length(Vec3 v) {
  return glm::length(v);
}

template <typename T>
  requires pdef::ProjectileVector<T> || std::floating_point<T>
bool approximately_equal(
    const T &actual, const T &expected,
    decltype(distance(actual, expected)) absolute_tolerance,
    decltype(distance(actual, expected)) relative_tolerance) {
  using ToleranceType = decltype(distance(actual, expected));
  const ToleranceType error = distance(actual, expected);
  const ToleranceType allowed_error =
      absolute_tolerance + relative_tolerance * length(expected);

  return error <= allowed_error;
}

template <pdef::ProjectileVector Vec3>
void print_projectiles(
    std::span<const pdef::BaseProjectile<Vec3>> projectile_view) {
  for (const auto &projectile : projectile_view) {

    std::println("{}", projectile);
  }
}

template <pdef::ProjectileVector Vec3>
[[nodiscard]]
pdef::Projectiles<Vec3>
make_random_projectiles(const std::size_t projectile_count,
                        const typename Vec3::value_type sim_seconds,
                        const std::uint32_t seed) {
  using Scalar = typename Vec3::value_type;

  std::mt19937 gen(seed);
  std::uniform_real_distribution velocity_distro(Scalar{-10}, Scalar{10});
  std::uniform_real_distribution position_distro(Scalar{-30}, Scalar{30});

  pdef::Projectiles<Vec3> projectiles;
  projectiles.reserve(projectile_count);
  for (std::size_t i = 0; i < projectile_count; ++i) {
    projectiles.emplace_back(
        Vec3{position_distro(gen), position_distro(gen), position_distro(gen)},
        Vec3{velocity_distro(gen), velocity_distro(gen), velocity_distro(gen)},
        sim_seconds + 10.0f, 0);
  }
  return projectiles;
}

template <typename Vec3>
double tick_engine(psim::ProjectileEngine<Vec3> &engine, const int num_ticks,
                   const typename Vec3::value_type dt) {

  const auto start = Clock::now();

  for (int tick = 0; tick < num_ticks; ++tick) {
    engine.tick(dt);
  }

  const auto end = Clock::now();

  return std::chrono::duration<double, std::milli>(end - start).count();
}

template <typename T>
  requires std::floating_point<T>
struct Tolerance {
  T abs_position_tolerance = 0.001f;
  T rel_position_tolerance = 0.0001f;
  T abs_velocity_tolerance = 0.001f;
  T rel_velocity_tolerance = 0.0001f;
  T abs_lifetime_tolerance = 0.001f;
  T rel_lifetime_tolerance = 0.0001f;
};

template <typename T = double>
  requires std::floating_point<T>
struct RunConfig {
  int tick_rate;
  int sim_seconds;
  int num_ticks;
  T dt;
  std::uint32_t seed;
  std::size_t projectile_count;
};

// TODO for benchmarks, maybe a config run struct: struct RunConfig {};

template <pdef::ProjectileVector Vec3>
std::vector<std::string>
valid_projectile(const pdef::BaseProjectile<Vec3> &final_projectile,
                 const pdef::BaseProjectile<Vec3> &initial_projectile,
                 const Tolerance<double> &tolerances,
                 const glm::dvec3 &acceleration, const double total_time) {

  pdef::DoubleProjectile expected_projectile = pdef::update_balistic(
      pdef::projectile_cast<glm::dvec3>(initial_projectile), acceleration,
      total_time);

  std::vector<std::string> results{};

  if (!approximately_equal(glm::dvec3(final_projectile.position),
                           expected_projectile.position,
                           tolerances.abs_position_tolerance,
                           tolerances.rel_position_tolerance)) {
    results.emplace_back(std::format(
        "final position is not within toleranace. Final: {}, Expected: {}",
        final_projectile.position, expected_projectile.position));
  }

  if (!approximately_equal(glm::dvec3(final_projectile.velocity),
                           expected_projectile.velocity,
                           tolerances.abs_velocity_tolerance,
                           tolerances.rel_velocity_tolerance)) {
    results.emplace_back(std::format(
        "final velocity is not within tolerance. Final: {}, Expected: {}",
        final_projectile.velocity, expected_projectile.velocity));
  }
  if (!approximately_equal(static_cast<double>(final_projectile.lifetime),
                           expected_projectile.lifetime,
                           tolerances.abs_lifetime_tolerance,
                           tolerances.rel_lifetime_tolerance)) {
    results.emplace_back(std::format(
        "final lifetime is not within tolerance. Final: {}, Expected: {}",
        final_projectile.lifetime, expected_projectile.lifetime));
  }
  if (final_projectile.type != expected_projectile.type) {
    results.emplace_back(std::format(
        "final type does not match expected type. Final {}, Expected: {}",
        final_projectile.type, expected_projectile.type));
  }
  return results;
}

// add formatting for this
struct ValidationResult {
  std::size_t error_count{0};
  std::vector<std::vector<std::string>> projectile_errors;
};

template <pdef::ProjectileVector Vec3>
[[nodiscard]]
ValidationResult has_correct_state(
    std::span<const pdef::BaseProjectile<Vec3>> initial_projectiles,
    std::span<const pdef::BaseProjectile<Vec3>> final_projectiles,
    Tolerance<double> tolerances, const glm::dvec3 &gravity,
    const double time) {

  std::vector<std::vector<std::string>> results;
  results.reserve(initial_projectiles.size());

  std::size_t err_count = 0;
  for (const auto &[final_projectile, initial_projectile] :
       std::views::zip(final_projectiles, initial_projectiles)) {
    auto res = valid_projectile(final_projectile, initial_projectile,
                                tolerances, gravity, time);
    if (!res.empty()) {
      ++err_count;
    }
    results.emplace_back(res);
  }
  return {.error_count = err_count, .projectile_errors = results};
}

template <typename Scalar>
  requires std::floating_point<Scalar>
constexpr RunConfig<Scalar> default_config() {

  constexpr int tick_rate = 60;
  constexpr int sim_seconds = 60;
  constexpr int num_ticks = sim_seconds * tick_rate;
  constexpr Scalar dt = Scalar{1.0} / static_cast<Scalar>(tick_rate);

  return {
      .tick_rate = tick_rate,
      .sim_seconds = sim_seconds,
      .num_ticks = num_ticks,
      .dt = dt,
      .seed = 5000,
      .projectile_count = 100000,
  };
}

int main() {
  using Vec3 = glm::vec3;
  using Scalar = typename Vec3::value_type;

  constexpr auto run_config = default_config<Scalar>();
  constexpr Tolerance<double> tolerances{};

  pdef::Projectiles<Vec3> projectiles = make_random_projectiles<Vec3>(
      run_config.projectile_count, run_config.sim_seconds, run_config.seed);

  psim::ProjectileEngine<Vec3> engine(projectiles);
  auto elapsed_ms = tick_engine(engine, run_config.num_ticks, run_config.dt);

  std::println("Time elapsed in test: {} ms", elapsed_ms);

  std::println("Total: {:.3f} ms | Average tick: {:.3f} us", elapsed_ms,
               elapsed_ms * 1000.0 / run_config.num_ticks);

  const glm::dvec3 acceleration{0.0, -engine.gravity(), 0.0};
  constexpr double total_time = static_cast<double>(run_config.num_ticks) *
                                static_cast<double>(run_config.dt);

  auto result = has_correct_state<Vec3>(projectiles, engine.projectiles(),
                                        tolerances, acceleration, total_time);
  // std::println("{}", result);
  for (const auto &projectile_errors : result.projectile_errors) {
    if (projectile_errors.empty()) {
      continue;
    }
    std::println("Invalid Projectile found");
    for (const auto &err : projectile_errors) {
      std::println("{}", err);
    }
  }

  if (result.error_count > 0) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
