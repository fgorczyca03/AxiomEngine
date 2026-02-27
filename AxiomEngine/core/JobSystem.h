#pragma once

#include <functional>
#include <string>
#include <vector>

namespace axiom::core {

enum class JobPriority {
    High,
    Normal,
    Low
};

struct JobDesc {
    std::string name{};
    JobPriority priority{JobPriority::Normal};
    std::function<void()> task{};
};

class IJobSystem {
  public:
    virtual ~IJobSystem() = default;
    virtual void Enqueue(JobDesc job) = 0;
    virtual void Flush() = 0;
    [[nodiscard]] virtual std::size_t WorkerCount() const = 0;
};

class InlineJobSystem final : public IJobSystem {
  public:
    void Enqueue(JobDesc job) override;
    void Flush() override;
    [[nodiscard]] std::size_t WorkerCount() const override;

  private:
    std::vector<JobDesc> queue_{};
};

} // namespace axiom::core
