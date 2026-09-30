#include "projectile_sim/simulation.hpp"
#include <chrono>
#include <print>
#include <random>

// Timing only. Correctness checks live in tests/projectile_tests.cpp.
int main() {
  constexpr int ticks = 60 * 60;
  constexpr float dt = 1.0f / 60;
  std::mt19937 generator(5000);
  std::uniform_real_distribution<float> position(-30, 30);
  std::uniform_real_distribution<float> velocity(-10, 10);
  pdef::Projectiles<glm::vec3> projectiles;
  projectiles.reserve(100000);
  for (int i = 0; i < 100000; ++i) {
    projectiles.push_back(
        {{position(generator), position(generator), position(generator)},
         {velocity(generator), velocity(generator), velocity(generator)},
         70,
         0});
  }
  psim::ProjectileEngine<glm::vec3> engine(projectiles);
  const auto start = std::chrono::steady_clock::now();
  for (int tick = 0; tick < ticks; ++tick) {
    engine.tick(dt);
  }
  const double milliseconds = std::chrono::duration<double, std::milli>(
                                  std::chrono::steady_clock::now() - start)
                                  .count();
  std::println("Total: {:.3f} ms | Average tick: {:.3f} us", milliseconds,
               milliseconds * 1000 / ticks);
}
