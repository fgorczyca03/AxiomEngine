#include "AxiomEngine/core/JobSystem.h"

#include "AxiomEngine/profiling/Profiling.h"

#include <algorithm>

#include <thread>

namespace axiom::core {

std::size_t InlineJobSystem::Enqueue(JobDesc job) {
    AXIOM_PROFILE_FUNCTION();
    queue_.push_back(std::move(job));
    return queue_.size() - 1;
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

ThreadPoolJobSystem::ThreadPoolJobSystem(std::optional<std::size_t> workerCount) {
    const std::size_t requested = workerCount.value_or(std::thread::hardware_concurrency());
    const std::size_t count = std::max<std::size_t>(1, requested);
    workers_.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        workers_.emplace_back([this]() { WorkerMain(); });
    }
}

ThreadPoolJobSystem::~ThreadPoolJobSystem() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
        cv_.notify_all();
    }

    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

std::size_t ThreadPoolJobSystem::Enqueue(JobDesc job) {
    std::lock_guard<std::mutex> lock(mutex_);
    const std::size_t id = queue_.size();
    queue_.push_back(std::move(job));
    remainingDependencies_.push_back(queue_[id].dependencies.size());
    dependents_.emplace_back();
    for (const std::size_t dependency : queue_[id].dependencies) {
        if (dependency < dependents_.size()) {
            dependents_[dependency].push_back(id);
        }
    }
    if (remainingDependencies_[id] == 0 && flushRequested_) {
        readyQueue_.push_back(id);
        cv_.notify_one();
    }
    return id;
}

void ThreadPoolJobSystem::Flush() {
    std::unique_lock<std::mutex> lock(mutex_);
    flushRequested_ = true;

    for (std::size_t id = 0; id < queue_.size(); ++id) {
        if (remainingDependencies_[id] == 0 && std::find(completed_.begin(), completed_.end(), id) == completed_.end()
            && std::find(running_.begin(), running_.end(), id) == running_.end()
            && std::find(readyQueue_.begin(), readyQueue_.end(), id) == readyQueue_.end()) {
            readyQueue_.push_back(id);
        }
    }
    cv_.notify_all();

    cv_.wait(lock, [this]() { return completed_.size() == queue_.size(); });

    queue_.clear();
    remainingDependencies_.clear();
    dependents_.clear();
    readyQueue_.clear();
    running_.clear();
    completed_.clear();
    flushRequested_ = false;
}

std::size_t ThreadPoolJobSystem::WorkerCount() const { return workers_.size(); }

void ThreadPoolJobSystem::WorkerMain() {
    while (true) {
        std::size_t jobId = 0;
        JobDesc job{};
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() { return stopping_ || (!readyQueue_.empty() && flushRequested_); });
            if (stopping_) {
                return;
            }
            jobId = readyQueue_.back();
            readyQueue_.pop_back();
            running_.push_back(jobId);
            job = queue_[jobId];
        }

        AXIOM_PROFILE_ZONE(job.name.c_str());
        if (job.task) {
            job.task();
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            running_.erase(std::remove(running_.begin(), running_.end(), jobId), running_.end());
            completed_.push_back(jobId);

            for (const std::size_t dependentId : dependents_[jobId]) {
                if (remainingDependencies_[dependentId] > 0) {
                    --remainingDependencies_[dependentId];
                }
                if (remainingDependencies_[dependentId] == 0) {
                    readyQueue_.push_back(dependentId);
                }
            }
            cv_.notify_all();
        }
    }
}

} // namespace axiom::core
