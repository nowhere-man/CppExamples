#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t numThreads)
    : m_shutdown(false), m_activeThreads(0), m_coreThreads(numThreads ? numThreads : std::thread::hardware_concurrency())
{
    m_workerThreads.reserve(m_coreThreads);
    std::lock_guard<std::mutex> lock(m_mutex);
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
        m_coreThreads = 0;
    }
    m_condition.notify_all();
    for (auto &t : m_workerThreads) {
        if (t.joinable()) {
            t.join();
        }
    }
    m_workerThreads.clear();
    m_activeThreads = 0;
}

void ThreadPool::Resize(size_t newSize)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_shutdown) {
        return;
    }
    if (newSize == 0) {
        newSize = std::thread::hardware_concurrency();
    }
    m_coreThreads = newSize;
    size_t current = m_workerThreads.size();
    if (newSize > current) {
        for (size_t i = 0; i < newSize - current; ++i) {
            m_workerThreads.emplace_back([this](){
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

            if (m_activeThreads > m_coreThreads) {
                --m_activeThreads;
                return;
            }

            if (m_shutdown && m_taskQueue.empty()) {
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
