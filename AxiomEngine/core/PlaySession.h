#pragma once

#include "AxiomEngine/ecs/ECSWorld.h"

#include <cstddef>
#include <optional>
#include <utility>

namespace axiom::core {

// Keep the complete edit world in memory, including entity IDs and unsaved data.
class PlaySession {
  public:
    bool Start(const ecs::ECSWorld& world) {
        if (IsPlaying()) return false;
        editWorld_.emplace(world.Clone());
        paused_ = false;
        stepRequested_ = false;
        tick_ = 0;
        return true;
    }

    bool Stop(ecs::ECSWorld& world) {
        if (!IsPlaying()) return false;
        world = std::move(*editWorld_);
        editWorld_.reset();
        paused_ = false;
        stepRequested_ = false;
        tick_ = 0;
        return true;
    }

    void SetPaused(bool paused) {
        if (IsPlaying()) paused_ = paused;
        stepRequested_ = false;
    }
    void RequestStep() { stepRequested_ = IsPaused(); }
    void DidStep() { ++tick_; stepRequested_ = false; }
    [[nodiscard]] bool IsPlaying() const { return editWorld_.has_value(); }
    [[nodiscard]] bool IsPaused() const { return IsPlaying() && paused_; }
    [[nodiscard]] bool ShouldStep() const { return IsPlaying() && (!paused_ || stepRequested_); }
    [[nodiscard]] std::size_t Tick() const { return tick_; }

  private:
    std::optional<ecs::ECSWorld> editWorld_{};
    bool paused_{false};
    bool stepRequested_{false};
    std::size_t tick_{0};
};

} // namespace axiom::core
