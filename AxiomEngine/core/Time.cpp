#include "AxiomEngine/core/Time.h"

namespace axiom::core {

void Time::Tick() {
    // Sample once so this frame's delta and total runtime share the same boundary.
    const auto now = Clock::now();
    deltaSeconds_ = std::chrono::duration<double>(now - previous_).count();
    elapsedSeconds_ = std::chrono::duration<double>(now - start_).count();
    previous_ = now;
}

} // namespace axiom::core
