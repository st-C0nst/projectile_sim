#pragma once
#include <concepts>
#include <cstdint>
#include <format>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/vec3.hpp>
#include <type_traits>
#include <utility>
#include <vector>

namespace pdef {

using ProjectileId = std::uint16_t;

/* TODO */
// Should seperate vectors based on variants, then process each at same time
// removes branching
// Balstic, Drag, Homing, Linear, vector for each update functions for each

enum class MotionModel : std::uint8_t { Balistic };

struct ProjectileType {
  float mass;
};

template <typename Vec3>
concept ProjectileVector =
    std::same_as<Vec3, glm::vec3> || std::same_as<Vec3, glm::dvec3>;

template <ProjectileVector Vec3 = glm::vec3> struct BaseProjectile {
  using ScalarType = typename Vec3::value_type;

  Vec3 position{0};
  Vec3 velocity{0};
  ScalarType lifetime{0};
  ProjectileId type{0};
};

// Aliases
using Projectile = BaseProjectile<>;
using DoubleProjectile = BaseProjectile<glm::dvec3>;
template <ProjectileVector Vec3>
using Projectiles = std::vector<BaseProjectile<Vec3>>;

// Utils

template <ProjectileVector ToVec, ProjectileVector FromVec>
[[nodiscard]]
BaseProjectile<ToVec> projectile_cast(const BaseProjectile<FromVec> &p) {
  using ToScalar = typename ToVec::value_type;
  return {
      .position = ToVec(p.position),
      .velocity = ToVec(p.velocity),
      .lifetime = static_cast<ToScalar>(p.lifetime),
      .type = p.type,
  };
}

template <ProjectileVector Vec3>
[[nodiscard]]
inline Vec3 make_position(const BaseProjectile<Vec3> &initial_projectile,
                          const std::type_identity_t<Vec3> &acceleration,
                          const typename Vec3::value_type time) {
  using Scalar = typename Vec3::value_type;
  return initial_projectile.position + initial_projectile.velocity * time +
         Scalar{0.5} * acceleration * time * time;
}

template <ProjectileVector Vec3>
[[nodiscard]]
inline Vec3 make_velocity(const BaseProjectile<Vec3> &initial_projectile,
                          const std::type_identity_t<Vec3> &acceleration,
                          const typename Vec3::value_type time) {
  return initial_projectile.velocity + acceleration * time;
}

template <ProjectileVector Vec3>
inline BaseProjectile<Vec3>
update_balistic(const BaseProjectile<Vec3> &projectile,
                const std::type_identity_t<Vec3> &gravity,
                typename Vec3::value_type dt) {

  Vec3 new_position = make_position(projectile, gravity, dt);
  Vec3 new_velocity = make_velocity(projectile, gravity, dt);

  return BaseProjectile<Vec3>{
      .position = new_position,
      .velocity = new_velocity,
      .lifetime = projectile.lifetime - dt,
      .type = projectile.type,
  };
}
} // namespace pdef

template <glm::length_t L, typename T, glm::qualifier Q>
struct std::formatter<glm::vec<L, T, Q>> {
  std::formatter<T> component_formatter;

  constexpr auto parse(std::format_parse_context &ctx) {
    return component_formatter.parse(ctx);
  }

  auto format(const glm::vec<L, T, Q> &vec, std::format_context &ctx) const {
    auto out = std::format_to(ctx.out(), "(");

    auto write_component = [&](auto index) {
      if (index != 0) {
        out = std::format_to(out, ", ");
      }

      ctx.advance_to(out);
      out = component_formatter.format(vec[index], ctx);
    };

    [&]<std::size_t... I>(std::index_sequence<I...>) {
      (write_component(I), ...);
    }(std::make_index_sequence<L>{});
    return std::format_to(out, ")");
  }
};

template <pdef::ProjectileVector Vec3>
struct std::formatter<pdef::BaseProjectile<Vec3>>
    : std::formatter<std::string_view> {
  auto format(const pdef::BaseProjectile<Vec3> &p,
              std::format_context &ctx) const {
    auto text = std::format("Projectile {{ pos {:.3f}, "
                            "vel {:.3f}, "
                            "lifetime {:.3f}, "
                            "type {}"
                            "}}",
                            p.position, p.velocity, p.lifetime, p.type);
    return std::formatter<std::string_view>::format(text, ctx);
  }
};
