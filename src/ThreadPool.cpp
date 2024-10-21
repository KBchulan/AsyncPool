//
// Created by whx on 24-10-21.
//

#include <../include/ThreadPool.hpp>

ThreadPool::ThreadPool(int64_t m_min, int64_t m_max) :
    m_stop(false),
    m_minSize(m_min),m_maxSize(m_max),
    current_work_size(m_min),current_free_size(m_min)
{
    m_manager = std::make_unique<std::thread>(&ThreadPool::manager, this);

    for(auto i = 0; i < m_min; i++) {
        m_workers.emplace_back(&ThreadPool::worker, this);
    }

}

void ThreadPool::addTask(const std::function<void()>& task) {
    {
        std::lock_guard<std::mutex> lock_guard(m_queue_mutex);
        m_tasks.emplace(task);
    }
    m_condition.notify_one();
}


void ThreadPool::worker() {
    while(!m_stop.load()) {
        std::function<void()> task = nullptr;
        {
            std::unique_lock<std::mutex> locker(m_queue_mutex);

            // empty
            while (m_tasks.empty() && m_stop) {
                m_condition.wait(locker);
            }

            if (!m_tasks.empty()) {
                task = std::move(m_tasks.front());
                m_tasks.pop();
            }
        }

        if (task) {
            // 此处可以进行绑定器实现有参传递
            --current_free_size;
            task();
            ++current_free_size;
        }
    }
}

void ThreadPool::manager() {
    while(!m_stop.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}