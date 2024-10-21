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
#include <future>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <condition_variable>

class ThreadPool {
public:
    ~ThreadPool();
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(const ThreadPool&&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&&) = delete;

    // 初始化原子
    explicit ThreadPool(int64_t min = 2, int64_t max = std::thread::hardware_concurrency());

    // 增加任务
    void add_task(const std::function<void()>& task);

    // 异步的增加
    template <typename T, typename... Args>
    auto add_task(T &&t, Args&&... args) -> std::future<std::result_of_t<T(Args...)>> {
        using return_type_ = std::result_of_t<T(Args...)>;

        auto tmp = std::make_shared<std::packaged_task<return_type_()>>(
            // 必须用完美转发
            std::bind(std::forward<T>(t), std::forward<Args>(args)...)
        );

        std::future<return_type_> res = tmp->get_future();
        {
            std::unique_lock locker(mutex_);
            tasks_.emplace([tmp]() mutable {
                try {
                    (*tmp)();
                }
                catch (const std::exception& e) {
                    std::cout << e.what() << std::endl;
                }
                });
        }
        condition_variable_.notify_one();
        return res;
    }


private:
    // 管理者线程的作用方案
    void manage();

    // 工作线程的作用方案
    void work();

private:
    // 参数
    std::atomic_bool is_stop_;

    std::atomic_int64_t min_size_;
    std::atomic_int64_t max_size_;

    std::atomic_int64_t free_size_;
    std::atomic_int64_t exit_size_;
    std::atomic_int64_t current_size_;

private:
    // 对象
    std::unique_ptr<std::thread> manager_;
    std::unordered_map<std::thread::id, std::thread> workers_;

    // 任务队列
    std::queue<std::function<void()>> tasks_;
    std::vector<std::thread::id> exit_task_ids_;

    // 线程安全
    std::mutex mutex_;
    std::mutex mutex_ids_;
    std::condition_variable condition_variable_;

};

#endif //THREADPOOL_HPP
