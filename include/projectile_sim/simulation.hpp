#pragma once

#include "projectile_group.hpp"
#include <glm/vec3.hpp>
#include <span>

/* TODO */
// membership in vector should imply if its alive, batch cleanup
// operation

namespace psim {
inline constexpr float default_gravity = 9.81f;

using Vec3 = glm::vec3;

inline Projectile update_balistic(const Projectile &projectile, float gravity,
                                  float dt) {

  Vec3 new_position = Vec3(0, -0.5 * gravity * dt * dt, 0) +
                      projectile.velocity * dt + projectile.position;
  Vec3 new_velocity = Vec3(0, -1 * gravity * dt, 0) + projectile.velocity;
  return Projectile{
      .position = new_position,
      .velocity = new_velocity,
      .lifetime = projectile.lifetime - dt,
      .type = projectile.type,
  };
}

class ProjectileEngine {
public:
  // Must initialize data
  ProjectileEngine() = delete;

  explicit ProjectileEngine(Projectiles &projectiles,
                            const float gravity = default_gravity)
      : projectiles_(projectiles), gravity_(gravity) {};

  explicit ProjectileEngine(ProjectileEngine &&other) noexcept = default;
  ProjectileEngine &operator=(ProjectileEngine &&other) noexcept = default;

  // No copying
  ProjectileEngine(const ProjectileEngine &) = delete;
  ProjectileEngine operator=(const ProjectileEngine &) = delete;

  void update_projectiles(float dt) {
    for (auto &projectile : projectiles_) {
      if (projectile.lifetime <= 0) {
        continue;
      }

      projectile = update_balistic(projectile, gravity_, dt);
    }
  }
  void tick(float dt) { update_projectiles(dt); }
  [[nodiscard]]
  std::span<const Projectile> projectiles() {
    return {projectiles_.data(), projectiles_.size()};
  }

  [[nodiscard]]
  float gravity() const noexcept {
    return gravity_;
  }

private:
  Projectiles projectiles_;
  float gravity_ = default_gravity;
};

} // namespace psim
