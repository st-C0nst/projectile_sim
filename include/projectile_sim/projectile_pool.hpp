#pragma once

#include "projectile_sim/projectile.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <vector>

namespace psim {
template <pdef::ProjectileVector Vec3> class ProjectileEngine;
}

namespace ppool {

// TODO what is stopping us from growing past capacity
//

// ECS mappings
using SlotId = std::uint32_t;
using Generation = std::uint32_t;
struct ProjectileHandle {
  SlotId slot_id{0};
  Generation generation{0};
};

constexpr std::uint32_t invalid_index =
    std::numeric_limits<std::uint32_t>::max();
using DenseId = std::uint32_t;
struct Slot {
  Generation generation{0};
  DenseId dense_index = invalid_index;
};

template <pdef::ProjectileVector Vec3> class ProjectilePool {
public:
  explicit ProjectilePool(std::size_t capacity) {
    if (capacity > invalid_index) {
      throw std::length_error(
          std::format("Capacity {} cannot be larger than invalid index {}",
                      capacity, invalid_index));
    }
    projectiles_.reserve(capacity);
    slots_by_dense_.reserve(capacity);
    slots_.resize(capacity);

    free_slots_.reserve(capacity);
    for (const auto i :
         std::views::iota(std::size_t{0}, capacity) | std::views::reverse) {
      free_slots_.push_back(i);
    }
  }

  [[nodiscard]]
  std::optional<ProjectileHandle> spawn(pdef::BaseProjectile<Vec3> projectile) {
    const auto free_res = reuse_slot();
    if (!free_res.has_value()) {
      return {};
    }

    const auto slot_index = free_res.value();
    const auto dense_index = static_cast<DenseId>(projectiles_.size());

    projectiles_.push_back(std::move(projectile));
    slots_by_dense_.push_back(slot_index);

    Slot &slot = slots_[slot_index];
    slot.dense_index = dense_index;

    return ProjectileHandle{.slot_id = slot_index,
                            .generation = slot.generation};
  }

  [[nodiscard]]
  std::vector<ProjectileHandle>
  spawn_projectiles(std::span<const pdef::BaseProjectile<Vec3>> projectiles) {
    std::vector<ProjectileHandle> projectile_handles;
    projectile_handles.reserve(std::min(projectiles.size(), free_slots_.size()));

    for (const auto &projectile : projectiles) {
      auto handle = spawn(projectile);
      if (!handle)
        break;
      projectile_handles.push_back(*handle);
    }
    return projectile_handles;
  }

  const pdef::BaseProjectile<Vec3> *try_get(ProjectileHandle handle) const {
    if (!is_alive(handle)) {
      return nullptr;
    }

    return &projectiles_[slots_[handle.slot_id].dense_index];
  }

  bool erase_remove(ProjectileHandle handle) {
    if (!is_alive(handle)) {
      return false;
    }

    Slot &removed_slot = slots_[handle.slot_id];

    erase(removed_slot);
    remove(removed_slot, handle.slot_id);

    return true;
  }

  bool is_alive(ProjectileHandle handle) const {
    if (handle.slot_id >= slots_.size()) {
      return false;
    }

    const Slot &slot = slots_[handle.slot_id];

    return slot.dense_index != invalid_index &&
           handle.generation == slot.generation;
  }
  [[nodiscard]]
  std::size_t size() const {
    return projectiles_.size();
  }

  [[nodiscard]]
  std::span<const pdef::BaseProjectile<Vec3>> projectiles() const noexcept {
    return projectiles_;
  }

private:
  friend class psim::ProjectileEngine<Vec3>;
  void erase(Slot target_slot) {
    const DenseId removed_index = target_slot.dense_index;
    const DenseId last_index = static_cast<DenseId>(projectiles_.size() - 1);

    // Fill the gap unless we're already removing the last entry.
    if (removed_index != last_index) {
      projectiles_[removed_index] = std::move(projectiles_[last_index]);

      const SlotId moved_slot_index = slots_by_dense_[last_index];

      slots_by_dense_[removed_index] = moved_slot_index;

      // Repair the moved projectile's forward mapping.
      slots_[moved_slot_index].dense_index = removed_index;
    }
  }

  void remove(Slot &removed_slot, SlotId removed_slot_id) {

    projectiles_.pop_back();
    slots_by_dense_.pop_back();

    // Invalidate the removed projectile's handle.
    removed_slot.dense_index = invalid_index;

    if (removed_slot.generation != std::numeric_limits<Generation>::max()) {
      ++removed_slot.generation;
      free_slots_.push_back(removed_slot_id);
    }
    // If generation num has hit limit, retire slot
  }
  std::optional<SlotId> reuse_slot() {
    if (free_slots_.empty()) {
      return {};
    }

    const SlotId slot_index = free_slots_.back();
    free_slots_.pop_back();
    return slot_index;
  }

  pdef::Projectiles<Vec3> projectiles_;
  std::vector<SlotId> free_slots_;
  std::vector<SlotId> slots_by_dense_;
  std::vector<Slot> slots_;
};

} // namespace ppool
