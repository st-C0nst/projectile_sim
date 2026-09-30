#pragma once

#include "projectile_group.hpp"
#include <glm/vec3.hpp>
#include <span>

/* TODO */
// membership in vector should imply if its alive, batch cleanup
// operation. method to pawn new projectiles

namespace psim {
inline constexpr double default_gravity = 9.81;

template <pdef::ProjectileVector Vec3> class ProjectileEngine {
public:
  using ScalarType = typename Vec3::value_type;
  using VectorType = Vec3;

  ProjectileEngine() = delete;

  explicit ProjectileEngine(
      pdef::Projectiles<Vec3> &projectiles,
      const ScalarType gravity = static_cast<ScalarType>(default_gravity))
      : projectiles_(projectiles), gravity_(gravity) {};

  explicit ProjectileEngine(ProjectileEngine &&other) noexcept = default;
  ProjectileEngine &operator=(ProjectileEngine &&other) noexcept = default;

  // No copying
  ProjectileEngine(const ProjectileEngine &) = delete;
  ProjectileEngine operator=(const ProjectileEngine &) = delete;

  void update_projectiles(const ScalarType dt) {
    for (auto &projectile : projectiles_) {
      if (projectile.lifetime <= 0) {
        continue;
      }

      projectile = pdef::update_balistic(
          projectile, Vec3{0, ScalarType{-1} * gravity_, 0}, dt);
    }
  }
  void tick(const ScalarType dt) { update_projectiles(dt); }
  [[nodiscard]]
  std::span<const pdef::BaseProjectile<Vec3>> projectiles() const {
    return {projectiles_.data(), projectiles_.size()};
  }

  [[nodiscard]]
  ScalarType gravity() const noexcept {
    return gravity_;
  }

private:
  pdef::Projectiles<Vec3> projectiles_;
  ScalarType gravity_ = default_gravity;
};

} // namespace psim
