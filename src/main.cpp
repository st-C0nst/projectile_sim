#include "projectile_sim/projectile_group.hpp"
#include "projectile_sim/simulation.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <glm/geometric.hpp>
#include <print>
#include <random>
#include <ranges>
#include <span>
#include <tuple>

using Clock = std::chrono::steady_clock;

void print_projectiles(std::span<const Projectile> projectile_view) {
  for (const auto &projectile : projectile_view) {

    std::println("{}", projectile);
  }
}

[[nodiscard]]
inline Vec3 position(Vec3 initial_position, Vec3 initial_velocity,
                     Vec3 acceleration, float time) {
  return initial_position + initial_velocity * time +
         0.5f * acceleration * time * time;
}

[[nodiscard]]
inline Vec3 velocity(Vec3 initial_velocity, Vec3 acceleration, float time) {
  return initial_velocity + acceleration * time;
}

[[nodiscard]]
inline Projectile make_expected_projectile(Projectile initial_projectile,
                                           Vec3 acceleration, float time) {
  float expected_lifetime = initial_projectile.lifetime - time;
  Vec3 expected_position =
      position(initial_projectile.position, initial_projectile.velocity,
               acceleration, time);
  Vec3 expected_velocity =
      velocity(initial_projectile.velocity, acceleration, time);
  return {
      .position = expected_position,
      .velocity = expected_velocity,
      .lifetime = expected_lifetime,
      .type = initial_projectile.type,
  };
}

[[nodiscard]]
std::tuple<Vec3, Vec3, float, bool>
make_projectile_state_deviation(Projectile final_projectile,
                                Projectile initial_projectile,
                                Vec3 acceleration, float time) {
  Projectile expected_projectile =
      make_expected_projectile(initial_projectile, acceleration, time);
  return {
      final_projectile.position - expected_projectile.position,
      final_projectile.velocity - expected_projectile.velocity,
      final_projectile.lifetime - expected_projectile.lifetime,
      final_projectile.type == expected_projectile.type,
  };
}
// TODO set up way to validate between runs performance relative to other runs

// TODO add units if not obvious
[[nodiscard]]
std::tuple<bool, bool, bool, bool> // TODO make this api better, for now we just
                                   // validate every field
is_projectile_valid(Projectile final_projectile, Projectile initial_projectile,
                    Vec3 acceleration, float time, float position_tolerance,
                    float velocity_tolerance, float lifetime_tolerance) {
  const auto &[position_dev, velocity_dev, lifetime_dev, type_match] =
      make_projectile_state_deviation(final_projectile, initial_projectile,
                                      acceleration, time);
  return {
      glm::length(position_dev) <= position_tolerance,
      glm::length(velocity_dev) <= velocity_tolerance,
      std::abs(lifetime_dev) <= lifetime_tolerance,
      type_match,
  };
}

Projectiles make_random_projectiles(const std::size_t projectile_count,
                                    const float sim_seconds,
                                    const std::uint32_t seed) {
  constexpr float max_velocity = 10.0f;
  constexpr float max_position = 30.0f;
  std::mt19937 gen(seed);
  std::uniform_real_distribution velocity_distro(-max_velocity, max_velocity);
  std::uniform_real_distribution position_distro(-max_position, max_position);

  Projectiles projectiles;
  projectiles.reserve(projectile_count);
  for (std::size_t i = 0; i < projectile_count; ++i) {
    projectiles.emplace_back(
        Vec3(position_distro(gen), position_distro(gen), position_distro(gen)),
        Vec3(velocity_distro(gen), velocity_distro(gen), velocity_distro(gen)),
        sim_seconds + 10.0f, 0);
  }
  return projectiles;
}

constexpr double make_checksum(std::span<const Projectile> projectiles) {
  double checksum = 0.0;

  for (const auto &projectile : projectiles) {
    checksum += static_cast<double>(projectile.position.x) +
                static_cast<double>(projectile.position.y) +
                static_cast<double>(projectile.position.z);
  }
  return checksum;
}

template <typename... Ts> bool all_true(const std::tuple<Ts...> &values) {
  return std::apply(
      [](const auto &...xs) { return (static_cast<bool>(xs) && ...); }, values);
}

int main() {

  constexpr int tick_rate = 60;
  constexpr int sim_seconds = 60;
  constexpr int num_ticks = sim_seconds * tick_rate;
  constexpr float dt = 1.0f / tick_rate;
  constexpr std::uint32_t seed = 5000;
  constexpr std::size_t projectile_count = 100000;
  constexpr float position_tolerance = 0.0001;
  constexpr float velocity_tolerance = 0.0001;
  constexpr float lifetime_tolerance = 0.0001;

  Projectiles projectiles =
      make_random_projectiles(projectile_count, sim_seconds, seed);
  double prerun_checksum = make_checksum(projectiles);
  std::println("Prerun checksum: {}", prerun_checksum);
  psim::ProjectileEngine engine(projectiles);

  const auto start = Clock::now();

  for (int tick = 0; tick < num_ticks; ++tick) {
    engine.tick(dt);
  }

  const auto end = Clock::now();
  const double elapsed_ms =
      std::chrono::duration<double, std::milli>(end - start).count();

  std::println("Time elapsed in test: {} ms", elapsed_ms);
  double postrun_checksum = make_checksum(engine.projectiles());
  std::println("Final state checksum: {}", postrun_checksum);

  // TODO better alternative is to calc endstates after dt time steps and then
  // validate final state with some degree of tolerance
  std::println("Prerun to postrun checksum comparisons: {}",
               prerun_checksum != postrun_checksum);
  std::println("Total: {:.3f} ms | Average tick: {:.3f} us", elapsed_ms,
               elapsed_ms * 1000.0 / num_ticks);
  std::vector<std::tuple<bool, bool, bool, bool, Projectile, Projectile>>
      invalid_projectiles;

  const Vec3 acceleration{0.0f, -engine.gravity(), 0.0f};
  const float total_time = num_ticks * dt;

  for (const auto &[final_projectile, initial_projectile] :
       std::views::zip(engine.projectiles(), projectiles)) {

    const auto [position_ok, velocity_ok, lifetime_ok, type_ok] =
        is_projectile_valid(final_projectile, initial_projectile, acceleration,
                            total_time, position_tolerance, velocity_tolerance,
                            lifetime_tolerance);

    if (!(position_ok && velocity_ok && lifetime_ok && type_ok)) {
      invalid_projectiles.emplace_back(position_ok, velocity_ok, lifetime_ok,
                                       type_ok, final_projectile,
                                       initial_projectile);
    }
  }

  return 0;
}
