#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <climits>
#include <cstddef>
#include <mutex>
#include <utility>

template <typename T, size_t BlockSize = 4096, bool ThreadSafe = true>
class MemoryPool {
public:
    MemoryPool() noexcept;
    ~MemoryPool() noexcept;

    MemoryPool(const MemoryPool &memoryPool) = delete;
    MemoryPool &operator=(const MemoryPool &memoryPool) = delete;
    MemoryPool(MemoryPool &&memoryPool) = delete;
    MemoryPool &operator=(MemoryPool &&memoryPool) = delete;

    // 分配内存并构造一个元素
    template <typename... Args>
    T* NewElement(Args &&...args);
    // 析构一个元素并释放其内存
    void DeleteElement(T* p);
private:
    union Slot {
        T element;  // 占用状态，存元素
        Slot *next; // 空闲状态，存下一个slot的指针
    };

    Slot* m_firstBlock;     // 指向第一个内存块的指针
    Slot* m_currentSlot;    // 指向当前可用内存槽的指针
    Slot* m_lastSlot;       // 指向当前内存块最后一个可用槽的指针
    Slot* m_freeSlots;      // 指向已释放的内存槽链表的指针
    std::mutex m_mutex;     // 用于线程安全的互斥锁

    // 分配一个新的内存块
    void AllocateBlock();
    // 从内存池中分配一个元素的内存
    T* Allocate();
    // 将一个元素的内存返还给内存池
    void Deallocate(T* p);
    // 在指定的内存位置上构造一个对象
    template <typename U, typename... Args>
    void Construct(U* p, Args &&...args);
    // 析构指定内存位置上的对象
    template <typename U>
    void Destroy(U* p);
    // 计算为了内存对齐需要填充的字节数
    size_t Padding(char* p, size_t align) const noexcept;
};

template <typename T, size_t BlockSize, bool ThreadSafe>
MemoryPool<T, BlockSize, ThreadSafe>::MemoryPool() noexcept
{
    m_firstBlock = nullptr;
    m_currentSlot = nullptr;
    m_lastSlot = nullptr;
    m_freeSlots = nullptr;
}

template <typename T, size_t BlockSize, bool ThreadSafe>
MemoryPool<T, BlockSize, ThreadSafe>::~MemoryPool() noexcept
{
    Slot* cur = m_firstBlock;
    while (cur != nullptr) {
        Slot* next = cur->next;
        delete[] reinterpret_cast<char*>(cur);
        cur = next;
    }
}

template <typename T, size_t BlockSize, bool ThreadSafe>
void MemoryPool<T, BlockSize, ThreadSafe>::AllocateBlock()
{
    // 分配一个新的内存块
    char* newBlock = new char[BlockSize];
    Slot* newBlockSlot = reinterpret_cast<Slot*>(newBlock);

    // 将新块链接到块列表的头部
    newBlockSlot->next = m_firstBlock;
    m_firstBlock = newBlockSlot;

    // 计算新块中可用于存储元素的内存区域的起始和结束位置
    char* body = newBlock + sizeof(Slot*);
    size_t bodyPadding = Padding(body, alignof(Slot));
    m_currentSlot = reinterpret_cast<Slot*>(body + bodyPadding);
    m_lastSlot = reinterpret_cast<Slot*>(newBlock + BlockSize - sizeof(Slot) + 1);
}

template <typename T, size_t BlockSize, bool ThreadSafe>
inline T* MemoryPool<T, BlockSize, ThreadSafe>::Allocate()
{
    if constexpr (ThreadSafe) {
        std::lock_guard<std::mutex> lock(m_mutex);
    }
    if (m_freeSlots != nullptr) {
        T* result = reinterpret_cast<T*>(m_freeSlots);
        m_freeSlots = m_freeSlots->next;
        return result;
    } else {
        if (m_currentSlot >= m_lastSlot) {
            AllocateBlock();
        }
        return reinterpret_cast<T*>(m_currentSlot++);
    }
}

template <typename T, size_t BlockSize, bool ThreadSafe>
inline void MemoryPool<T, BlockSize, ThreadSafe>::Deallocate(T* p)
{
    if constexpr (ThreadSafe) {
        std::lock_guard<std::mutex> lock(m_mutex);
    }
    if (p != nullptr) {
        reinterpret_cast<Slot*>(p)->next = m_freeSlots;
        m_freeSlots = reinterpret_cast<Slot*>(p);
    }
}

template <typename T, size_t BlockSize, bool ThreadSafe>
template <typename U, typename... Args>
void MemoryPool<T, BlockSize, ThreadSafe>::Construct(U* p, Args&&... args)
{
    new (p) U(std::forward<Args>(args)...);
}

template <typename T, size_t BlockSize, bool ThreadSafe>
template <typename U>
void MemoryPool<T, BlockSize, ThreadSafe>::Destroy(U* p)
{
    p->~U();
}

template <typename T, size_t BlockSize, bool ThreadSafe>
template <typename... Args>
inline T* MemoryPool<T, BlockSize, ThreadSafe>::NewElement(Args&&... args)
{
    T* result = Allocate();
    Construct(result, std::forward<Args>(args)...);
    return result;
}

template <typename T, size_t BlockSize, bool ThreadSafe>
inline void MemoryPool<T, BlockSize, ThreadSafe>::DeleteElement(T* p)
{
    if (p != nullptr) {
        Destroy(p);
        Deallocate(p);
    }
}

template <typename T, size_t BlockSize, bool ThreadSafe>
inline size_t MemoryPool<T, BlockSize, ThreadSafe>::Padding(char* p, size_t align) const noexcept
{
    // result是p指向的内存地址的数值表示
    size_t result = reinterpret_cast<size_t>(p);
    // result % align: 当前地址result相对于align的余数
    auto mod = result % align;
    // align - (result % align): 从当前地址result开始，还需要多少个字节才能达到下一个align的倍数。
    return mod ? align - mod : 0;
}

#endif