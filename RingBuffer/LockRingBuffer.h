#include <vector>
#include <mutex>
#include <condition_variable>

template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity);
    void Push(const T &);
    void Pop(T&);
private:
    std::vector<T> m_buffer;
    size_t m_size{0};
    size_t m_capacity;
    size_t m_head{0};
    size_t m_tail{0};
    std::mutex m_mutex;
    std::condition_variable m_notEmpty;
    std::condition_variable m_notFull;
};

template <typename T>
RingBuffer<T>::RingBuffer(size_t capacity) : m_capacity(capacity)
{
    m_buffer.resize(m_capacity);
}

template <typename T>
void RingBuffer<T>::Push(const T &buf)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_notFull.wait(lock, [this](){
        return m_size < m_capacity;
    });
    m_buffer[m_head] = buf;
    m_head = (m_head + 1) % m_capacity;
    m_size++;
    m_notEmpty.notify_one();
}

template <typename T>
void RingBuffer<T>::Pop(T& buf)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_notEmpty.wait(lock, [this](){
        return m_size > 0;
    });
    buf = m_buffer[m_tail];
    m_tail = (m_tail + 1) % m_capacity;
    m_size--;
    m_notFull.notify_one();
}
