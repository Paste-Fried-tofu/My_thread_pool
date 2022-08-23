#ifndef __SAFE_QUEUE_HPP__
#define __SAFE_QUEUE_HPP__

#include <mutex>
#include <queue>

template <typename T>
class safeQueue
{
private:
    std::queue<T> s_queue;
    std::mutex q_mutex;

public:
    safeQueue() = default;
    safeQueue(const safeQueue &other)
    {
        s_queue(other.s_queue);
    }
    ~safeQueue() = default;

    bool empty()
    {
        std::unique_lock<std::mutex> lock(q_mutex);
        return s_queue.empty();
    }

    size_t size()
    {
        std::unique_lock<std::mutex> lock(q_mutex);
        return s_queue.size();
    }

    void enqueue(const T &t)
    {
        std::unique_lock<std::mutex> lock(q_mutex);
        s_queue.push(t);
    }

    bool dequeue(T &t)
    {
        std::unique_lock<std::mutex> lock(q_mutex);
        if (s_queue.empty())
        {
            return false;
        }
        t = std::move(s_queue.front());

        s_queue.pop();
        return true;
    }
};

#endif