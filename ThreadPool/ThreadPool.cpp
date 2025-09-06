
#include <condition_variable>
#include <functional>
#include <utility>

#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t numThreads) : m_shutdown(false)
{
    size_t threadCount = numThreads == 0 ? std::thread::hardware_concurrency() : numThreads;
    m_workerThreads.reserve(threadCount);

    auto worker = [this]() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait(lock, [this]() {
                    return m_shutdown || !m_taskQueue.empty();
                });
                if (m_shutdown && m_taskQueue.empty()) {
                    return;
                }
                task = std::move(m_taskQueue.front());
                m_taskQueue.pop();
            }
            if (task) {
                task();
            }
        }
    };

    for (size_t i = 0; i < numThreads; i++) {
        m_workerThreads.emplace_back(worker);
    }

}

ThreadPool::~ThreadPool()
{
    Shutdown();
}

void ThreadPool::Shutdown()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_shutdown) {
            return;
        }
        m_shutdown = true;
    }
    m_condition.notify_all();
    for (auto &worker : m_workerThreads) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}