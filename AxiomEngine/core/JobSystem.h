#pragma once

#include <functional>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
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
    std::vector<std::size_t> dependencies{};
};

class IJobSystem {
  public:
    virtual ~IJobSystem() = default;
    virtual std::size_t Enqueue(JobDesc job) = 0;
    virtual void Flush() = 0;
    [[nodiscard]] virtual std::size_t WorkerCount() const = 0;
};

class InlineJobSystem final : public IJobSystem {
  public:
    std::size_t Enqueue(JobDesc job) override;
    void Flush() override;
    [[nodiscard]] std::size_t WorkerCount() const override;

  private:
    std::vector<JobDesc> queue_{};
};

class ThreadPoolJobSystem final : public IJobSystem {
  public:
    explicit ThreadPoolJobSystem(std::optional<std::size_t> workerCount = std::nullopt);
    ~ThreadPoolJobSystem() override;

    std::size_t Enqueue(JobDesc job) override;
    void Flush() override;
    [[nodiscard]] std::size_t WorkerCount() const override;

  private:
    void WorkerMain();

    std::vector<JobDesc> queue_{};
    std::vector<std::size_t> remainingDependencies_{};
    std::vector<std::vector<std::size_t>> dependents_{};
    std::vector<std::size_t> readyQueue_{};
    std::vector<std::size_t> running_{};
    std::vector<std::size_t> completed_{};
    std::vector<std::thread> workers_{};
    std::mutex mutex_{};
    std::condition_variable cv_{};
    bool stopping_{false};
    bool flushRequested_{false};
};

} // namespace axiom::core
