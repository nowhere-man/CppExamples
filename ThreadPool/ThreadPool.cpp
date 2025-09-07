#include "ThreadPool.h"

#include <algorithm>
#include <unordered_set>

ThreadPool::ThreadPool(size_t numThreads) : m_shutdown(false)
{
    m_coreThreads = std::min(THREADS_COUNT_MAX, std::max(THREADS_COUNT_MIN, numThreads));
    m_workerThreads.reserve(m_coreThreads);
    for (size_t i = 0; i < m_coreThreads; ++i) {
        m_workerThreads.emplace_back([this]() {
            Worker();
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

void ThreadPool::RemoveUnusedThreads()
{
    for (auto& thread : m_workerThreads) {
        if (m_threadsToJoin.count(thread.get_id())) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    auto first_to_remove = std::remove_if(m_workerThreads.begin(), m_workerThreads.end(), [](const std::thread& t) {
        return !t.joinable();
    });
    m_workerThreads.erase(first_to_remove, m_workerThreads.end());
    m_threadsToJoin.clear();
}

void ThreadPool::Resize(size_t newSize)
{
    newSize = std::min(THREADS_COUNT_MAX, std::max(THREADS_COUNT_MIN, newSize));

    std::lock_guard<std::mutex> lock(m_mutex);
    RemoveUnusedThreads();
    m_coreThreads = newSize;

    size_t current_size = m_workerThreads.size();
    if (newSize > current_size) {
        for (size_t i = 0; i < newSize - current_size; ++i) {
            m_workerThreads.emplace_back([this]() {
                Worker();
            });
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
                return m_shutdown || !m_taskQueue.empty() || m_workerThreads.size() > m_coreThreads;
            });

            if ((m_shutdown || m_workerThreads.size() > m_coreThreads) && m_taskQueue.empty()) {
                m_threadsToJoin.insert(std::this_thread::get_id());
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
