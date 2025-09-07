#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency());
    ~ThreadPool();
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    template <typename F, typename... Args>
    void CommitTask(F &&f, Args &&...args) {
        auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_taskQueue.emplace(task);
        }
        m_condition.notify_one();
    }
    void Resize(size_t newSize);

private:
    void Worker();

    std::queue<std::function<void()>> m_taskQueue;
    std::vector<std::thread> m_workerThreads;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic_bool m_shutdown;
    std::atomic_size_t m_activeThreads;
    size_t m_coreThreads;
};

#endif