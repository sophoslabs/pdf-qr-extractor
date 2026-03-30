#pragma once

#include <functional>
#include <queue>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace extractor {

class WorkerPool {
public:
    explicit WorkerPool(size_t threadCount);
    ~WorkerPool();

    // Enqueue work; returns false if pool is stopping or overloaded
    bool enqueue(std::function<void()> task);

    void shutdown();

private:
    void workerLoop();

    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_stopping;

    static constexpr size_t MAX_QUEUE_SIZE = 128;
};

} // namespace extractor
