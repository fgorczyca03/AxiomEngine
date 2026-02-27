#include "AxiomEngine/profiling/Profiling.h"

namespace axiom::profiling {

namespace {
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
    if (active_) {
        gBackend->EndZone();
    }
}

} // namespace axiom::profiling
