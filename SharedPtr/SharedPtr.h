#ifndef SHARED_PTR_H
#define SHARED_PTR_H

#include <atomic>
#include <cassert>
#include <cstddef>
#include <utility>

struct ControlBlock {
    std::atomic<size_t> m_refCount{1};
};

template <typename T>
class SharedPtr {
public:
    SharedPtr();
    explicit SharedPtr(T* p);
    ~SharedPtr();

    SharedPtr(const SharedPtr& other);
    SharedPtr& operator=(const SharedPtr& other);

    SharedPtr(SharedPtr&& other) noexcept;
    SharedPtr& operator=(SharedPtr&& other) noexcept;

    T* Get() const;
    long UseCount() const;
    bool Unique() const;
    explicit operator bool() const;

    void Reset();
    void Reset(T* p);

    T& operator*() const;
    T* operator->() const;

private:
    void Release();
    void swap(SharedPtr& other) noexcept;

    T* m_data{nullptr};
    ControlBlock* m_block{nullptr};
};

template <typename T>
SharedPtr<T>::SharedPtr() : m_data(nullptr), m_block(nullptr)
{
}

template <typename T>
SharedPtr<T>::SharedPtr(T* p) : m_data(p)
{
    if (m_data) {
        m_block = new ControlBlock();
    }
}

template <typename T>
SharedPtr<T>::~SharedPtr()
{
    Release();
}

template <typename T>
void SharedPtr<T>::Release()
{
    if (m_block && m_block->m_refCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        delete m_data;
        delete m_block;
    }
    m_data = nullptr;
    m_block = nullptr;
}

template <typename T>
SharedPtr<T>::SharedPtr(const SharedPtr& other) : m_data(other.m_data), m_block(other.m_block)
{
    if (m_block) {
        m_block->m_refCount.fetch_add(1, std::memory_order_relaxed);
    }
}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr& other)
{
    // copy and swap
    SharedPtr(other).swap(*this);
    return *this;
}

template <typename T>
SharedPtr<T>::SharedPtr(SharedPtr&& other) noexcept : m_data(other.m_data), m_block(other.m_block)
{
    other.m_data = nullptr;
    other.m_block = nullptr;
}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr&& other) noexcept
{
    // move and swap
    SharedPtr(std::move(other)).swap(*this);
    return *this;
}

template <typename T>
void SharedPtr<T>::swap(SharedPtr& other) noexcept
{
    std::swap(m_data, other.m_data);
    std::swap(m_block, other.m_block);
}

template <typename T>
T* SharedPtr<T>::Get() const
{
    return m_data;
}

template <typename T>
long SharedPtr<T>::UseCount() const
{
    if (m_block) {
        return m_block->m_refCount.load(std::memory_order_relaxed);
    }
    return 0;
}

template <typename T>
bool SharedPtr<T>::Unique() const
{
    return UseCount() == 1;
}

template <typename T>
void SharedPtr<T>::Reset()
{
    Release();
}

template <typename T>
void SharedPtr<T>::Reset(T* p)
{
    SharedPtr(p).swap(*this);
}

template <typename T>
T& SharedPtr<T>::operator*() const
{
    return *m_data;
}

template <typename T>
T* SharedPtr<T>::operator->() const
{
    return m_data;
}

template <typename T>
SharedPtr<T>::operator bool() const
{
    return m_data != nullptr;
}

#endif