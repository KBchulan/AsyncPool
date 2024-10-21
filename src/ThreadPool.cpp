//
// Created by whx on 24-10-21.
//

#include "../include/ThreadPool.hpp"

#include <chrono>
#include <iostream>

ThreadPool::ThreadPool(const int64_t min, const int64_t max) :
    is_stop_(false),
    min_size_(min), max_size_(max),
    free_size_(max), current_size_(max)
{
    // 创建管理者线程
    manager_ = std::make_unique<std::thread>(&ThreadPool::manage, this);

    // 创建工作线程
    for (auto i = 0; i < max; i++) {
        std::thread tmp(&ThreadPool::work, this);
        workers_[tmp.get_id()] = std::move(tmp);
    }
}

ThreadPool::~ThreadPool() {
    is_stop_ = true;
    condition_variable_.notify_all();

    // ReSharper disable once CppUseElementsView
    for (auto &[fst, snd] : workers_) {
        if(std::thread& it = snd;
            it.joinable()) {
            std::cout << "线程已销毁:" << std::this_thread::get_id() << std::endl;
            it.join();
        }
    }

    if (manager_->joinable()) {
        manager_->join();
    }
}

void ThreadPool::add_task(const std::function<void()>& task) {
    {
        std::lock_guard locker(mutex_);
        tasks_.emplace(task);
    }
    condition_variable_.notify_one();
}

void ThreadPool::manage() {
    while(!is_stop_.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        const int64_t free = free_size_.load();
        const int64_t current = current_size_.load();

        // 销毁线程
        if (free > current / 2 && current > min_size_) {
            exit_size_.store(2);
            condition_variable_.notify_all();

            std::lock_guard<std::mutex> locker_for_ids(mutex_ids_);

            for (const auto &id : exit_task_ids_) {
                if(workers_.contains(id)) {
                    std::cout << "----线程----：" <<
                           id << "被销毁" << std::endl;
                    workers_[id].join();
                    workers_.erase(id);
                }
            }

            exit_task_ids_.clear();

        }

        // 增加线程
        else if (free == 0 && current < max_size_) {
            std::thread tmp(&ThreadPool::work, this);
            workers_[tmp.get_id()] = std::move(tmp);
            ++current_size_;
            ++free_size_;
        }

    }
}

void ThreadPool::work() {
    while(!is_stop_.load()) {
        std::function<void()> task = nullptr;

        {
            std::unique_lock locker(mutex_);

            // 任务队列空
            while (tasks_.empty() && is_stop_) {
                condition_variable_.wait(locker);

                if (exit_size_) {
                    --current_size_;
                    --free_size_;
                    --exit_size_;
                    std::cout << "有线程退出了，ID是：" << std::this_thread::get_id() << std::endl;
                    std::lock_guard<std::mutex> locker_for_ids(mutex_ids_);
                    exit_task_ids_.push_back(std::this_thread::get_id());
                    return;
                }
            }

            if (!tasks_.empty()) {
                std::cout << "任务取出" << std::endl;
                task = std::move(tasks_.front());
                tasks_.pop();
            }
        }

        if (task) {
            // 可以考虑一下为什么不用后置
            --free_size_;
            task();
            ++free_size_;
        }
    }
}


