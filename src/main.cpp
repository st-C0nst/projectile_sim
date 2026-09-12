#include "projectile_sim/projectile_group.hpp"
#include "projectile_sim/simulation.hpp"
#include <chrono>
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
  requires std::is_scalar_v<Scalar>
Scalar distance(const Scalar &actual, const Scalar &expected) {
  return std::abs(static_cast<double>(actual) - static_cast<double>(expected));
}

template <typename Scalar>
  requires std::is_scalar_v<Scalar>
Scalar length(Scalar s) {
  return s;
}

template <pdef::ProjectileVector Vec3> Vec3 length(Vec3 v) {
  return glm::length(v);
}

template <typename T>
  requires pdef::ProjectileVector<T> || std::is_scalar_v<T>
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
                        const float sim_seconds, const std::uint32_t seed) {
  constexpr float max_velocity = 10.0f;
  constexpr float max_position = 30.0f;
  std::mt19937 gen(seed);
  std::uniform_real_distribution velocity_distro(-max_velocity, max_velocity);
  std::uniform_real_distribution position_distro(-max_position, max_position);

  pdef::Projectiles<Vec3> projectiles;
  projectiles.reserve(projectile_count);
  for (std::size_t i = 0; i < projectile_count; ++i) {
    projectiles.emplace_back(
        glm::vec3(position_distro(gen), position_distro(gen),
                  position_distro(gen)),
        glm::vec3(velocity_distro(gen), velocity_distro(gen),
                  velocity_distro(gen)),
        sim_seconds + 10.0f, 0);
  }
  return projectiles;
}

template <typename Vec3>
double tick_engine(psim::ProjectileEngine<Vec3> &engine, const int num_ticks,
                   const float dt) {

  const auto start = Clock::now();

  for (int tick = 0; tick < num_ticks; ++tick) {
    engine.tick(dt);
  }

  const auto end = Clock::now();

  return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {
  constexpr int tick_rate = 60;
  constexpr int sim_seconds = 60;
  constexpr int num_ticks = sim_seconds * tick_rate;
  constexpr float dt = 1.0f / tick_rate;
  constexpr std::uint32_t seed = 5000;
  constexpr std::size_t projectile_count = 100000;
  constexpr double abs_position_tolerance = 0.001f;
  constexpr double rel_position_tolerance = 0.0001f;
  constexpr double abs_velocity_tolerance = 0.001f;
  constexpr double rel_velocity_tolerance = 0.0001f;
  constexpr double abs_lifetime_tolerance = 0.001f;
  constexpr double rel_lifetime_tolerance = 0.001f;

  pdef::Projectiles<glm::vec3> projectiles =
      make_random_projectiles<glm::vec3>(projectile_count, sim_seconds, seed);

  psim::ProjectileEngine<glm::vec3> engine(projectiles);
  auto elapsed_ms = tick_engine(engine, num_ticks, dt);

  std::println("Time elapsed in test: {} ms", elapsed_ms);

  std::println("Total: {:.3f} ms | Average tick: {:.3f} us", elapsed_ms,
               elapsed_ms * 1000.0 / num_ticks);

  const glm::dvec3 acceleration{0.0, -engine.gravity(), 0.0};
  constexpr double total_time =
      static_cast<double>(num_ticks) * static_cast<double>(dt);

  for (const auto &[final_projectile, initial_projectile] :
       std::views::zip(engine.projectiles(), projectiles)) {

    pdef::DoubleProjectile expected_projectile =
        pdef::update_balistic(pdef::make_double_projectile(initial_projectile),
                              acceleration, total_time);

    // TODO should really collect these errors as formatted strings, need to tie
    // error to projectile with an id or something
    if (!approximately_equal(glm::dvec3(final_projectile.position),
                             expected_projectile.position,
                             abs_position_tolerance, rel_position_tolerance)) {
      std::println(
          "final position is not within toleranace. Final: {}, Expected: {}",
          final_projectile.position, expected_projectile.position);
    }

    if (!approximately_equal(glm::dvec3(final_projectile.velocity),
                             expected_projectile.velocity,
                             abs_velocity_tolerance, rel_velocity_tolerance)) {
      std::println(
          "final velocity is not within tolerance. Final: {}, Expected: {}",
          final_projectile.velocity, expected_projectile.velocity);
    }
    if (!approximately_equal(static_cast<double>(final_projectile.lifetime),
                             expected_projectile.lifetime,
                             abs_lifetime_tolerance, rel_lifetime_tolerance)) {
      std::println(
          "final lifetime is not within tolerance. Final: {}, Expected: {}",
          final_projectile.lifetime, expected_projectile.lifetime);
    }
    if (final_projectile.type != expected_projectile.type) {
      std::println(
          "final type does not match expected type. Final {}, Expected: {}",
          final_projectile.type, expected_projectile.type);
    }
  }

  return 0;
}
