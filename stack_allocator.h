#pragma once

#include <cstdint>

#include <memory>

#ifdef GENERIC_STACK

class stack_allocator;

template<typename T>
class proxy
{
public:
    inline explicit proxy(T* ptr) : m_ptr(ptr)
    {
        new (m_ptr) T();
    }

    inline proxy(proxy<T>&& other)
    {
        m_ptr = other.m_ptr;
        other.m_ptr = nullptr;
    }

    proxy(const proxy&) = delete;
    proxy& operator=(const proxy&) = delete;
    proxy& operator=(proxy&&) = delete;

    inline ~proxy()
    {
        if (m_ptr)
        {
            m_ptr->~T();
        }
    }


    T& operator*() { return *m_ptr; }

    T* operator->() { return m_ptr; }

private:
    T* m_ptr;
};

class stack_allocator
{

public:

    inline stack_allocator(size_t size)
        : m_size_bytes(size), m_current_marker(0)
    {
        m_stack = std::unique_ptr<uint8_t[]>(new uint8_t[size]);
    }

    inline size_t get_marker()
    {
        return m_current_marker;
    }

    inline void set_marker(size_t marker)
    {
        m_current_marker = marker;
    }

    inline void* allocate(size_t bytes)
    {
        if (m_size_bytes - m_current_marker < bytes)
        {
            return nullptr;
        }

        void* mem = top();
        m_current_marker += bytes;
        return mem;
    }

    inline void* allocate_aligned(size_t bytes, size_t alignment)
    {
        size_t mask = alignment - 1;
        uintptr_t address = (uintptr_t) allocate(bytes + mask);

        return (void*)((address + mask) & ~(mask));
    }

    template<typename T>
    inline proxy<T> allocate()
    {
        return proxy<T>((T*)allocate_aligned(sizeof(T), alignof(T)));
    }

private:

    inline void* top() const { return &m_stack[m_current_marker]; }

    size_t m_size_bytes;
    size_t m_current_marker;
    std::unique_ptr<uint8_t[]> m_stack;
};

class stack_marker
{
public:

    inline stack_marker(stack_allocator& _allocator) : m_allocator(_allocator)
    {
        m_marker = _allocator.get_marker();
    }

    inline ~stack_marker()
    {
        m_allocator.set_marker(m_marker);
    }

private:
    size_t m_marker;
    stack_allocator& m_allocator;
};
#else 

/// <summary>
/// A stack allocator for only a specific type
/// </summary>
/// <typeparam name="T"></typeparam>
template<typename T, size_t N>
class stack_allocator_typed
{
public:
    inline size_t get_marker() const { return m_ptr; }
    inline void set_marker(size_t marker) { m_ptr = marker; }

    inline T* allocate() 
    { 
        if (m_ptr >= N)
        {
            return nullptr;
        }
        T* next = &stack[m_ptr++];

        //*next = T{};

        return next;
    }

private:
    std::unique_ptr<T[]> stack = std::unique_ptr<T[]>(new T[N]);
    size_t m_ptr = 0;
};

/// <summary>
/// An object used to automatically clear allocations on a stack when done with them
/// </summary>
/// <typeparam name="T"></typeparam>
template<typename T, size_t N>
class stack_marker_typed
{
public:

    inline stack_marker_typed(stack_allocator_typed<T, N>& _allocator) : m_allocator(_allocator)
    {
        m_marker = _allocator.get_marker();
    }

    inline ~stack_marker_typed()
    {
        m_allocator.set_marker(m_marker);
    }

private:
    size_t m_marker;
    stack_allocator_typed<T,N>& m_allocator;
};

#endif

