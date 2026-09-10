#pragma once
#include <cstdint>
#include <format>
#include <glm/vec3.hpp>
#include <vector>

using Vec3 = glm::vec3;
using ProjectileId = std::uint16_t;

/* TODO */
// Should seperate vectors based on variants, then process each at same time
// removes branching
// Balstic, Drag, Homing, Linear, vector for each update functions for each

enum class MotionModel : std::uint8_t { Balistic };

struct ProjectileType {
  float mass;
};

struct Projectile {
  Vec3 position;
  Vec3 velocity;
  float lifetime;
  ProjectileId type;
};

using Projectiles = std::vector<Projectile>;

template <>
struct std::formatter<Projectile> : std::formatter<std::string_view> {
  auto format(const Projectile &p, std::format_context &ctx) const {
    auto text =
        std::format("Projectile {{ pos {{ {:.3f}, {:.3f} {:.3f} }}, "
                    "vel {{ {:.3f}, {:.3f}, {:.3f} }}, "
                    "lifetime {:.3f}, "
                    "type {}"
                    "}}",
                    p.position.x, p.position.y, p.position.z, p.velocity.x,
                    p.velocity.y, p.velocity.z, p.lifetime, p.type);
    return std::formatter<std::string_view>::format(text, ctx);
  }
};
