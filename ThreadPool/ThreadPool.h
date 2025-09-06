#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <condition_variable>
#include <functional>
#include <queue>
#include <thread>
#include <atomic>

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads);
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool& operator=(ThreadPool &&) = delete;
    ~ThreadPool();

    template <typename F, typename... Args>
    void CommitTask(F &&f, Args &&...args) {
        auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_taskQueue.push(task);
        }
        m_condition.notify_one();
    }
    void Shutdown();

private:
    std::queue<std::function<void()>> m_taskQueue;
    std::vector<std::thread> m_workerThreads;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic_bool m_shutdown;
};

#endif