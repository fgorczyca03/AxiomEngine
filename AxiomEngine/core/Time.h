#pragma once

#include <chrono>

namespace axiom::core {

class Time {
  public:
    void Tick();

    [[nodiscard]] double DeltaSeconds() const { return deltaSeconds_; }
    [[nodiscard]] double ElapsedSeconds() const { return elapsedSeconds_; }

  private:
    using Clock = std::chrono::steady_clock;
    Clock::time_point start_{Clock::now()};
    Clock::time_point previous_{start_};
    double deltaSeconds_{0.0};
    double elapsedSeconds_{0.0};
};

} // namespace axiom::core
