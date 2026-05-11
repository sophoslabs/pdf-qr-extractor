// Copyright (C) 2026 Sophos Limited
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This file is part of pdf-qr-extractor.
//
// pdf-qr-extractor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// pdf-qr-extractor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with pdf-qr-extractor. If not, see <https://www.gnu.org/licenses/>

#include "server/worker_pool.h"
#include "logging/logger.h"

#include <chrono>
#include <iostream>
#include <thread>

namespace extractor {

WorkerPool::WorkerPool(size_t threadCount)
    : m_stopping(false),
      m_maxQueueSize(threadCount * 4)
{
    for (size_t i = 0; i < threadCount; ++i) {
        m_workers.emplace_back([this, i]() {
            workerLoop(i);
        });
    }
}

WorkerPool::~WorkerPool()
{
    shutdown();
}

bool WorkerPool::enqueue(std::function<void()> task)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_stopping) {
        LOG_WARN("WORKER", "Rejecting task: pool stopping");
        return false;
    }

    if (m_tasks.size() >= m_maxQueueSize) {
        LOG_WARN("WORKER", "Queue full, rejecting task");
        return false;
    }

    m_tasks.push(std::move(task));
    m_cv.notify_one();
    return true;
}

void WorkerPool::shutdown()
{
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_stopping)
            return;

        m_stopping = true;
    }

    m_cv.notify_all();

    for (auto& t : m_workers) {
        if (t.joinable())
            t.join();
    }
}

void WorkerPool::workerLoop(size_t idx)
{
    (void)idx;

    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(m_mutex);

            m_cv.wait(lock, [this] {
                return m_stopping || !m_tasks.empty();
            });

            if (m_stopping && m_tasks.empty())
                return;

            task = std::move(m_tasks.front());
            m_tasks.pop();
        }

        try {
            task();
        } catch (const std::exception& e) {
            try { LOG_ERROR("WORKER", std::string("Unhandled exception: ") + e.what()); }
            catch (...) { std::cerr << "[WORKER] Logger threw while reporting exception: " << e.what() << std::endl; }
        } catch (...) {
            try { LOG_ERROR("WORKER", "Unknown unhandled exception"); }
            catch (...) { std::cerr << "[WORKER] Logger threw while reporting unknown exception" << std::endl; }
        }
    }
}

} // namespace extractor
