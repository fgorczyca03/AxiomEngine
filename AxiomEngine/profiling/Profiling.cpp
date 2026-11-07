#include "AxiomEngine/profiling/Profiling.h"

namespace axiom::profiling {

namespace {
// The app owns the backend, so the profiling layer does not introduce a dependency on a profiler library.
IProfilerBackend* gBackend = nullptr;
}

void SetBackend(IProfilerBackend* backend) { gBackend = backend; }

IProfilerBackend* GetBackend() { return gBackend; }

ScopedZone::ScopedZone(const char* name, const char* file, int line) {
    if (gBackend != nullptr) {
        gBackend->BeginZone(name, file, line);
        active_ = true;
    }
}

ScopedZone::~ScopedZone() {
    // Only close zones that were actually opened; profiling can be toggled off between scopes.
    if (active_) {
        gBackend->EndZone();
    }
}

} // namespace axiom::profiling
