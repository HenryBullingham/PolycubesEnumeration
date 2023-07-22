#pragma once

#include <list>
#include <mutex>

///As the writer of this code, helps me logically manage unlabelled scopes, ie for when mutexes should be unlocked
#define scope if(false){} else 

/// <summary>
/// A thread safe-queue that allows enqueing and dequeing from multiple producer / consumer threads
/// </summary>
/// <typeparam name="T"></typeparam>
/// <returns></returns>
template<typename T>
class thread_safe_queue
{
public:

    thread_safe_queue(int64_t bound) : m_size_bound(bound)
    {
    }

    thread_safe_queue() : thread_safe_queue(-1)
    {
    }

    /// <summary>
    /// blocks if size >= bound
    /// </summary>
    /// <param name="element"></param>
    inline void enqueue(T element)
    {
        std::list<T> node;
        node.push_back(std::move(element));

        while (true)
        {
            scope 
            {
                std::lock_guard<std::mutex> lock{ m_mutex };

                //Using 'splice' to avoid memory allocations in critical section
                if (m_size_bound < 0 || m_queue.size() < (size_t) m_size_bound)
                {
                    m_queue.splice(m_queue.end(), node);
                    return;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    /// <summary>
    /// Attempts to dequeue an element, blocks until an element in received
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <returns></returns>
    inline T blocking_dequeue()
    {
        T element;
        while (!dequeue(element))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return element;
    }

    /// <summary>
    /// Attempts to dequeue an element, blocks until mutex is acquired, returns true if the element is retrieved
    /// </summary>
    /// <param name="outElement"></param>
    /// <returns></returns>
    inline bool dequeue(T& outElement)
    {
        std::list<T> temp;
        
        scope
        {
            std::lock_guard<std::mutex> lock{ m_mutex };

            if (m_queue.size() < 1)
            {
                return false;
            }

            temp.splice(temp.end(), m_queue, m_queue.begin());
        }

        outElement = std::move(temp.front());

        temp.pop_front();

        return true;
    }

    /// <summary>
    /// Returns number of elements in queue. Note that because of threading, this can easily become incorrect
    /// </summary>
    /// <returns></returns>
    inline size_t size() const
    {
        std::lock_guard<std::mutex> lock{ m_mutex };
        return m_queue.size();
    }

private:
    std::list<T> m_queue;
    std::mutex m_mutex;
    int64_t m_size_bound;
};