#ifndef __THREAD_POOL_HPP__
#define __THREAD_POOL_HPP__

#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <future>
#include <utility>
#include <vector>
#include "safeQueue.hpp"

class threadPool
{
private:
    class threadWorker
    {
    private:
        int tid;
        threadPool *pool;

    public:
        threadWorker(threadPool *thread_pool, const int id)
            : pool(thread_pool), tid(id) {}
        void operator()()
        {
            std::function<void()> func;
            bool dequeued;
            while (!pool->isShutdown)
            {
                {
                    std::unique_lock<std::mutex> lock(pool->mutex);
                    if (pool->tasks.empty())
                    {
                        pool->condition.wait(lock);
                    }
                    dequeued = pool->tasks.dequeue(func);
                }
                if (dequeued)
                {
                    func();
                }
            }
        }
    };
    bool isShutdown;
    safeQueue<std::function<void()>> tasks;
    std::vector<std::thread> workers;
    std::mutex mutex;
    std::condition_variable condition;

public:
    threadPool(const int threadsNum)
        : workers(std::vector<std::thread>(threadsNum)), isShutdown(false) {}
    threadPool(const threadPool &) = delete;
    threadPool(threadPool &&) = delete;
    threadPool &operator=(const threadPool &) = delete;
    threadPool &operator=(threadPool &&) = delete;

    void init()
    {
        for (size_t i = 0; i < workers.size(); ++i)
        {
            workers[i] = std::thread(threadWorker(this, i));
        }
    }

    void shutdown()
    {
        isShutdown = true;
        condition.notify_all();
        for (size_t i = 0; i < workers.size(); ++i)
        {
            if (workers[i].joinable())
            {
                workers[i].join();
            }
        }
    }

    template <typename F, typename... Args>
    auto submit(F &&f, Args &&...args) -> std::future<decltype(f(args...))>
    {
        std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
        std::function<void()> wrapper_func = [task_ptr]()
        {
            (*task_ptr)();
        };
        tasks.enqueue(wrapper_func);
        condition.notify_one();
        return task_ptr->get_future();
    }
};

#endif