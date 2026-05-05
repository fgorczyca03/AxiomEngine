#include "AxiomEngine/core/JobSystem.h"

#include "AxiomEngine/profiling/Profiling.h"

namespace axiom::core {

void InlineJobSystem::Enqueue(JobDesc job) {
    AXIOM_PROFILE_FUNCTION();
    queue_.push_back(std::move(job));
}

void InlineJobSystem::Flush() {
    AXIOM_PROFILE_FUNCTION();
    for (JobDesc& job : queue_) {
        AXIOM_PROFILE_ZONE(job.name.c_str());
        if (job.task) {
            job.task();
        }
    }
    queue_.clear();
}

std::size_t InlineJobSystem::WorkerCount() const { return 1; }

} // namespace axiom::core
