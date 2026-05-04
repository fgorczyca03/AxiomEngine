#pragma once

namespace axiom::profiling {

class IProfilerBackend {
  public:
    virtual ~IProfilerBackend() = default;
    virtual void BeginZone(const char* name, const char* file, int line) = 0;
    virtual void EndZone() = 0;
    virtual void FrameMark() = 0;
};

void SetBackend(IProfilerBackend* backend);
IProfilerBackend* GetBackend();

class ScopedZone {
  public:
    ScopedZone(const char* name, const char* file, int line);
    ~ScopedZone();

    ScopedZone(const ScopedZone&) = delete;
    ScopedZone& operator=(const ScopedZone&) = delete;

  private:
    bool active_{false};
};

} // namespace axiom::profiling

#define AXIOM_PROFILE_ZONE(name) ::axiom::profiling::ScopedZone axiomScopedZone##__LINE__{name, __FILE__, __LINE__}
#define AXIOM_PROFILE_FUNCTION() AXIOM_PROFILE_ZONE(__func__)
#define AXIOM_PROFILE_FRAME_MARK()                     \
    do {                                               \
        if (::axiom::profiling::GetBackend()) {       \
            ::axiom::profiling::GetBackend()->FrameMark(); \
        }                                              \
    } while (false)
