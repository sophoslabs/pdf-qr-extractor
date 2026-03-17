#include "server/worker_pool.h"

namespace extractor {

WorkerPool::WorkerPool(size_t threadCount)
    : m_stopping(false)
{
    for (size_t i = 0; i < threadCount; ++i) {
        m_workers.emplace_back(&WorkerPool::workerLoop, this);
    }
}

WorkerPool::~WorkerPool()
{
    shutdown();
}

bool WorkerPool::enqueue(std::function<void()> task)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_stopping || m_tasks.size() >= MAX_QUEUE_SIZE)
        return false;

    m_tasks.push(std::move(task));
    m_cv.notify_one();
    return true;
}

void WorkerPool::shutdown()
{
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_stopping = true;
    }

    m_cv.notify_all();

    for (auto& t : m_workers) {
        if (t.joinable())
            t.join();
    }
}

void WorkerPool::workerLoop()
{
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [&] {
                return m_stopping || !m_tasks.empty();
            });

            if (m_stopping && m_tasks.empty())
                return;

            task = std::move(m_tasks.front());
            m_tasks.pop();
        }

        task();
    }
}

} // namespace extractor
