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

const size_t THREADS_COUNT_MIN = 1;
const size_t THREADS_COUNT_MAX = std::thread::hardware_concurrency();

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads = THREADS_COUNT_MAX);
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
    bool m_shutdown;
    size_t m_activeThreads;
    size_t m_coreThreads;
};

#endif