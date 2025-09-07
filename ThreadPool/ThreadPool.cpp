#include "ThreadPool.h"

#include <algorithm>

ThreadPool::ThreadPool(size_t numThreads) : m_shutdown(false)
{
    m_coreThreads = std::min(THREADS_COUNT_MAX, std::max(THREADS_COUNT_MIN, numThreads));
    for (size_t i = 0; i < m_coreThreads; ++i) {
        m_workerThreads.emplace_back([this, i]() {
            Worker(i);
        });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_shutdown) {
            return;
        }
        m_shutdown = true;
    }
    m_condition.notify_all();
    for (auto& t : m_workerThreads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void ThreadPool::Resize(size_t newSize)
{
    newSize = std::min(THREADS_COUNT_MAX, std::max(THREADS_COUNT_MIN, newSize));

    std::lock_guard<std::mutex> lock(m_mutex);
    m_coreThreads = newSize;

    size_t current_size = m_workerThreads.size();
    if (newSize > current_size) {
        for (size_t i = current_size; i < newSize; ++i) {
            m_workerThreads.emplace_back([this, i]() {
                Worker(i);
            });
        }
    }
    m_condition.notify_all();
}

void ThreadPool::Worker(size_t index)
{
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.wait(lock, [this, index] {
                return m_shutdown || (!m_taskQueue.empty() && index < m_coreThreads);
            });

            if (m_shutdown && m_taskQueue.empty()) {
                return;
            }

            if (!m_taskQueue.empty()) {
                task = std::move(m_taskQueue.front());
                m_taskQueue.pop();
            }
        }
        if (task) {
            task();
        }
    }
}