#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t numThreads) : m_shutdown(false), m_activeThreads(0)
{
    m_coreThreads = std::min(THREADS_COUNT_MAX, std::max(THREADS_COUNT_MIN, numThreads));
    m_workerThreads.reserve(m_coreThreads);

    for (size_t i = 0; i < m_coreThreads; ++i) {
        m_workerThreads.emplace_back([this]() {
            Worker();
        });
        ++m_activeThreads;
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
    for (auto &t : m_workerThreads) {
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
    if (newSize > m_activeThreads) {
        for (size_t i = m_activeThreads; i < newSize; ++i) {
            m_workerThreads.emplace_back([this]() {
                Worker();
            });
            ++m_activeThreads;
        }
    }
    m_condition.notify_all();
}

void ThreadPool::Worker()
{
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.wait(lock, [this] {
                return m_shutdown || !m_taskQueue.empty() || m_activeThreads > m_coreThreads;
            });

            if ((m_shutdown || m_activeThreads > m_coreThreads) && m_taskQueue.empty()) {
                --m_activeThreads;
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