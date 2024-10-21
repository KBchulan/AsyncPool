//
// Created by whx on 24-10-21.
//

#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <queue>
#include <atomic>
#include <vector>
#include <memory>
#include <thread>
#include <functional>
#include <condition_variable>

class ThreadPool {
public:
    ThreadPool() = default;
    explicit ThreadPool(int64_t m_min = 2, int64_t m_max = static_cast<int64_t>(std::thread::hardware_concurrency()));
    ~ThreadPool();

    void addTask(const std::function<void()>& task);

private:
    void worker();
    void manager();

private:
    // main control
    std::atomic_bool m_stop;

     // size is not sure
    std::atomic_int64_t m_minSize = 0;
    std::atomic_int64_t m_maxSize = 0;

    std::atomic_int64_t current_work_size = 0;
    std::atomic_int64_t current_free_size = 0;

private:
    // sync
    std::mutex m_queue_mutex;
    std::condition_variable m_condition;

    std::vector<std::thread> m_workers;
    std::unique_ptr<std::thread> m_manager;

    std::queue<std::function<void()>> m_tasks;
};

#endif //THREADPOOL_HPP
