#pragma once

#include "projectile.hpp"
#include "projectile_pool.hpp"
#include <glm/vec3.hpp>

namespace psim {
inline constexpr double default_gravity = 9.81;

template <pdef::ProjectileVector Vec3> class ProjectileEngine {
public:
  using ScalarType = typename Vec3::value_type;
  using VectorType = Vec3;

  using Projectile = pdef::BaseProjectile<Vec3>;
  using ProjectileHandle = ppool::ProjectileHandle;

  explicit ProjectileEngine(
      std::size_t capacity,
      const ScalarType gravity = static_cast<ScalarType>(default_gravity))
      : projectile_pool_(capacity), gravity_(gravity) {}

  explicit ProjectileEngine(
      const pdef::Projectiles<Vec3> &projectiles,
      const ScalarType gravity = static_cast<ScalarType>(default_gravity))
      : ProjectileEngine(projectiles.size(), gravity) {
    for (const auto &projectile : projectiles) {
      (void)projectile_pool_.spawn(projectile);
    }
  }

  explicit ProjectileEngine(ProjectileEngine &&other) noexcept = default;
  ProjectileEngine &operator=(ProjectileEngine &&other) noexcept = default;

  // No copying
  ProjectileEngine(const ProjectileEngine &) = delete;
  ProjectileEngine &operator=(const ProjectileEngine &) = delete;

  [[nodiscard]]
  std::optional<ProjectileHandle> spawn(Projectile projectile) {
    return projectile_pool_.spawn(std::move(projectile));
  }

  [[nodiscard]]
  std::vector<ProjectileHandle>
  spawn_projectiles(std::span<const Projectile> projectiles) {
    return projectile_pool_.spawn_projectiles(projectiles);
  }

  [[nodiscard]]
  const Projectile *try_get(ProjectileHandle handle) const {
    return projectile_pool_.try_get(handle);
  }

  bool erase_remove(ProjectileHandle handle) {
    return projectile_pool_.erase_remove(handle);
  }

  [[nodiscard]]
  bool is_alive(ProjectileHandle handle) const {
    return projectile_pool_.is_alive(handle);
  }

  [[nodiscard]]
  std::size_t size() const { return projectile_pool_.size(); }

  // Borrowed view; mutations can invalidate pointers and change dense ordering.
  [[nodiscard]]
  std::span<const Projectile> projectiles() const noexcept {
    return projectile_pool_.projectiles();
  }

  void update_projectiles(const ScalarType dt) {
    for (auto &projectile : projectile_pool_.projectiles_) {
      if (projectile.lifetime <= 0) {
        continue;
      }

      projectile = pdef::update_balistic(
          projectile, Vec3{0, ScalarType{-1} * gravity_, 0}, dt);
    }
  }
  void tick(const ScalarType dt) { update_projectiles(dt); }

  [[nodiscard]]
  ScalarType gravity() const noexcept {
    return gravity_;
  }

private:
  ppool::ProjectilePool<Vec3> projectile_pool_;

  ScalarType gravity_ = default_gravity;
};

} // namespace psim
